#include "encode_vt.hpp"

#include "vau.hpp"

#include <CoreFoundation/CoreFoundation.h>
#include <CoreMedia/CoreMedia.h>
#include <CoreVideo/CoreVideo.h>
#include <VideoToolbox/VideoToolbox.h>

#include <cstdint>
#include <cstring>
#include <fstream>
#include <mutex>
#include <sstream>
#include <utility>

namespace learning::video::encode {
namespace {

std::string osstatus(const char* what, OSStatus status) {
  std::ostringstream oss;
  oss << what << " OSStatus=" << static_cast<int>(status);
  return oss.str();
}

struct CfRelease {
  CFTypeRef ref = nullptr;
  ~CfRelease() {
    if (ref != nullptr) {
      CFRelease(ref);
    }
  }
  CfRelease() = default;
  explicit CfRelease(CFTypeRef value) : ref(value) {}
  CfRelease(const CfRelease&) = delete;
  CfRelease& operator=(const CfRelease&) = delete;
};

struct SessionRelease {
  VTCompressionSessionRef session = nullptr;
  ~SessionRelease() {
    if (session != nullptr) {
      VTCompressionSessionInvalidate(session);
      CFRelease(session);
    }
  }
};

struct PixelRelease {
  CVPixelBufferRef buffer = nullptr;
  ~PixelRelease() {
    if (buffer != nullptr) {
      CVPixelBufferRelease(buffer);
    }
  }
};

struct EncodeContext {
  const std::vector<RawFrame>* frames = nullptr;
  std::mutex mu;
  std::string error;
  std::vector<AccessUnit> access_units;
  std::vector<std::uint8_t> got;
  std::vector<ParameterSet> parameter_sets;
};

void set_error(EncodeContext& ctx, const std::string& message) {
  std::lock_guard<std::mutex> lock(ctx.mu);
  if (ctx.error.empty()) {
    ctx.error = message;
  }
}

bool copy_parameter_sets(CMFormatDescriptionRef format,
                         std::vector<ParameterSet>& sets, std::string& error) {
  size_t count = 0;
  int nal_length = 0;
  const uint8_t* first = nullptr;
  size_t first_size = 0;
  const OSStatus first_status = CMVideoFormatDescriptionGetH264ParameterSetAtIndex(
      format, 0, &first, &first_size, &count, &nal_length);
  if (first_status != noErr) {
    error = osstatus("CMVideoFormatDescriptionGetH264ParameterSetAtIndex",
                     first_status);
    return false;
  }
  if (nal_length != 4) {
    error = "H.264 NAL length size is not 4";
    return false;
  }
  if (count == 0 || count > 8 || first == nullptr || first_size == 0) {
    error = "H.264 parameter set count out of range";
    return false;
  }

  sets.resize(count);
  sets[0].nal.assign(first, first + first_size);
  for (size_t i = 1; i < count; ++i) {
    const uint8_t* ptr = nullptr;
    size_t size = 0;
    size_t ignored_count = 0;
    int ignored_nal = 0;
    const OSStatus status = CMVideoFormatDescriptionGetH264ParameterSetAtIndex(
        format, i, &ptr, &size, &ignored_count, &ignored_nal);
    if (status != noErr || ptr == nullptr || size == 0) {
      error = osstatus("CMVideoFormatDescriptionGetH264ParameterSetAtIndex",
                       status);
      return false;
    }
    sets[i].nal.assign(ptr, ptr + size);
  }
  return true;
}

extern "C" void encode_output_callback(void* output_refcon, void* source_refcon,
                                       OSStatus status, VTEncodeInfoFlags info_flags,
                                       CMSampleBufferRef sample_buffer) {
  auto* ctx = static_cast<EncodeContext*>(output_refcon);
  if (ctx == nullptr) {
    return;
  }
  if (status != noErr) {
    set_error(*ctx, osstatus("VTCompressionSession output", status));
    return;
  }
  if ((info_flags & kVTEncodeInfo_FrameDropped) != 0 || sample_buffer == nullptr) {
    set_error(*ctx, "encoder dropped a frame");
    return;
  }
  if (!CMSampleBufferDataIsReady(sample_buffer)) {
    set_error(*ctx, "encoded sample buffer is not ready");
    return;
  }
  if (CMSampleBufferGetNumSamples(sample_buffer) != 1) {
    set_error(*ctx, "encoded sample buffer does not contain one sample");
    return;
  }

  CMFormatDescriptionRef format =
      CMSampleBufferGetFormatDescription(sample_buffer);
  if (format == nullptr) {
    set_error(*ctx, "encoded sample has no format description");
    return;
  }
  std::vector<ParameterSet> sets;
  std::string sets_error;
  if (!copy_parameter_sets(format, sets, sets_error)) {
    set_error(*ctx, sets_error);
    return;
  }

  CMBlockBufferRef block = CMSampleBufferGetDataBuffer(sample_buffer);
  if (block == nullptr) {
    set_error(*ctx, "encoded sample has no data buffer");
    return;
  }
  const size_t length = CMBlockBufferGetDataLength(block);
  if (length == 0 || length > kMaxAccessUnitBytes) {
    set_error(*ctx, "encoded access unit size out of range");
    return;
  }
  std::vector<std::uint8_t> payload(length);
  const OSStatus copy_status =
      CMBlockBufferCopyDataBytes(block, 0, length, payload.data());
  if (copy_status != noErr) {
    set_error(*ctx, osstatus("CMBlockBufferCopyDataBytes", copy_status));
    return;
  }

  bool keyframe = true;
  CFArrayRef attachments =
      CMSampleBufferGetSampleAttachmentsArray(sample_buffer, false);
  if (attachments != nullptr && CFArrayGetCount(attachments) > 0) {
    auto dict = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(attachments, 0));
    auto not_sync = static_cast<CFBooleanRef>(
        CFDictionaryGetValue(dict, kCMSampleAttachmentKey_NotSync));
    if (not_sync != nullptr && CFBooleanGetValue(not_sync)) {
      keyframe = false;
    }
  }

