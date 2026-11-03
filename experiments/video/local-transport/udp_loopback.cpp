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

namespace learning::video::local_transport {
namespace {

std::string errno_message(const char* what) {
  std::ostringstream oss;
  oss << what << ": " << std::strerror(errno);
  return oss.str();
}

struct SocketClose {
  int fd = -1;
  ~SocketClose() {
    if (fd >= 0) {
      ::close(fd);
    }
  }
};

ssize_t recv_retry(int fd, std::uint8_t* buf, std::size_t cap, sockaddr_in& from,
                   socklen_t& from_len) {
  for (;;) {
    const ssize_t got =
        ::recvfrom(fd, buf, cap, 0, reinterpret_cast<sockaddr*>(&from), &from_len);
    if (got >= 0 || errno != EINTR) {
      return got;
    }
  }
}

}  // namespace

bool udp_loopback_roundtrip(
    const std::vector<std::vector<std::uint8_t>>& access_units,
    LoopbackStats& stats, std::string& error) {
  stats = {};
  if (access_units.empty()) {
    error = "no access units to send";
    return false;
  }

  SocketClose sock;
  sock.fd = ::socket(AF_INET, SOCK_DGRAM, 0);
  if (sock.fd < 0) {
    error = errno_message("socket(AF_INET, SOCK_DGRAM)");
    return false;
  }

  timeval timeout{};
  timeout.tv_sec = 2;
  if (::setsockopt(sock.fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) !=
      0) {
    error = errno_message("setsockopt(SO_RCVTIMEO)");
    return false;
  }

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(0);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if (::bind(sock.fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
    error = errno_message("bind(127.0.0.1:0)");
    return false;
  }
  socklen_t addr_len = sizeof(addr);
  if (::getsockname(sock.fd, reinterpret_cast<sockaddr*>(&addr), &addr_len) !=
      0) {
    error = errno_message("getsockname");
    return false;
  }
  stats.bound_port = static_cast<int>(ntohs(addr.sin_port));

  std::vector<std::uint8_t> recv_buf(kDatagramHeaderBytes + kFragPayloadMax);

  for (std::size_t au_index = 0; au_index < access_units.size(); ++au_index) {
    std::vector<std::vector<std::uint8_t>> datagrams;
    if (!split_access_unit(static_cast<std::uint32_t>(au_index),
                           access_units[au_index], datagrams, error)) {
      return false;
    }
    for (const auto& datagram : datagrams) {
      const ssize_t sent = ::sendto(
          sock.fd, datagram.data(), datagram.size(), 0,
          reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
      if (sent < 0 || static_cast<std::size_t>(sent) != datagram.size()) {
        error = errno_message("sendto");
        return false;
      }
      ++stats.fragments_sent;
    }
    ++stats.units_sent;
    stats.payload_bytes_sent += access_units[au_index].size();

    std::vector<std::vector<std::uint8_t>> received;
    received.reserve(datagrams.size());
    for (std::size_t i = 0; i < datagrams.size(); ++i) {
      sockaddr_in from{};
      socklen_t from_len = sizeof(from);
      const ssize_t got =
          recv_retry(sock.fd, recv_buf.data(), recv_buf.size(), from, from_len);
      if (got < 0) {
        error = errno_message("recvfrom");
        return false;
      }
      received.emplace_back(recv_buf.begin(), recv_buf.begin() + got);
      ++stats.fragments_recv;
    }

    std::vector<std::uint8_t> payload;
    if (!reassemble_access_unit(received, static_cast<std::uint32_t>(au_index),
                                payload, error)) {
      return false;
    }
    if (payload != access_units[au_index]) {
      error = "loopback payload mismatch";
      return false;
    }
    ++stats.units_recv;
    stats.payload_bytes_recv += payload.size();
  }

  if (stats.units_sent != stats.units_recv ||
      stats.payload_bytes_sent != stats.payload_bytes_recv ||
      stats.fragments_sent != stats.fragments_recv) {
    error = "loopback counters disagree";
    return false;
  }
  error.clear();
  return true;
}

}  // namespace learning::video::local_transport
