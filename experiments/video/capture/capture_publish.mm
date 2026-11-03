#import "capture_publish.hpp"

#import <CoreGraphics/CoreGraphics.h>
#import <CoreImage/CoreImage.h>
#import <CoreVideo/CoreVideo.h>
#import <ImageIO/ImageIO.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

namespace learning::video::capture {
namespace {

void cleanup_staging(const std::filesystem::path& staging) {
  std::error_code ec;
  std::filesystem::remove_all(staging, ec);
}

bool write_bytes(const std::filesystem::path& path,
                 const std::uint8_t* data, std::size_t nbytes,
                 std::string& error) {
  std::ofstream out(path, std::ios::binary);
  if (!out) {
    error = "failed to open " + path.string();
    return false;
  }
  if (nbytes > 0) {
    out.write(reinterpret_cast<const char*>(data),
              static_cast<std::streamsize>(nbytes));
  }
  if (!out.good()) {
    error = "failed while writing " + path.string();
    return false;
  }
  out.close();
  if (out.fail()) {
    error = "failed while closing " + path.string();
    return false;
  }
  return true;
}

std::string frame_file_name(std::size_t index) {
  char buf[64];
  std::snprintf(buf, sizeof(buf), "frame-%06zu.nv12", index);
  return buf;
}

}  // namespace

bool write_preview_png_from_nv12(const std::filesystem::path& png_path,
                                 const std::uint8_t* nv12, std::size_t nbytes,
                                 int width, int height, std::string& error) {
  if (nv12 == nullptr || width <= 0 || height <= 0) {
    error = "invalid NV12 for PNG preview";
    return false;
  }
  if (nbytes != nv12_tight_bytes(width, height)) {
    error = "NV12 byte count does not match tight layout for PNG preview";
    return false;
  }

  const std::size_t y_bytes =
      static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
  const int chroma_w = (width + 1) / 2;
  const size_t y_row = static_cast<size_t>(width);
  const size_t uv_row = static_cast<size_t>(chroma_w) * 2u;

  NSDictionary* attrs = @{
    (id)kCVPixelBufferIOSurfacePropertiesKey : @{},
  };
  CVPixelBufferRef pixel_buffer = nullptr;
  const CVReturn created = CVPixelBufferCreate(
      kCFAllocatorDefault, static_cast<size_t>(width),
      static_cast<size_t>(height), kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange,
      (__bridge CFDictionaryRef)attrs, &pixel_buffer);
  if (created != kCVReturnSuccess || pixel_buffer == nullptr) {
    error = "CVPixelBufferCreate failed for PNG preview";
    return false;
  }

  CVPixelBufferLockBaseAddress(pixel_buffer, 0);
  uint8_t* y_dst = static_cast<uint8_t*>(
      CVPixelBufferGetBaseAddressOfPlane(pixel_buffer, 0));
  uint8_t* uv_dst = static_cast<uint8_t*>(
      CVPixelBufferGetBaseAddressOfPlane(pixel_buffer, 1));
  const size_t y_stride = CVPixelBufferGetBytesPerRowOfPlane(pixel_buffer, 0);
  const size_t uv_stride = CVPixelBufferGetBytesPerRowOfPlane(pixel_buffer, 1);

  for (int row = 0; row < height; ++row) {
    std::memcpy(y_dst + static_cast<size_t>(row) * y_stride,
                nv12 + static_cast<size_t>(row) * y_row, y_row);
  }
  const int chroma_h = (height + 1) / 2;
  const uint8_t* uv_src = nv12 + y_bytes;
  for (int row = 0; row < chroma_h; ++row) {
    std::memcpy(uv_dst + static_cast<size_t>(row) * uv_stride,
                uv_src + static_cast<size_t>(row) * uv_row, uv_row);
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
    CGImageRef cg_image =
        [context createCGImage:image fromRect:image.extent];
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

    NSURL* url =
        [NSURL fileURLWithPath:[NSString stringWithUTF8String:png_path.c_str()]];
    CGImageDestinationRef dest = CGImageDestinationCreateWithURL(
        (__bridge CFURLRef)url, (__bridge CFStringRef)UTTypePNG.identifier, 1,
        nullptr);
    if (dest == nullptr) {
      CGImageRelease(cg_image);
      error = "CGImageDestinationCreateWithURL failed for " + png_path.string();
      return false;
    }
    CGImageDestinationAddImage(dest, cg_image, nullptr);
    const bool ok = CGImageDestinationFinalize(dest);
    CFRelease(dest);
    CGImageRelease(cg_image);
    if (!ok) {
      error = "CGImageDestinationFinalize failed for " + png_path.string();
      return false;
    }
  }

  error.clear();
  return true;
}

bool publish_capture_artifacts(const std::filesystem::path& root,
                               const std::vector<CapturedFrame>& frames,
                               int width, int height, std::string& error) {
  namespace fs = std::filesystem;

  if (frames.empty()) {
    error = "no frames to publish";
    return false;
  }
  if (width <= 0 || height <= 0) {
    error = "invalid frame dimensions";
    return false;
  }

  const std::size_t expected = nv12_tight_bytes(width, height);
  for (std::size_t i = 0; i < frames.size(); ++i) {
    if (frames[i].nv12.size() != expected) {
      error = "frame " + std::to_string(i) + " NV12 size mismatch: got " +
              std::to_string(frames[i].nv12.size()) + " expected " +
              std::to_string(expected);
      return false;
    }
  }

  const fs::path staging = root / kStagingRelativeDir;
  const fs::path publish = root / kPublishRelativeDir;
  cleanup_staging(staging);

  std::error_code ec;
  fs::create_directories(staging / "frames", ec);
  if (ec) {
    error = "failed to create staging frames dir: " + ec.message();
    cleanup_staging(staging);
    return false;
  }

  const std::int64_t first_pts = frames.front().pts_ns;
  std::ostringstream csv;
  csv << "frame_index,file,width,height,pixel_format,pts_ns,relative_pts_ns,"
         "bytes\n";

  for (std::size_t i = 0; i < frames.size(); ++i) {
    const std::string name = frame_file_name(i);
    const fs::path frame_path = staging / "frames" / name;
    if (!write_bytes(frame_path, frames[i].nv12.data(), frames[i].nv12.size(),
                     error)) {
      cleanup_staging(staging);
      return false;
    }
    const std::int64_t rel = relative_pts_ns(frames[i].pts_ns, first_pts);
    csv << i << ",frames/" << name << ',' << width << ',' << height << ','
        << kPixelFormatToken << ',' << frames[i].pts_ns << ',' << rel << ','
        << frames[i].nv12.size() << '\n';
  }

  const fs::path manifest_path = staging / "manifest.csv";
  {
    std::ofstream out(manifest_path);
    if (!out) {
      error = "failed to open " + manifest_path.string();
      cleanup_staging(staging);
      return false;
    }
    out << csv.str();
    if (!out.good()) {
      error = "failed while writing " + manifest_path.string();
      cleanup_staging(staging);
      return false;
    }
  }

  if (!write_preview_png_from_nv12(staging / "preview.png",
                                   frames.front().nv12.data(),
                                   frames.front().nv12.size(), width, height,
                                   error)) {
    cleanup_staging(staging);
    return false;
  }

  // Staging is complete. Replace publish: move aside old, rename staging in,
  // then drop the aside copy. If rename fails after aside, restore prior.
  const fs::path aside = root / "out/video/capture.aside";
  fs::remove_all(aside, ec);
  const bool had_publish = fs::exists(publish);
  if (had_publish) {
    fs::rename(publish, aside, ec);
    if (ec) {
      error = "failed to move aside prior publish: " + ec.message();
      cleanup_staging(staging);
      return false;
    }
  }

  fs::create_directories(publish.parent_path(), ec);
  fs::rename(staging, publish, ec);
  if (ec) {
    error = "failed to publish staging: " + ec.message();
    if (had_publish) {
      std::error_code restore_ec;
      fs::rename(aside, publish, restore_ec);
    }
    cleanup_staging(staging);
    return false;
  }

  fs::remove_all(aside, ec);
  error.clear();
  return true;
}

}  // namespace learning::video::capture
