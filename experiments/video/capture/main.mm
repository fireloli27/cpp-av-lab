#import "capture_publish.hpp"

#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace cap = learning::video::capture;

namespace {

constexpr auto kCaptureTimeout = std::chrono::seconds(15);

struct FrameSink {
  std::mutex mu;
  std::condition_variable cv;
  std::vector<cap::CapturedFrame> frames;
  std::size_t target = 0;
  std::atomic<bool> failed{false};
  std::string error;
  bool done = false;
};

bool cm_time_to_ns(CMTime t, std::int64_t& out_ns, std::string& error) {
  if (!CMTIME_IS_VALID(t) || CMTIME_IS_INDEFINITE(t)) {
    error = "invalid or indefinite Presentation Timestamp";
    return false;
  }
  const double seconds = CMTimeGetSeconds(t);
  if (seconds < 0 || !std::isfinite(seconds)) {
    error = "Presentation Timestamp is not a finite non-negative time";
    return false;
  }
  out_ns = static_cast<std::int64_t>(seconds * 1e9);
  return true;
}

std::string fourcc_token(OSType fmt) {
  char fourcc[5] = {static_cast<char>((fmt >> 24) & 0xff),
                    static_cast<char>((fmt >> 16) & 0xff),
                    static_cast<char>((fmt >> 8) & 0xff),
                    static_cast<char>(fmt & 0xff), 0};
  return fourcc;
}

std::string available_pixel_formats(AVCaptureVideoDataOutput* output) {
  if (output == nil) {
    return "(no output)";
  }
  NSArray* formats = output.availableVideoCVPixelFormatTypes;
  if (formats == nil || formats.count == 0) {
    return "(none reported)";
  }
  std::string list;
  for (NSNumber* n in formats) {
    if (!list.empty()) {
      list += ',';
    }
    list += fourcc_token(static_cast<OSType>(n.unsignedIntValue));
  }
  return list;
}

bool copy_tight_nv12(CVPixelBufferRef pixel_buffer, int expect_w, int expect_h,
                     AVCaptureVideoDataOutput* output,
                     std::vector<std::uint8_t>& out, std::string& error) {
  const OSType fmt = CVPixelBufferGetPixelFormatType(pixel_buffer);
  if (fmt != kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange) {
    error = "unexpected pixel format " + fourcc_token(fmt) +
            " (need 420v); available videoSettings pixel formats: " +
            available_pixel_formats(output);
    return false;
  }

  const size_t width = CVPixelBufferGetWidth(pixel_buffer);
  const size_t height = CVPixelBufferGetHeight(pixel_buffer);
  if (static_cast<int>(width) != expect_w ||
      static_cast<int>(height) != expect_h) {
    error = "unexpected frame size " + std::to_string(width) + "x" +
            std::to_string(height) + " (need " + std::to_string(expect_w) +
            "x" + std::to_string(expect_h) +
            "); available videoSettings pixel formats: " +
            available_pixel_formats(output);
    return false;
  }

  CVPixelBufferLockBaseAddress(pixel_buffer, kCVPixelBufferLock_ReadOnly);
  const uint8_t* y_src = static_cast<const uint8_t*>(
      CVPixelBufferGetBaseAddressOfPlane(pixel_buffer, 0));
  const uint8_t* uv_src = static_cast<const uint8_t*>(
      CVPixelBufferGetBaseAddressOfPlane(pixel_buffer, 1));
  const size_t y_stride = CVPixelBufferGetBytesPerRowOfPlane(pixel_buffer, 0);
  const size_t uv_stride = CVPixelBufferGetBytesPerRowOfPlane(pixel_buffer, 1);
  const bool ok = cap::pack_nv12_planes_to_tight(
      y_src, y_stride, uv_src, uv_stride, expect_w, expect_h, out, error);
  CVPixelBufferUnlockBaseAddress(pixel_buffer, kCVPixelBufferLock_ReadOnly);
  return ok;
}

bool parse_frames(int argc, char** argv, int& frame_count, std::string& error) {
  frame_count = cap::kDefaultFrameCount;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--frames") {
      if (i + 1 >= argc) {
        error = "--frames requires a value";
        return false;
      }
      char* end = nullptr;
      const long v = std::strtol(argv[++i], &end, 10);
      if (end == argv[i] || *end != '\0') {
        error = "--frames must be an integer";
        return false;
      }
      if (v < cap::kMinFrameCount || v > cap::kMaxFrameCount) {
        error = "--frames must be in [1, 90]";
        return false;
      }
      frame_count = static_cast<int>(v);
    } else {
      error = "unknown argument: " + arg;
      return false;
    }
  }
  return true;
}

