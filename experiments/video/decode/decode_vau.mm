#include "decode_vau.hpp"

#include "vau.hpp"

#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <CoreImage/CoreImage.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>
#import <ImageIO/ImageIO.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <VideoToolbox/VideoToolbox.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <mutex>
#include <sstream>
#include <vector>

namespace learning::video::decode {
namespace {

std::string osstatus(const char* what, OSStatus status) {
  std::ostringstream oss;
  oss << what << " OSStatus=" << static_cast<int>(status);
  return oss.str();
}

std::string fourcc_string(OSType fmt) {
  char text[5] = {
      static_cast<char>((fmt >> 24) & 0xff),
      static_cast<char>((fmt >> 16) & 0xff),
      static_cast<char>((fmt >> 8) & 0xff),
      static_cast<char>(fmt & 0xff),
      0,
  };
  return text;
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

struct Frame {
  std::vector<std::uint8_t> nv12;
  std::int64_t pts_ns = 0;
};

struct DecodeSlot {
  std::mutex mu;
  bool got = false;
  std::string error;
  std::vector<std::uint8_t> nv12;
};

bool copy_tight_nv12(CVImageBufferRef image, std::vector<std::uint8_t>& out,
                     std::string& error) {
  auto* buffer = static_cast<CVPixelBufferRef>(image);
  const OSType format = CVPixelBufferGetPixelFormatType(buffer);
  if (format != kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange) {
    error = "decoded pixel format is not 420v: " + fourcc_string(format);
    return false;
  }
  const size_t width = CVPixelBufferGetWidth(buffer);
  const size_t height = CVPixelBufferGetHeight(buffer);
  if (width != static_cast<size_t>(kWidth) ||
      height != static_cast<size_t>(kHeight) ||
      CVPixelBufferGetPlaneCount(buffer) != 2) {
    error = "decoded frame is not biplanar 1280x720";
    return false;
  }
  if (CVPixelBufferLockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly) !=
      kCVReturnSuccess) {
    error = "CVPixelBufferLockBaseAddress failed";
    return false;
  }
  const auto* y_src = static_cast<const std::uint8_t*>(
      CVPixelBufferGetBaseAddressOfPlane(buffer, 0));
  const auto* uv_src = static_cast<const std::uint8_t*>(
      CVPixelBufferGetBaseAddressOfPlane(buffer, 1));
  const size_t y_stride = CVPixelBufferGetBytesPerRowOfPlane(buffer, 0);
  const size_t uv_stride = CVPixelBufferGetBytesPerRowOfPlane(buffer, 1);
  if (y_src == nullptr || uv_src == nullptr || y_stride < width ||
      uv_stride < width) {
    CVPixelBufferUnlockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
    error = "decoded NV12 plane layout is unusable";
    return false;
  }
  out.assign(kFrameBytes, 0);
  for (int row = 0; row < kHeight; ++row) {
    std::memcpy(out.data() + static_cast<size_t>(row) * width,
                y_src + static_cast<size_t>(row) * y_stride, width);
  }
  std::uint8_t* uv_dst = out.data() + static_cast<size_t>(kWidth) * kHeight;
  const int chroma_h = kHeight / 2;
  for (int row = 0; row < chroma_h; ++row) {
    std::memcpy(uv_dst + static_cast<size_t>(row) * width,
                uv_src + static_cast<size_t>(row) * uv_stride, width);
  }
  CVPixelBufferUnlockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
  error.clear();
  return true;
}

extern "C" void learning_video_decode_output(void* refcon, void* source_refcon,
                                             OSStatus status,
                                             VTDecodeInfoFlags info_flags,
                                             CVImageBufferRef image,
                                             CMTime presentation_ts,
                                             CMTime presentation_duration) {
  (void)source_refcon;
  (void)presentation_ts;
  (void)presentation_duration;
  auto* slot = static_cast<DecodeSlot*>(refcon);
  if (slot == nullptr) {
    return;
  }
  std::lock_guard<std::mutex> lock(slot->mu);
  slot->got = true;
  if (status != noErr) {
    slot->error = osstatus("VTDecompressionSession output", status);
    return;
  }
  if ((info_flags & kVTDecodeInfo_FrameDropped) != 0 || image == nullptr) {
    slot->error = "decoder dropped a frame";
    return;
  }
  std::string copy_error;
  if (!copy_tight_nv12(image, slot->nv12, copy_error)) {
    slot->error = copy_error;
  }
}

}  // namespace

bool write_preview_png_from_nv12(const std::filesystem::path& png_path,
                                 const std::uint8_t* nv12, std::size_t nbytes,
                                 std::string& error) {
  if (nv12 == nullptr || nbytes != kFrameBytes) {
    error = "invalid NV12 for PNG preview";
    return false;
  }
  const std::size_t y_bytes =
      static_cast<std::size_t>(kWidth) * static_cast<std::size_t>(kHeight);
  NSDictionary* attrs = @{
    (id)kCVPixelBufferIOSurfacePropertiesKey : @{},
  };
  CVPixelBufferRef pixel_buffer = nullptr;
  const CVReturn created = CVPixelBufferCreate(
      kCFAllocatorDefault, static_cast<size_t>(kWidth),
      static_cast<size_t>(kHeight),
      kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange,
      (__bridge CFDictionaryRef)attrs, &pixel_buffer);
  if (created != kCVReturnSuccess || pixel_buffer == nullptr) {
    error = "CVPixelBufferCreate failed for PNG preview";
    return false;
  }
  CVPixelBufferLockBaseAddress(pixel_buffer, 0);
  auto* y_dst = static_cast<std::uint8_t*>(
      CVPixelBufferGetBaseAddressOfPlane(pixel_buffer, 0));
  auto* uv_dst = static_cast<std::uint8_t*>(
      CVPixelBufferGetBaseAddressOfPlane(pixel_buffer, 1));
  const size_t y_stride = CVPixelBufferGetBytesPerRowOfPlane(pixel_buffer, 0);
  const size_t uv_stride = CVPixelBufferGetBytesPerRowOfPlane(pixel_buffer, 1);
  for (int row = 0; row < kHeight; ++row) {
    std::memcpy(y_dst + static_cast<size_t>(row) * y_stride,
                nv12 + static_cast<size_t>(row) * kWidth,
                static_cast<size_t>(kWidth));
  }
  const std::uint8_t* uv_src = nv12 + y_bytes;
  const int chroma_h = kHeight / 2;
  for (int row = 0; row < chroma_h; ++row) {
    std::memcpy(uv_dst + static_cast<size_t>(row) * uv_stride,
                uv_src + static_cast<size_t>(row) * kWidth,
                static_cast<size_t>(kWidth));
  }
  CVPixelBufferUnlockBaseAddress(pixel_buffer, 0);

  @autoreleasepool {
    CIImage* image = [CIImage imageWithCVPixelBuffer:pixel_buffer];
    if (image == nil) {
      CVPixelBufferRelease(pixel_buffer);
      error = "CIImage imageWithCVPixelBuffer failed";
      return false;
    }
    CIContext* context = [CIContext contextWithOptions:nil];
    CGImageRef cg_image = [context createCGImage:image fromRect:image.extent];
    CVPixelBufferRelease(pixel_buffer);
    if (cg_image == nullptr) {
      error = "CIContext createCGImage failed";
      return false;
    }
    std::error_code ec;
    std::filesystem::create_directories(png_path.parent_path(), ec);
    if (ec) {
      CGImageRelease(cg_image);
      error = "failed to create " + png_path.parent_path().string() + ": " +
              ec.message();
      return false;
    }
    NSURL* url = [NSURL
        fileURLWithPath:[NSString stringWithUTF8String:png_path.c_str()]];
    CGImageDestinationRef dest = CGImageDestinationCreateWithURL(
        (__bridge CFURLRef)url, (__bridge CFStringRef)UTTypePNG.identifier, 1,
        nullptr);
    if (dest == nullptr) {
      CGImageRelease(cg_image);
      error = "CGImageDestinationCreateWithURL failed";
      return false;
    }
    CGImageDestinationAddImage(dest, cg_image, nullptr);
    const bool ok = CGImageDestinationFinalize(dest);
    CFRelease(dest);
    CGImageRelease(cg_image);
    if (!ok) {
      error = "CGImageDestinationFinalize failed";
      return false;
    }
  }
  error.clear();
  return true;
}

bool publish_frames(const std::filesystem::path& root,
                    const std::vector<Frame>& frames, std::string& error) {
  namespace fs = std::filesystem;
  const fs::path staging = root / "out/video/decode.staging";
  const fs::path publish = root / "out/video/decode";
  const fs::path aside = root / "out/video/decode.aside";
  std::error_code ec;
  fs::remove_all(staging, ec);
  fs::create_directories(staging / "frames", ec);
  if (ec) {
    error = "failed to create decode staging: " + ec.message();
    return false;
  }

  const std::int64_t first_pts = frames.front().pts_ns;
  std::ostringstream csv;
  csv << "frame_index,file,width,height,pixel_format,pts_ns,relative_pts_ns,"
         "bytes\n";
  for (std::size_t i = 0; i < frames.size(); ++i) {
    char name[64];
    std::snprintf(name, sizeof(name), "frame-%06zu.nv12", i);
    const fs::path frame_path = staging / "frames" / name;
    std::ofstream out(frame_path, std::ios::binary);
    if (!out) {
      error = "failed to open " + frame_path.string();
      fs::remove_all(staging, ec);
      return false;
    }
    out.write(reinterpret_cast<const char*>(frames[i].nv12.data()),
              static_cast<std::streamsize>(frames[i].nv12.size()));
    if (!out.good()) {
      error = "failed while writing " + frame_path.string();
      fs::remove_all(staging, ec);
      return false;
    }
    csv << i << ",frames/" << name << ',' << kWidth << ',' << kHeight << ','
        << kPixelFormatToken << ',' << frames[i].pts_ns << ','
        << (frames[i].pts_ns - first_pts) << ',' << frames[i].nv12.size()
        << '\n';
  }
  {
    std::ofstream out(staging / "manifest.csv");
    if (!out) {
      error = "failed to open decode manifest";
      fs::remove_all(staging, ec);
      return false;
    }
    out << csv.str();
    if (!out.good()) {
      error = "failed while writing decode manifest";
      fs::remove_all(staging, ec);
      return false;
    }
  }
  if (!write_preview_png_from_nv12(staging / "preview.png",
                                   frames.front().nv12.data(),
                                   frames.front().nv12.size(), error)) {
    fs::remove_all(staging, ec);
    return false;
  }

  fs::remove_all(aside, ec);
  const bool had_publish = fs::exists(publish);
  if (had_publish) {
    fs::rename(publish, aside, ec);
    if (ec) {
      error = "failed to move aside prior decode dir: " + ec.message();
      fs::remove_all(staging, ec);
      return false;
    }
  }
  fs::create_directories(publish.parent_path(), ec);
  fs::rename(staging, publish, ec);
  if (ec) {
    error = "failed to publish decode dir: " + ec.message();
    if (had_publish) {
      std::error_code restore_ec;
      fs::rename(aside, publish, restore_ec);
    }
    fs::remove_all(staging, ec);
    return false;
  }
  fs::remove_all(aside, ec);
  error.clear();
  return true;
}

CFDictionaryRef make_destination_attrs() {
  const int32_t width = kWidth;
  const int32_t height = kHeight;
  const int32_t pixel = kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange;
  CfRelease width_num(
      CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &width));
  CfRelease height_num(
      CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &height));
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