  const auto index =
      reinterpret_cast<std::uintptr_t>(source_refcon);
  if (ctx->frames == nullptr || index == 0 || index > ctx->frames->size()) {
    set_error(*ctx, "encoder callback index is out of range");
    return;
  }

  AccessUnit unit;
  unit.pts_ns = (*ctx->frames)[index - 1].pts_ns;
  unit.keyframe = keyframe;
  unit.payload = std::move(payload);

  std::lock_guard<std::mutex> lock(ctx->mu);
  if (!ctx->error.empty()) {
    return;
  }
  if (ctx->parameter_sets.empty()) {
    ctx->parameter_sets = std::move(sets);
  } else if (ctx->parameter_sets.size() != sets.size()) {
    ctx->error = "H.264 parameter sets changed mid-stream";
    return;
  } else {
    for (size_t i = 0; i < sets.size(); ++i) {
      if (ctx->parameter_sets[i].nal != sets[i].nal) {
        ctx->error = "H.264 parameter sets changed mid-stream";
        return;
      }
    }
  }
  if (ctx->got[index - 1] != 0) {
    ctx->error = "encoder emitted the same frame twice";
    return;
  }
  ctx->access_units[index - 1] = std::move(unit);
  ctx->got[index - 1] = 1;
}

CFDictionaryRef make_source_attrs() {
  const int32_t width = kWidth;
  const int32_t height = kHeight;
  const int32_t pixel = kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange;
  CfRelease width_num(CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type,
                                     &width));
  CfRelease height_num(CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type,
                                      &height));
  CfRelease pixel_num(
      CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &pixel));
  CfRelease io_surface(CFDictionaryCreate(
      kCFAllocatorDefault, nullptr, nullptr, 0, &kCFTypeDictionaryKeyCallBacks,
      &kCFTypeDictionaryValueCallBacks));
  const void* keys[] = {
      kCVPixelBufferPixelFormatTypeKey,
      kCVPixelBufferWidthKey,
      kCVPixelBufferHeightKey,
      kCVPixelBufferIOSurfacePropertiesKey,
  };
  const void* values[] = {pixel_num.ref, width_num.ref, height_num.ref,
                          io_surface.ref};
  return CFDictionaryCreate(kCFAllocatorDefault, keys, values, 4,
                            &kCFTypeDictionaryKeyCallBacks,
                            &kCFTypeDictionaryValueCallBacks);
}