bool ensure_camera_permission(std::string& error) {
  const AVAuthorizationStatus status =
      [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
  if (status == AVAuthorizationStatusAuthorized) {
    return true;
  }
  if (status == AVAuthorizationStatusDenied ||
      status == AVAuthorizationStatusRestricted) {
    error = "camera permission denied or restricted";
    return false;
  }

  // NotDetermined: request access (blocking).
  dispatch_semaphore_t sema = dispatch_semaphore_create(0);
  __block BOOL granted = NO;
  [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo
                           completionHandler:^(BOOL g) {
                             granted = g;
                             dispatch_semaphore_signal(sema);
                           }];
  dispatch_semaphore_wait(sema, DISPATCH_TIME_FOREVER);
  if (!granted) {
    error = "camera permission not granted";
    return false;
  }
  return true;
}

}  // namespace

@interface CaptureDelegate : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
@property(nonatomic, assign) FrameSink* sink;
@property(nonatomic, weak) AVCaptureVideoDataOutput* videoOutput;
@end

@implementation CaptureDelegate
- (void)captureOutput:(AVCaptureOutput*)output
    didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
           fromConnection:(AVCaptureConnection*)connection {
  (void)output;
  (void)connection;
  FrameSink* sink = self.sink;
  if (sink == nullptr || sink->failed.load() || sink->done) {
    return;
  }

  CVPixelBufferRef pixel_buffer = CMSampleBufferGetImageBuffer(sampleBuffer);
  if (pixel_buffer == nullptr) {
    std::lock_guard<std::mutex> lock(sink->mu);
    sink->failed = true;
    sink->error = "CMSampleBuffer missing image buffer";
    sink->done = true;
    sink->cv.notify_all();
    return;
  }

  cap::CapturedFrame frame;
  std::string copy_error;
  if (!copy_tight_nv12(pixel_buffer, cap::kWidth, cap::kHeight, self.videoOutput,
                       frame.nv12, copy_error)) {
    std::lock_guard<std::mutex> lock(sink->mu);
    sink->failed = true;
    sink->error = copy_error;
    sink->done = true;
    sink->cv.notify_all();
    return;
  }
  if (!cm_time_to_ns(CMSampleBufferGetPresentationTimeStamp(sampleBuffer),
                     frame.pts_ns, copy_error)) {
    std::lock_guard<std::mutex> lock(sink->mu);
    sink->failed = true;
    sink->error = copy_error;
    sink->done = true;
    sink->cv.notify_all();
    return;
  }

  {
    std::lock_guard<std::mutex> lock(sink->mu);
    if (sink->done || sink->failed.load()) {
      return;
    }
    sink->frames.push_back(std::move(frame));
    if (sink->frames.size() >= sink->target) {
      sink->done = true;
      sink->cv.notify_all();
    }
  }
}
@end

