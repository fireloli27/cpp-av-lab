#include "render_frames.hpp"

#import <AppKit/AppKit.h>
#import <CoreImage/CoreImage.h>
#import <CoreVideo/CoreVideo.h>

#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

namespace rend = learning::video::render;
constexpr double kHoldLastFrameSeconds = 0.5;
constexpr double kWindowWidthPt = 960;
constexpr double kWindowHeightPt = 540;

bool parse_args(int argc, char** argv, std::filesystem::path& input,
                std::string& error) {
  input = rend::kDefaultInputRelativePath;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--input") {
      if (i + 1 >= argc) {
        error = "--input requires a path";
        return false;
      }
      input = argv[++i];
    } else {
      error = "unknown argument: " + arg;
      return false;
    }
  }
  return true;
}

NSImage* image_from_nv12(const std::vector<std::uint8_t>& nv12) {
  if (nv12.size() != rend::kFrameBytes) {
    return nil;
  }
  NSDictionary* attrs = @{
    (id)kCVPixelBufferIOSurfacePropertiesKey : @{},
  };
  CVPixelBufferRef pixel_buffer = nullptr;
  const CVReturn created = CVPixelBufferCreate(
      kCFAllocatorDefault, static_cast<size_t>(rend::kWidth),
      static_cast<size_t>(rend::kHeight),
      kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange,
      (__bridge CFDictionaryRef)attrs, &pixel_buffer);
  if (created != kCVReturnSuccess || pixel_buffer == nullptr) {
    return nil;
  }
  CVPixelBufferLockBaseAddress(pixel_buffer, 0);
  auto* y_dst = static_cast<std::uint8_t*>(
      CVPixelBufferGetBaseAddressOfPlane(pixel_buffer, 0));
  auto* uv_dst = static_cast<std::uint8_t*>(
      CVPixelBufferGetBaseAddressOfPlane(pixel_buffer, 1));
  const size_t y_stride = CVPixelBufferGetBytesPerRowOfPlane(pixel_buffer, 0);
  const size_t uv_stride = CVPixelBufferGetBytesPerRowOfPlane(pixel_buffer, 1);
  if (y_dst == nullptr || uv_dst == nullptr) {
    CVPixelBufferUnlockBaseAddress(pixel_buffer, 0);
    CVPixelBufferRelease(pixel_buffer);
    return nil;
  }
  for (int row = 0; row < rend::kHeight; ++row) {
    std::memcpy(y_dst + static_cast<size_t>(row) * y_stride,
                nv12.data() + static_cast<size_t>(row) * rend::kWidth,
                static_cast<size_t>(rend::kWidth));
  }
  const std::uint8_t* uv_src =
      nv12.data() + static_cast<std::size_t>(rend::kWidth) * rend::kHeight;
  const int chroma_h = rend::kHeight / 2;
  for (int row = 0; row < chroma_h; ++row) {
    std::memcpy(uv_dst + static_cast<size_t>(row) * uv_stride,
                uv_src + static_cast<size_t>(row) * rend::kWidth,
                static_cast<size_t>(rend::kWidth));
  }
  CVPixelBufferUnlockBaseAddress(pixel_buffer, 0);

  CIImage* ci = [CIImage imageWithCVPixelBuffer:pixel_buffer];
  CIContext* context = [CIContext contextWithOptions:nil];
  CGImageRef cg = [context createCGImage:ci fromRect:ci.extent];
  CVPixelBufferRelease(pixel_buffer);
  if (cg == nullptr) {
    return nil;
  }
  NSImage* image = [[NSImage alloc]
      initWithCGImage:cg
                 size:NSMakeSize(rend::kWidth, rend::kHeight)];
  CGImageRelease(cg);
  return image;
}

}  // namespace

int main(int argc, char** argv) {
  @autoreleasepool {
    std::filesystem::path input;
    std::string error;
    if (!parse_args(argc, argv, input, error)) {
      std::cerr << error << '\n';
      return 1;
    }

    std::vector<rend::RenderFrame> frames;
    if (!rend::load_frame_directory(input, frames, error)) {
      std::cerr << error << '\n';
      return 1;
    }
    std::vector<std::int64_t> relative;
    relative.reserve(frames.size());
    for (const auto& frame : frames) {
      relative.push_back(frame.relative_pts_ns);
    }
    std::vector<std::int64_t> delays;
    if (!rend::presentation_delays_ns(relative, delays, error)) {
      std::cerr << error << '\n';
      return 1;
    }

    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    const NSRect rect = NSMakeRect(0, 0, kWindowWidthPt, kWindowHeightPt);
    NSWindow* window = [[NSWindow alloc]
        initWithContentRect:rect
                  styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                             NSWindowStyleMaskMiniaturizable)
                    backing:NSBackingStoreBuffered
                      defer:NO];
    if (window == nil) {
      std::cerr << "NSWindow init failed\n";
      return 1;
    }
    [window setTitle:@"learning_video_render"];
    NSImageView* view = [[NSImageView alloc] initWithFrame:rect];
    [view setImageScaling:NSImageScaleProportionallyUpOrDown];
    [view setImageAlignment:NSImageAlignCenter];
    [view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [window setContentView:view];
    [window center];
    [window makeKeyAndOrderFront:nil];
    if (@available(macOS 14.0, *)) {
      [NSApp activate];
    } else {
      [NSApp activateIgnoringOtherApps:YES];
    }

    std::uint64_t presented = 0;
    for (std::size_t i = 0; i < frames.size(); ++i) {
      if (delays[i] > 0) {
        const double seconds =
            static_cast<double>(delays[i]) / 1000000000.0;
        [[NSRunLoop currentRunLoop]
            runUntilDate:[NSDate dateWithTimeIntervalSinceNow:seconds]];
      }
      NSImage* image = image_from_nv12(frames[i].nv12);
      if (image == nil) {
        std::cerr << "failed to build an image for frame " << i << '\n';
        return 1;
      }
      [view setImage:image];
      [view setNeedsDisplay:YES];
      [window displayIfNeeded];
      ++presented;
    }
    [[NSRunLoop currentRunLoop]
        runUntilDate:[NSDate dateWithTimeIntervalSinceNow:kHoldLastFrameSeconds]];

    if (presented != frames.size()) {
      std::cerr << "frames_presented does not match frames_loaded\n";
      return 1;
    }

    std::cout << "rendered"
              << " input=" << input.string()
              << " frames_loaded=" << frames.size()
              << " frames_presented=" << presented
              << " width=" << rend::kWidth
              << " height=" << rend::kHeight
              << " pixel_format=" << rend::kPixelFormatToken << '\n';
    return 0;
  }
}