bool set_session_properties(VTCompressionSessionRef session, std::string& error) {
  const int32_t bitrate = static_cast<int32_t>(kBitrateBps);
  const int32_t frame_rate = kFrameRate;
  const int32_t key_interval = kMaxKeyFrameInterval;
  CfRelease bitrate_num(
      CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &bitrate));
  CfRelease fps_num(
      CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &frame_rate));
  CfRelease key_num(
      CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &key_interval));

  const struct Item {
    CFStringRef key;
    CFTypeRef value;
    const char* name;
  } items[] = {
      {kVTCompressionPropertyKey_AverageBitRate, bitrate_num.ref,
       "AverageBitRate"},
      {kVTCompressionPropertyKey_ExpectedFrameRate, fps_num.ref,
       "ExpectedFrameRate"},
      {kVTCompressionPropertyKey_MaxKeyFrameInterval, key_num.ref,
       "MaxKeyFrameInterval"},
      {kVTCompressionPropertyKey_AllowFrameReordering, kCFBooleanFalse,
       "AllowFrameReordering"},
      {kVTCompressionPropertyKey_RealTime, kCFBooleanFalse, "RealTime"},
      {kVTCompressionPropertyKey_ProfileLevel,
       kVTProfileLevel_H264_Baseline_AutoLevel, "ProfileLevel"},
  };
  for (const auto& item : items) {
    const OSStatus status = VTSessionSetProperty(session, item.key, item.value);
    if (status != noErr) {
      error = osstatus(item.name, status);
      return false;
    }
  }
  const OSStatus prepared = VTCompressionSessionPrepareToEncodeFrames(session);
  if (prepared != noErr) {
    error = osstatus("VTCompressionSessionPrepareToEncodeFrames", prepared);
    return false;
  }
  return true;
}

bool make_pixel_buffer(const RawFrame& frame, CVPixelBufferRef& out,
                       std::string& error) {
  out = nullptr;
  CfRelease attrs(make_source_attrs());
  CVPixelBufferRef buffer = nullptr;
  const CVReturn created = CVPixelBufferCreate(
      kCFAllocatorDefault, static_cast<size_t>(kWidth),
      static_cast<size_t>(kHeight),
      kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange,
      static_cast<CFDictionaryRef>(attrs.ref), &buffer);
  if (created != kCVReturnSuccess || buffer == nullptr) {
    error = "CVPixelBufferCreate failed: " + std::to_string(created);
    return false;
  }

  if (CVPixelBufferLockBaseAddress(buffer, 0) != kCVReturnSuccess) {
    CVPixelBufferRelease(buffer);
    error = "CVPixelBufferLockBaseAddress failed";
    return false;
  }
  auto* y_dst = static_cast<std::uint8_t*>(CVPixelBufferGetBaseAddressOfPlane(buffer, 0));
  auto* uv_dst =
      static_cast<std::uint8_t*>(CVPixelBufferGetBaseAddressOfPlane(buffer, 1));
  const size_t y_stride = CVPixelBufferGetBytesPerRowOfPlane(buffer, 0);
  const size_t uv_stride = CVPixelBufferGetBytesPerRowOfPlane(buffer, 1);
  if (y_dst == nullptr || uv_dst == nullptr || y_stride < static_cast<size_t>(kWidth) ||
      uv_stride < static_cast<size_t>(kWidth)) {
    CVPixelBufferUnlockBaseAddress(buffer, 0);
    CVPixelBufferRelease(buffer);
    error = "CVPixelBuffer plane layout is not biplanar NV12";
    return false;
  }
  for (int row = 0; row < kHeight; ++row) {
    std::memcpy(y_dst + static_cast<size_t>(row) * y_stride,
                frame.nv12.data() + static_cast<size_t>(row) * kWidth,
                static_cast<size_t>(kWidth));
  }
  const std::uint8_t* uv_src =
      frame.nv12.data() + static_cast<size_t>(kWidth) * kHeight;
  const int chroma_h = kHeight / 2;
  for (int row = 0; row < chroma_h; ++row) {
    std::memcpy(uv_dst + static_cast<size_t>(row) * uv_stride,
                uv_src + static_cast<size_t>(row) * kWidth,
                static_cast<size_t>(kWidth));
  }
  CVPixelBufferUnlockBaseAddress(buffer, 0);

  CVBufferSetAttachment(buffer, kCVImageBufferColorPrimariesKey,
                        kCVImageBufferColorPrimaries_ITU_R_709_2,
                        kCVAttachmentMode_ShouldPropagate);
  CVBufferSetAttachment(buffer, kCVImageBufferTransferFunctionKey,
                        kCVImageBufferTransferFunction_ITU_R_709_2,
                        kCVAttachmentMode_ShouldPropagate);
  CVBufferSetAttachment(buffer, kCVImageBufferYCbCrMatrixKey,
                        kCVImageBufferYCbCrMatrix_ITU_R_709_2,
                        kCVAttachmentMode_ShouldPropagate);
  out = buffer;
  return true;
}