int main(int argc, char** argv) {
  int frame_count = cap::kDefaultFrameCount;
  std::string error;
  if (!parse_frames(argc, argv, frame_count, error)) {
    std::cerr << error << '\n';
    return 1;
  }

  @autoreleasepool {
    if (!ensure_camera_permission(error)) {
      std::cerr << error << '\n';
      return 1;
    }

    AVCaptureDevice* device =
        [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
    if (device == nil) {
      std::cerr << "no default video capture device\n";
      return 1;
    }

    NSError* ns_error = nil;
    AVCaptureDeviceInput* input =
        [AVCaptureDeviceInput deviceInputWithDevice:device error:&ns_error];
    if (input == nil) {
      std::cerr << "AVCaptureDeviceInput failed: "
                << (ns_error ? ns_error.localizedDescription.UTF8String
                             : "unknown")
                << '\n';
      return 1;
    }

    AVCaptureSession* session = [[AVCaptureSession alloc] init];
    [session beginConfiguration];
    if (![session canSetSessionPreset:AVCaptureSessionPreset1280x720]) {
      std::cerr << "device cannot use AVCaptureSessionPreset1280x720\n";
      return 1;
    }
    session.sessionPreset = AVCaptureSessionPreset1280x720;

    if (![session canAddInput:input]) {
      std::cerr << "cannot add video input to session\n";
      return 1;
    }
    [session addInput:input];

    AVCaptureDeviceFormat* chosen = nil;
    for (AVCaptureDeviceFormat* format in device.formats) {
      const CMVideoDimensions dims =
          CMVideoFormatDescriptionGetDimensions(format.formatDescription);
      if (dims.width != cap::kWidth || dims.height != cap::kHeight) {
        continue;
      }
      BOOL supports_30 = NO;
      for (AVFrameRateRange* range in format.videoSupportedFrameRateRanges) {
        if (range.minFrameRate <= 30.0 + 1e-6 &&
            range.maxFrameRate + 1e-6 >= 30.0) {
          supports_30 = YES;
          break;
        }
      }
      if (supports_30) {
        chosen = format;
        break;
      }
    }
    if (chosen == nil) {
      std::cerr << "no activeFormat supports 1280x720 at 30 fps\n";
      return 1;
    }

    auto pin_720p_30 = [&](std::string& pin_error) -> bool {
      NSError* lock_error = nil;
      if (![device lockForConfiguration:&lock_error]) {
        pin_error = std::string("lockForConfiguration failed: ") +
                    (lock_error ? lock_error.localizedDescription.UTF8String
                                : "unknown");
        return false;
      }
      device.activeFormat = chosen;
      device.activeVideoMinFrameDuration = CMTimeMake(1, 30);
      device.activeVideoMaxFrameDuration = CMTimeMake(1, 30);
      [device unlockForConfiguration];
      return true;
    };

    if (!pin_720p_30(error)) {
      std::cerr << error << '\n';
      return 1;
    }

    AVCaptureVideoDataOutput* output = [[AVCaptureVideoDataOutput alloc] init];
    output.alwaysDiscardsLateVideoFrames = YES;
    NSDictionary* settings = @{
      (id)kCVPixelBufferPixelFormatTypeKey :
          @(kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange)
    };
    output.videoSettings = settings;

    dispatch_queue_t queue =
        dispatch_queue_create("learning.video.capture", DISPATCH_QUEUE_SERIAL);
    FrameSink sink;
    sink.target = static_cast<std::size_t>(frame_count);
    CaptureDelegate* delegate = [[CaptureDelegate alloc] init];
    delegate.sink = &sink;
    delegate.videoOutput = output;
    // Attach delegate only after startRunning + re-pin (macOS may emit 1080p
    // frames between those two steps).

    if (![session canAddOutput:output]) {
      std::cerr << "cannot add video data output to session\n";
      return 1;
    }
    [session addOutput:output];

    AVCaptureConnection* connection =
        [output connectionWithMediaType:AVMediaTypeVideo];
    if (connection != nil) {
      if (connection.isVideoMirroringSupported) {
        connection.automaticallyAdjustsVideoMirroring = NO;
        connection.videoMirrored = NO;
      }
      // Explicitly pin identity rotation when the API exists (no auto-rotate).
      if ([connection respondsToSelector:@selector(isVideoRotationAngleSupported:)] &&
          [connection respondsToSelector:@selector(setVideoRotationAngle:)] &&
          [connection isVideoRotationAngleSupported:0.0]) {
        connection.videoRotationAngle = 0.0;
      }
    }
    [session commitConfiguration];

    [session startRunning];
    // macOS resets activeFormat on startRunning; pin 720p/30 again.
    if (!pin_720p_30(error)) {
      [session stopRunning];
      std::cerr << error << '\n';
      return 1;
    }
    [output setSampleBufferDelegate:delegate queue:queue];

    const auto deadline = std::chrono::steady_clock::now() + kCaptureTimeout;
    {
      std::unique_lock<std::mutex> lock(sink.mu);
      const bool completed = sink.cv.wait_until(lock, deadline, [&] {
        return sink.done || sink.failed.load();
      });
      if (!completed) {
        sink.failed = true;
        sink.error = "capture timed out after 15s before reaching frame count";
        sink.done = true;
      }
    }
    [session stopRunning];
    // Drain: clear delegate so no late callbacks touch sink after we leave.
    [output setSampleBufferDelegate:nil queue:nil];
    // Brief pause so in-flight callbacks finish before sink goes out of scope.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    if (sink.failed.load()) {
      std::cerr << sink.error << '\n';
      return 1;
    }
    if (sink.frames.size() != static_cast<std::size_t>(frame_count)) {
      std::cerr << "captured " << sink.frames.size() << " frames, need "
                << frame_count << '\n';
      return 1;
    }

    if (!cap::publish_capture_artifacts(".", sink.frames, cap::kWidth,
                                        cap::kHeight, error)) {
      std::cerr << error << '\n';
      return 1;
    }

    const std::size_t frame_bytes = cap::nv12_tight_bytes(cap::kWidth, cap::kHeight);
    std::cout << "frames_captured=" << sink.frames.size()
              << " width=" << cap::kWidth << " height=" << cap::kHeight
              << " pixel_format=" << cap::kPixelFormatToken
              << " frame_bytes=" << frame_bytes
              << " manifest=" << cap::kPublishRelativeDir << "/manifest.csv"
              << " preview=" << cap::kPublishRelativeDir << "/preview.png"
              << '\n';
  }
  return 0;
}