bool nal_type_present(const std::vector<learning::video::encode::ParameterSet>& sets,
                      int type) {
  for (const auto& set : sets) {
    if (!set.nal.empty() && (set.nal[0] & 0x1f) == type) {
      return true;
    }
  }
  return false;
}

bool decode_one(VTDecompressionSessionRef session, CMFormatDescriptionRef format,
                const learning::video::encode::AccessUnit& unit,
                std::int64_t sample_index, DecodeSlot& slot, std::string& error) {
  {
    std::lock_guard<std::mutex> lock(slot.mu);
    slot.got = false;
    slot.error.clear();
    slot.nv12.clear();
  }

  CMBlockBufferRef block = nullptr;
  const OSStatus block_status = CMBlockBufferCreateWithMemoryBlock(
      kCFAllocatorDefault, const_cast<std::uint8_t*>(unit.payload.data()),
      unit.payload.size(), kCFAllocatorNull, nullptr, 0, unit.payload.size(), 0,
      &block);
  if (block_status != noErr || block == nullptr) {
    error = osstatus("CMBlockBufferCreateWithMemoryBlock", block_status);
    return false;
  }
  CfRelease block_release(block);

  CMSampleTimingInfo timing{};
  timing.duration = kCMTimeInvalid;
  // Sample clock is the access-unit index. The published pts_ns stays the
  // capture timestamp stored in the VAU.
  timing.presentationTimeStamp = CMTimeMake(sample_index, 30);
  timing.decodeTimeStamp = kCMTimeInvalid;
  const size_t sample_size = unit.payload.size();
  CMSampleBufferRef sample = nullptr;
  const OSStatus sample_status = CMSampleBufferCreateReady(
      kCFAllocatorDefault, block, format, 1, 1, &timing, 1, &sample_size,
      &sample);
  if (sample_status != noErr || sample == nullptr) {
    error = osstatus("CMSampleBufferCreateReady", sample_status);
    return false;
  }
  CfRelease sample_release(sample);

  VTDecodeInfoFlags info = 0;
  const OSStatus decoded = VTDecompressionSessionDecodeFrame(
      session, sample, 0, nullptr, &info);
  if (decoded != noErr) {
    error = osstatus("VTDecompressionSessionDecodeFrame", decoded);
    return false;
  }
  if ((info & kVTDecodeInfo_FrameDropped) != 0) {
    error = "decoder dropped a frame";
    return false;
  }
  const OSStatus waited = VTDecompressionSessionWaitForAsynchronousFrames(session);
  if (waited != noErr) {
    error = osstatus("VTDecompressionSessionWaitForAsynchronousFrames", waited);
    return false;
  }
  std::lock_guard<std::mutex> lock(slot.mu);
  if (!slot.got) {
    error = "decoder produced no frame";
    return false;
  }
  if (!slot.error.empty()) {
    error = slot.error;
    return false;
  }
  if (slot.nv12.size() != kFrameBytes) {
    error = "decoded frame is not 1382400 bytes";
    return false;
  }
  error.clear();
  return true;
}