bool publish_tree(const std::filesystem::path& root, const std::vector<std::uint8_t>& vau,
                  const std::string& manifest, std::uint64_t& output_bytes,
                  std::string& error) {
  namespace fs = std::filesystem;
  const fs::path staging = root / "out/video/encode.staging";
  const fs::path publish = root / "out/video/encode";
  const fs::path aside = root / "out/video/encode.aside";
  std::error_code ec;
  fs::remove_all(staging, ec);
  fs::create_directories(staging, ec);
  if (ec) {
    error = "failed to create encode staging: " + ec.message();
    return false;
  }

  const fs::path vau_path = staging / "encode.vau";
  {
    std::ofstream out(vau_path, std::ios::binary);
    if (!out) {
      error = "failed to open " + vau_path.string();
      fs::remove_all(staging, ec);
      return false;
    }
    out.write(reinterpret_cast<const char*>(vau.data()),
              static_cast<std::streamsize>(vau.size()));
    if (!out.good()) {
      error = "failed while writing " + vau_path.string();
      fs::remove_all(staging, ec);
      return false;
    }
  }
  {
    std::ofstream out(staging / "manifest.csv");
    if (!out) {
      error = "failed to open encode manifest";
      fs::remove_all(staging, ec);
      return false;
    }
    out << manifest;
    if (!out.good()) {
      error = "failed while writing encode manifest";
      fs::remove_all(staging, ec);
      return false;
    }
  }

  fs::remove_all(aside, ec);
  const bool had_publish = fs::exists(publish);
  if (had_publish) {
    fs::rename(publish, aside, ec);
    if (ec) {
      error = "failed to move aside prior encode dir: " + ec.message();
      fs::remove_all(staging, ec);
      return false;
    }
  }
  fs::create_directories(publish.parent_path(), ec);
  fs::rename(staging, publish, ec);
  if (ec) {
    error = "failed to publish encode dir: " + ec.message();
    if (had_publish) {
      std::error_code restore_ec;
      fs::rename(aside, publish, restore_ec);
    }
    fs::remove_all(staging, ec);
    return false;
  }
  fs::remove_all(aside, ec);
  output_bytes = static_cast<std::uint64_t>(fs::file_size(publish / "encode.vau"));
  return true;
}

bool nal_type_present(const std::vector<ParameterSet>& sets, int type) {
  for (const auto& set : sets) {
    if (!set.nal.empty() && (set.nal[0] & 0x1f) == type) {
      return true;
    }
  }
  return false;
}

}  // namespace

