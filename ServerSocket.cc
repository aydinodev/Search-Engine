/*
 * Copyright ©2024 Hannah C. Tang.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Washington
 * CSE 333 for use solely during Spring Quarter 2024 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include <stdio.h>       // for snprintf()
#include <unistd.h>      // for close(), fcntl()
#include <sys/types.h>   // for socket(), getaddrinfo(), etc.
#include <sys/socket.h>  // for socket(), getaddrinfo(), etc.
#include <arpa/inet.h>   // for inet_ntop()
#include <netdb.h>       // for getaddrinfo()
#include <errno.h>       // for errno, used by strerror()
#include <string.h>      // for memset, strerror()
#include <iostream>      // for std::cerr, etc.

#include "./ServerSocket.h"

extern "C" {
  #include "libhw1/CSE333.h"
}

namespace hw4 {

ServerSocket::ServerSocket(uint16_t port) {
  port_ = port;
  listen_sock_fd_ = -1;
}

ServerSocket::~ServerSocket() {
  // Close the listening socket if it's not zero.  The rest of this
  // class will make sure to zero out the socket if it is closed
  // elsewhere.
  if (listen_sock_fd_ != -1)
    close(listen_sock_fd_);
  listen_sock_fd_ = -1;
}

bool ServerSocket::BindAndListen(int ai_family, int* const listen_fd) {
  // Use "getaddrinfo," "socket," "bind," and "listen" to
  // create a listening socket on port port_.  Return the
  // listening socket through the output parameter "listen_fd"
  // and set the ServerSocket data member "listen_sock_fd_"

  // STEP 1:
  // creates addrinfo hints for getaddrinfo()
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));

  // ensures ai_family provides correct listening socket.
  if (ai_family != AF_INET && ai_family != AF_INET6 && ai_family != AF_UNSPEC) {
    return false;
  }

  // hints is filled.
  hints.ai_family = ai_family;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  struct addrinfo *result;
  std::string port = std::to_string(port_);

  // result stores list of addresses.
  int res = getaddrinfo(NULL, port.c_str(), &hints, &result);

  // getaddrinfo() failure.
  if (res != 0) {
    return false;
  }

  int ret_fd = -1;
  for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next) {
    ret_fd = socket(rp->ai_family,
                    rp->ai_socktype,
                    rp->ai_protocol);

    // try next socket, current socket invalid.
    if (ret_fd == -1) {
      continue;
    }

    // set the SO_REUSEADDR option to make the port immediately
    // reusable after exit.
    int optval = 1;
    Verify333(setsockopt(ret_fd, SOL_SOCKET, SO_REUSEADDR,
             &optval, sizeof(optval)) == 0);

    // bind the socket to the address and port from getaddrinfo().
    if (bind(ret_fd, rp->ai_addr, rp->ai_addrlen) == 0) {
      // bind success.
      sock_family_ = rp->ai_family;
      break;
    }

    // on bind failure, close the socket and try the next
    // address/port from getaddrinfo().
    close(ret_fd);
    ret_fd = -1;
  }
  freeaddrinfo(result);

  // bind failure.
  if (ret_fd == -1) {
    return false;
  }

  // set listening socket.
  if (listen(ret_fd, SOMAXCONN) != 0) {
    // socket not made listening socket.
    close(ret_fd);
    return false;
  }

  // storing listening socket file descriptor.
  listen_sock_fd_ = ret_fd;
  *listen_fd = ret_fd;
  return true;
}

bool ServerSocket::Accept(int* const accepted_fd,
                          std::string* const client_addr,
                          uint16_t* const client_port,
                          std::string* const client_dns_name,
                          std::string* const server_addr,
                          std::string* const server_dns_name) const {
  // Accept a new connection on the listening socket listen_sock_fd_.
  // (Block until a new connection arrives.)  Return the newly accepted
  // socket, as well as information about both ends of the new connection,
  // through the various output parameters.

  // STEP 2:
  struct sockaddr_storage caddr;
  socklen_t caddr_len = sizeof(caddr);
  struct sockaddr *addr = reinterpret_cast<struct sockaddr *>(&caddr);
  int client_fd = -1;
  while (1) {
    client_fd = accept(listen_sock_fd_,
                      addr,
                      &caddr_len);

    if (client_fd < 0) {
      if ((errno == EAGAIN) || (errno == EINTR)) {
        continue;  // retry on temporary errors.
      }
      return false;  // fail on other errors.
    }
    break;  // connection accepted.
  }

  // check for valid client_fd.
  if (client_fd < 0) {
    return false;
  }

  // store client file descriptor.
  *accepted_fd = client_fd;

  // get client's IP address and port.
  if (addr->sa_family == AF_INET) {
    // IPv4 address.
    char addr_str[INET_ADDRSTRLEN];
    struct sockaddr_in *in4 = reinterpret_cast<struct sockaddr_in *>(addr);
    inet_ntop(AF_INET, &(in4->sin_addr), addr_str, INET_ADDRSTRLEN);

    *client_addr = std::string(addr_str);
    *client_port = htons(in4->sin_port);
  } else {
    // IPv6 address.
    char addr_str[INET6_ADDRSTRLEN];
    struct sockaddr_in6 *in6 = reinterpret_cast<struct sockaddr_in6 *>(addr);
    inet_ntop(AF_INET6, &(in6->sin6_addr), addr_str, INET6_ADDRSTRLEN);

    *client_addr = std::string(addr_str);
    *client_port = htons(in6->sin6_port);
  }

  // get client's DNS name.
  char host_name[1024];
  Verify333(getnameinfo(addr, caddr_len, host_name, 1024, NULL, 0, 0) == 0);
  *client_dns_name = std::string(host_name);

  // get server's IP address and DNS name.
  char h_name[1024];
  h_name[0] = '\0';
  if (sock_family_ == AF_INET) {
    // IPv4 address.
    struct sockaddr_in srvr;
    socklen_t srvr_len = sizeof(srvr);
    char addr_buf[INET_ADDRSTRLEN];
    getsockname(client_fd, (struct sockaddr *) &srvr, &srvr_len);
    inet_ntop(AF_INET, &srvr.sin_addr, addr_buf, INET_ADDRSTRLEN);
    getnameinfo((const struct sockaddr *) &srvr,
                srvr_len, h_name, 1024, NULL, 0, 0);

    *server_addr = std::string(addr_buf);
    *server_dns_name = std::string(h_name);
  } else {
    // IPv6 address.
    struct sockaddr_in6 srvr;
    socklen_t srvr_len = sizeof(srvr);
    char addr_buf[INET6_ADDRSTRLEN];
    getsockname(client_fd, (struct sockaddr *) &srvr, &srvr_len);
    inet_ntop(AF_INET6, &srvr.sin6_addr, addr_buf, INET_ADDRSTRLEN);
    getnameinfo((const struct sockaddr *) &srvr,
                srvr_len, h_name, 1024, NULL, 0, 0);

    *server_addr = std::string(addr_buf);
    *server_dns_name = std::string(h_name);
  }
  return true;
}

}  // namespace hw4
