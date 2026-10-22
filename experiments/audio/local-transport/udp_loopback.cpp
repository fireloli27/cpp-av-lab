#include "udp_loopback.hpp"

#include "transport_unit.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <sstream>
#include <vector>

namespace learning::audio::local_transport {
namespace {

constexpr std::size_t kMaxDatagram = 65507;

std::string errno_message(const char* what) {
  std::ostringstream oss;
  oss << what << ": " << std::strerror(errno);
  return oss.str();
}

}  // namespace

bool udp_loopback_roundtrip(
    const std::vector<std::vector<std::uint8_t>>& payloads, LoopbackStats& stats,
    std::string& error) {
  stats = {};

  const int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    error = errno_message("socket(AF_INET, SOCK_DGRAM)");
    return false;
  }

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(0);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
    error = errno_message("bind(127.0.0.1:0)");
    ::close(fd);
    return false;
  }

  socklen_t addr_len = sizeof(addr);
  if (::getsockname(fd, reinterpret_cast<sockaddr*>(&addr), &addr_len) != 0) {
    error = errno_message("getsockname");
    ::close(fd);
    return false;
  }
  stats.bound_port = static_cast<int>(ntohs(addr.sin_port));

  std::vector<std::uint8_t> recv_buf(kMaxDatagram);

  for (const auto& payload : payloads) {
    const auto unit = pack_transport_unit(payload);
    if (unit.size() > kMaxDatagram) {
      std::ostringstream oss;
      oss << "transport unit too large for UDP datagram: " << unit.size();
      error = oss.str();
      ::close(fd);
      return false;
    }

    const ssize_t sent = ::sendto(
        fd, unit.data(), unit.size(), 0, reinterpret_cast<sockaddr*>(&addr),
        sizeof(addr));
    if (sent < 0 || static_cast<std::size_t>(sent) != unit.size()) {
      error = errno_message("sendto");
      ::close(fd);
      return false;
    }
    ++stats.units_sent;
    stats.payload_bytes_sent += payload.size();

    sockaddr_in from{};
    socklen_t from_len = sizeof(from);
    const ssize_t got =
        ::recvfrom(fd, recv_buf.data(), recv_buf.size(), 0,
                   reinterpret_cast<sockaddr*>(&from), &from_len);
    if (got < 0) {
      error = errno_message("recvfrom");
      ::close(fd);
      return false;
    }

    std::vector<std::uint8_t> unit_recv(recv_buf.begin(),
                                        recv_buf.begin() + got);
    std::vector<std::uint8_t> payload_recv;
    if (!unpack_transport_unit(unit_recv, payload_recv, error)) {
      ::close(fd);
      return false;
    }
    if (payload_recv != payload) {
      error = "loopback payload mismatch";
      ::close(fd);
      return false;
    }
    ++stats.units_recv;
    stats.payload_bytes_recv += payload_recv.size();
  }

  ::close(fd);
  error.clear();
  return true;
}

}  // namespace learning::audio::local_transport
