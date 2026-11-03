#include "capture_publish.hpp"

#include <cstring>

namespace learning::video::capture {

std::size_t nv12_tight_bytes(int width, int height) {
  if (width <= 0 || height <= 0) {
    return 0;
  }
  const int chroma_w = (width + 1) / 2;
  const int chroma_h = (height + 1) / 2;
  const std::size_t y = static_cast<std::size_t>(width) *
                        static_cast<std::size_t>(height);
  const std::size_t uv = 2u * static_cast<std::size_t>(chroma_w) *
                         static_cast<std::size_t>(chroma_h);
  return y + uv;
}

bool pack_nv12_planes_to_tight(const std::uint8_t* y_plane, std::size_t y_stride,
                               const std::uint8_t* uv_plane,
                               std::size_t uv_stride, int width, int height,
                               std::vector<std::uint8_t>& out,
                               std::string& error) {
  if (y_plane == nullptr || uv_plane == nullptr || width <= 0 || height <= 0) {
    error = "invalid NV12 plane pointers or dimensions";
    return false;
  }
  const std::size_t y_row = static_cast<std::size_t>(width);
  const std::size_t uv_row =
      2u * static_cast<std::size_t>((width + 1) / 2);
  if (y_stride < y_row || uv_stride < uv_row) {
    error = "NV12 stride smaller than tight row width";
    return false;
  }

  const std::size_t expected = nv12_tight_bytes(width, height);
  out.resize(expected);
  for (int row = 0; row < height; ++row) {
    std::memcpy(out.data() + static_cast<std::size_t>(row) * y_row,
                y_plane + static_cast<std::size_t>(row) * y_stride, y_row);
  }
  const int chroma_h = (height + 1) / 2;
  std::uint8_t* uv_dst = out.data() + y_row * static_cast<std::size_t>(height);
  for (int row = 0; row < chroma_h; ++row) {
    std::memcpy(uv_dst + static_cast<std::size_t>(row) * uv_row,
                uv_plane + static_cast<std::size_t>(row) * uv_stride, uv_row);
  }
  error.clear();
  return true;
}

}  // namespace learning::video::capture