bool encode_nv12_to_vau(const std::filesystem::path& root,
                        const std::vector<RawFrame>& frames, EncodeStats& stats,
                        std::string& error) {
  stats = {};
  stats.width = kWidth;
  stats.height = kHeight;
  stats.bitrate_bps = kBitrateBps;

  if (frames.empty() || frames.size() > static_cast<std::size_t>(kMaxFrames)) {
    error = "encode frame count must be in [1, 90]";
    return false;
  }
  std::int64_t prev_pts = -1;
  for (std::size_t i = 0; i < frames.size(); ++i) {
    if (frames[i].nv12.size() != kFrameBytes) {
      error = "frame " + std::to_string(i) + " is not 1382400 tight NV12 bytes";
      return false;
    }
    if (frames[i].pts_ns < 0 || (i > 0 && frames[i].pts_ns <= prev_pts)) {
      error = "frame pts_ns must be >= 0 and strictly increasing";
      return false;
    }
    prev_pts = frames[i].pts_ns;
  }

  EncodeContext ctx;
  ctx.frames = &frames;
  ctx.access_units.resize(frames.size());
  ctx.got.assign(frames.size(), 0);
  SessionRelease session;
  CfRelease source_attrs(make_source_attrs());
  const void* spec_keys[] = {
      kVTVideoEncoderSpecification_EnableHardwareAcceleratedVideoEncoder};
  const void* spec_values[] = {kCFBooleanFalse};
  CfRelease encoder_spec(CFDictionaryCreate(
      kCFAllocatorDefault, spec_keys, spec_values, 1,
      &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
  const OSStatus created = VTCompressionSessionCreate(
      kCFAllocatorDefault, kWidth, kHeight, kCMVideoCodecType_H264,
      static_cast<CFDictionaryRef>(encoder_spec.ref),
      static_cast<CFDictionaryRef>(source_attrs.ref), nullptr,
      encode_output_callback, &ctx, &session.session);
  if (created != noErr || session.session == nullptr) {
    error = osstatus("VTCompressionSessionCreate", created);
    return false;
  }
  if (!set_session_properties(session.session, error)) {
    return false;
  }

  for (std::size_t i = 0; i < frames.size(); ++i) {
    PixelRelease pixel;
    if (!make_pixel_buffer(frames[i], pixel.buffer, error)) {
      return false;
    }
    CfRelease force_key;
    if (i == 0) {
      const void* keys[] = {kVTEncodeFrameOptionKey_ForceKeyFrame};
      const void* values[] = {kCFBooleanTrue};
      force_key.ref = CFDictionaryCreate(kCFAllocatorDefault, keys, values, 1,
                                         &kCFTypeDictionaryKeyCallBacks,
                                         &kCFTypeDictionaryValueCallBacks);
    }
    // Encoder clock is the frame index at kFrameRate. The file stores the
    // capture pts_ns from `frames`, not this CMTime (host PTS can be huge).
    const CMTime pts = CMTimeMake(static_cast<std::int64_t>(i), kFrameRate);
    const CMTime duration = CMTimeMake(1, kFrameRate);
    const OSStatus encoded = VTCompressionSessionEncodeFrame(
        session.session, pixel.buffer, pts, duration,
        static_cast<CFDictionaryRef>(force_key.ref),
        reinterpret_cast<void*>(static_cast<std::uintptr_t>(i + 1)), nullptr);
    if (encoded != noErr) {
      error = osstatus("VTCompressionSessionEncodeFrame", encoded);
      return false;
    }
    {
      std::lock_guard<std::mutex> lock(ctx.mu);
      if (!ctx.error.empty()) {
        error = ctx.error;
        return false;
      }
    }
  }

  const OSStatus completed =
      VTCompressionSessionCompleteFrames(session.session, kCMTimeInvalid);
  if (completed != noErr) {
    error = osstatus("VTCompressionSessionCompleteFrames", completed);
    return false;
  }

  std::vector<AccessUnit> units;
  std::vector<ParameterSet> sets;
  {
    std::lock_guard<std::mutex> lock(ctx.mu);
    if (!ctx.error.empty()) {
      error = ctx.error;
      return false;
    }
    units = std::move(ctx.access_units);
    sets = std::move(ctx.parameter_sets);
  }
  if (units.size() != frames.size()) {
    error = "encoder access unit count does not match input frames";
    return false;
  }
  for (std::size_t i = 0; i < frames.size(); ++i) {
    if (ctx.got[i] == 0) {
      error = "encoder did not emit access unit " + std::to_string(i);
      return false;
    }
    if (units[i].pts_ns != frames[i].pts_ns) {
      error = "encoder PTS does not match input pts_ns";
      return false;
    }
  }
  if (!units.front().keyframe) {
    error = "encoder did not emit a keyframe for the first frame";
    return false;
  }
  if (!nal_type_present(sets, 7) || !nal_type_present(sets, 8)) {
    error = "encoder parameter sets are missing SPS or PPS";
    return false;
  }

  VauFile file;
  file.width = static_cast<std::uint32_t>(kWidth);
  file.height = static_cast<std::uint32_t>(kHeight);
  file.bitrate_bps = kBitrateBps;
  file.parameter_sets = std::move(sets);
  file.access_units = std::move(units);

  const std::filesystem::path partial =
      root / "out/video/encode.staging/encode.vau";
  if (!write_vau(partial, file, error)) {
    std::error_code ec;
    std::filesystem::remove_all(root / "out/video/encode.staging", ec);
    return false;
  }

  std::ostringstream manifest;
  manifest << "au_index,pts_ns,is_keyframe,bytes\n";
  std::uint32_t keyframes = 0;
  for (std::size_t i = 0; i < file.access_units.size(); ++i) {
    if (file.access_units[i].keyframe) {
      ++keyframes;
    }
    manifest << i << ',' << file.access_units[i].pts_ns << ','
             << (file.access_units[i].keyframe ? 1 : 0) << ','
             << file.access_units[i].payload.size() << '\n';
  }

  std::vector<std::uint8_t> vau_bytes;
  {
    std::ifstream vau_in(partial, std::ios::binary);
    vau_bytes.assign(std::istreambuf_iterator<char>(vau_in),
                     std::istreambuf_iterator<char>());
  }
  if (vau_bytes.empty()) {
    error = "encoded VAU is empty";
    std::error_code ec;
    std::filesystem::remove_all(root / "out/video/encode.staging", ec);
    return false;
  }

  std::uint64_t output_bytes = 0;
  if (!publish_tree(root, vau_bytes, manifest.str(), output_bytes, error)) {
    return false;
  }

  stats.frames_encoded = static_cast<std::uint32_t>(frames.size());
  stats.access_units = stats.frames_encoded;
  stats.keyframes = keyframes;
  stats.output_bytes = output_bytes;
  error.clear();
  return true;
}

}  // namespace learning::video::encode