bool decode_vau_to_nv12(const std::filesystem::path& root,
                        const std::filesystem::path& vau_path,
                        DecodeStats& stats, std::string& error) {
  stats = {};
  stats.width = kWidth;
  stats.height = kHeight;
  stats.frame_bytes = static_cast<std::uint32_t>(kFrameBytes);

  learning::video::encode::VauFile file;
  if (!learning::video::encode::read_vau(vau_path, file, error)) {
    return false;
  }
  if (!nal_type_present(file.parameter_sets, 7) ||
      !nal_type_present(file.parameter_sets, 8)) {
    error = "VAU parameter sets are missing SPS or PPS";
    return false;
  }

  std::vector<const uint8_t*> pointers(file.parameter_sets.size());
  std::vector<size_t> sizes(file.parameter_sets.size());
  for (std::size_t i = 0; i < file.parameter_sets.size(); ++i) {
    pointers[i] = file.parameter_sets[i].nal.data();
    sizes[i] = file.parameter_sets[i].nal.size();
  }
  CMFormatDescriptionRef format = nullptr;
  const OSStatus format_status = CMVideoFormatDescriptionCreateFromH264ParameterSets(
      kCFAllocatorDefault, file.parameter_sets.size(), pointers.data(),
      sizes.data(), 4, &format);
  if (format_status != noErr || format == nullptr) {
    error = osstatus("CMVideoFormatDescriptionCreateFromH264ParameterSets",
                     format_status);
    return false;
  }
  CfRelease format_release(format);

  const void* spec_keys[] = {
      kVTVideoDecoderSpecification_EnableHardwareAcceleratedVideoDecoder};
  const void* spec_values[] = {kCFBooleanFalse};
  CfRelease decoder_spec(CFDictionaryCreate(
      kCFAllocatorDefault, spec_keys, spec_values, 1,
      &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
  CfRelease dest_attrs(make_destination_attrs());

  DecodeSlot slot;
  VTDecompressionOutputCallbackRecord callback{};
  callback.decompressionOutputCallback = learning_video_decode_output;
  callback.decompressionOutputRefCon = &slot;

  VTDecompressionSessionRef session = nullptr;
  const OSStatus created = VTDecompressionSessionCreate(
      kCFAllocatorDefault, format,
      static_cast<CFDictionaryRef>(decoder_spec.ref),
      static_cast<CFDictionaryRef>(dest_attrs.ref), &callback, &session);
  if (created != noErr || session == nullptr) {
    error = osstatus("VTDecompressionSessionCreate", created);
    return false;
  }
  CfRelease session_release(session);

  std::vector<Frame> frames;
  frames.reserve(file.access_units.size());
  for (std::size_t i = 0; i < file.access_units.size(); ++i) {
    const auto& unit = file.access_units[i];
    if (!decode_one(session, format, unit, static_cast<std::int64_t>(i), slot,
                    error)) {
      return false;
    }
    Frame frame;
    {
      std::lock_guard<std::mutex> lock(slot.mu);
      frame.nv12 = slot.nv12;
    }
    frame.pts_ns = unit.pts_ns;
    frames.push_back(std::move(frame));
  }
  VTDecompressionSessionInvalidate(session);
  if (!publish_frames(root, frames, error)) {
    return false;
  }
  stats.frames_decoded = static_cast<std::uint32_t>(frames.size());
  error.clear();
  return true;
}

}  // namespace learning::video::decode
