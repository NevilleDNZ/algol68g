//! @file rts-http.c
//! @author J. Marcel van der Veer

//! @section Copyright
//!
//! This file is part of Algol68G - an Algol 68 compiler-interpreter.
//! Copyright 2001-2023 J. Marcel van der Veer [algol68g@xs4all.nl].

//! @section License
//!
//! This program is free software; you can redistribute it and/or modify it 
//! under the terms of the GNU General Public License as published by the 
//! Free Software Foundation; either version 3 of the License, or 
//! (at your option) any later version.
//!
//! This program is distributed in the hope that it will be useful, but 
//! WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
//! or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for 
//! more details. You should have received a copy of the GNU General Public 
//! License along with this program. If not, see [http://www.gnu.org/licenses/].

//! @section Synopsis
//!
//! HTTP client.

#include "a68g.h"
#include "a68g-genie.h"
#include "a68g-prelude.h"
#include "a68g-transput.h"

#if defined (BUILD_WWW)

#if defined (HAVE_NETDB_H)
#  include <netdb.h>
#endif

#if defined (HAVE_NETINET_IN_H)
#  include <netinet/in.h>
#endif

#if defined (HAVE_SYS_SELECT_H)
#  include <sys/select.h>
#endif

#if defined (HAVE_SYS_SOCKET_H)
#  include <sys/socket.h>
#endif

#define PROTOCOL "tcp"
#define SERVICE "http"

#define CONTENT_BUFFER_SIZE (64 * KILOBYTE)
#define TIMEOUT_INTERVAL 15

// Own implementation of connect with time-out. 

int a68_connect (int socket_id, struct sockaddr * addr, size_t addrlen, struct timeval * timeout)
{
  int flags = fcntl (socket_id, F_GETFL, NULL);
  if (flags < 0) {
    return -1;
  }
  if (fcntl (socket_id, F_SETFL, flags | O_NONBLOCK) < 0) {
    return -1;
  }
  int ret = connect (socket_id, addr, addrlen);
  if (ret < 0) {
    if (errno == EINPROGRESS) {
      fd_set wait_set;
      FD_ZERO (&wait_set);
      FD_SET (socket_id, &wait_set);
      ret = select (socket_id + 1, NULL, &wait_set, NULL, timeout);
    }
  } else {
    ret = 1;
  }
  if (fcntl (socket_id, F_SETFL, flags) < 0) {
    return -1;
  }
  if (ret < 0) {
    return -1;
  } else if (ret == 0) {
    errno = ETIMEDOUT;
    return 1;
  } else {
    socklen_t len = sizeof (flags);
    if (getsockopt (socket_id, SOL_SOCKET, SO_ERROR, &flags, &len) < 0) {
      return -1;
    }
    if (flags) {
      errno = flags;
      return -1;
    }
  }
  return 0;
}

//! @brief PROC (REF STRING, STRING, STRING, INT) INT http content 

void genie_http_content (NODE_T * p)
{
// Send GET request to server and yield answer (TCP/HTTP only).
  errno = 0;
// Pop arguments.
  A68_INT port;
  POP_OBJECT (p, &port, A68_INT);
  CHECK_INIT (p, INITIALISED (&port), M_INT);
  A68_REF path_string, domain_string, content_string;
  POP_REF (p, &path_string);
  CHECK_INIT (p, INITIALISED (&path_string), M_STRING);
  POP_REF (p, &domain_string);
  CHECK_INIT (p, INITIALISED (&domain_string), M_STRING);
  POP_REF (p, &content_string);
  CHECK_REF (p, content_string, M_REF_STRING);
  *DEREF (A68_REF, &content_string) = empty_string (p);
// Reset buffers.
  reset_transput_buffer (DOMAIN_BUFFER);
  add_a_string_transput_buffer (p, DOMAIN_BUFFER, (BYTE_T *) & domain_string);
  reset_transput_buffer (PATH_BUFFER);
  add_a_string_transput_buffer (p, PATH_BUFFER, (BYTE_T *) & path_string);
// Compose request. Double \r\n at the end is essential!
  reset_transput_buffer (REQUEST_BUFFER);
  add_string_transput_buffer (p, REQUEST_BUFFER, "GET ");
  add_string_transput_buffer (p, REQUEST_BUFFER, get_transput_buffer (PATH_BUFFER));
  add_string_transput_buffer (p, REQUEST_BUFFER, " HTTP/1.0\r\n");
  add_string_transput_buffer (p, REQUEST_BUFFER, "Host: ");
  add_string_transput_buffer (p, REQUEST_BUFFER, get_transput_buffer (DOMAIN_BUFFER));
  add_string_transput_buffer (p, REQUEST_BUFFER, "\r\nAccept: */*\r\nConnection: close\r\n\r\n");
// Connect to host.
  struct timeval a68_timeout;
  TV_SEC (&a68_timeout) = TIMEOUT_INTERVAL;
  TV_USEC (&a68_timeout) = 0;
  struct sockaddr_in socket_address;
  FILL (&socket_address, 0, (int) sizeof (socket_address));
  SIN_FAMILY (&socket_address) = AF_INET;
  struct servent *service_address = getservbyname (SERVICE, PROTOCOL);
  if (service_address == NULL) {
    PUSH_VALUE (p, 1, A68_INT);
    return;
  }
  if (VALUE (&port) == 0) {
    SIN_PORT (&socket_address) = (uint16_t) (S_PORT (service_address));
  } else {
    SIN_PORT (&socket_address) = (uint16_t) (htons ((uint16_t) (VALUE (&port))));
    if (SIN_PORT (&socket_address) == 0) {
      PUSH_VALUE (p, (errno == 0 ? 1 : errno), A68_INT);
      return;
    }
  }
  struct hostent *host_address = gethostbyname (get_transput_buffer (DOMAIN_BUFFER));
  if (host_address == NULL) {
    PUSH_VALUE (p, (errno == 0 ? 1 : errno), A68_INT);
    return;
  }
  COPY (&SIN_ADDR (&socket_address), H_ADDR (host_address), H_LENGTH (host_address));
  struct protoent *protocol = getprotobyname (PROTOCOL);
  if (protocol == NULL) {
    PUSH_VALUE (p, (errno == 0 ? 1 : errno), A68_INT);
    return;
  }
  int socket_id = socket (PF_INET, SOCK_STREAM, P_PROTO (protocol));
  if (socket_id < 0) {
    PUSH_VALUE (p, (errno == 0 ? 1 : errno), A68_INT);
    return;
  }
  int conn = a68_connect (socket_id, (struct sockaddr *) &socket_address, (socklen_t) SIZE_ALIGNED (socket_address), &a68_timeout);
  if (conn < 0) {
    PUSH_VALUE (p, (errno == 0 ? 1 : errno), A68_INT);
    ASSERT (close (socket_id) == 0);
    return;
  }
// Read from host.
  WRITE (socket_id, get_transput_buffer (REQUEST_BUFFER));
  if (errno != 0) {
    PUSH_VALUE (p, errno, A68_INT);
    ASSERT (close (socket_id) == 0);
    return;
  }
// Initialise file descriptor set.
  fd_set set;
  FD_ZERO (&set);
  FD_SET (socket_id, &set);
// Block until server replies or a68_timeout blows up.
  switch (select (FD_SETSIZE, &set, NULL, NULL, &a68_timeout)) {
  case 0: {
      errno = ETIMEDOUT;
      PUSH_VALUE (p, errno, A68_INT);
      ASSERT (close (socket_id) == 0);
      return;
    }
  case -1: {
      PUSH_VALUE (p, errno, A68_INT);
      ASSERT (close (socket_id) == 0);
      return;
    }
  case 1: {
      break;
    }
  default: {
      ABEND (A68_TRUE, ERROR_ACTION, __func__);
    }
  }
// Read from the socket.
  char buffer[CONTENT_BUFFER_SIZE];
  reset_transput_buffer (CONTENT_BUFFER);
  int k;
  while ((k = (int) io_read (socket_id, &buffer, (CONTENT_BUFFER_SIZE - 1))) > 0) {
    add_chars_transput_buffer (p, CONTENT_BUFFER, k, buffer);
  }
  if (k < 0 || errno != 0) {
    PUSH_VALUE (p, (errno == 0 ? 1 : errno), A68_INT);
    ASSERT (close (socket_id) == 0);
    return;
  }
// Convert string.
  *DEREF (A68_REF, &content_string) = c_to_a_string (p, get_transput_buffer (CONTENT_BUFFER), get_transput_buffer_index (CONTENT_BUFFER));
  ASSERT (close (socket_id) == 0);
  PUSH_VALUE (p, errno, A68_INT);
}

//! @brief PROC (REF STRING, STRING, STRING, INT) INT tcp request

void genie_tcp_request (NODE_T * p)
{
// Send request to server and yield answer (TCP only).
  errno = 0;
// Pop arguments.
  A68_INT port;
  POP_OBJECT (p, &port, A68_INT);
  CHECK_INIT (p, INITIALISED (&port), M_INT);
  A68_REF request_string, domain_string, content_string;
  POP_REF (p, &request_string);
  CHECK_INIT (p, INITIALISED (&request_string), M_STRING);
  POP_REF (p, &domain_string);
  CHECK_INIT (p, INITIALISED (&domain_string), M_STRING);
  POP_REF (p, &content_string);
  CHECK_REF (p, content_string, M_REF_STRING);
  *DEREF (A68_REF, &content_string) = empty_string (p);
// Reset buffers.
  reset_transput_buffer (DOMAIN_BUFFER);
  reset_transput_buffer (REQUEST_BUFFER);
  reset_transput_buffer (CONTENT_BUFFER);
  add_a_string_transput_buffer (p, DOMAIN_BUFFER, (BYTE_T *) & domain_string);
  add_a_string_transput_buffer (p, REQUEST_BUFFER, (BYTE_T *) & request_string);
  add_string_transput_buffer (p, REQUEST_BUFFER, "\r\n\r\n");
// Connect to host.
  struct timeval a68_timeout;
  TV_SEC (&a68_timeout) = TIMEOUT_INTERVAL;
  TV_USEC (&a68_timeout) = 0;
  struct sockaddr_in socket_address;
  FILL (&socket_address, 0, (int) sizeof (socket_address));
  SIN_FAMILY (&socket_address) = AF_INET;
  struct servent *service_address = getservbyname (SERVICE, PROTOCOL);
  if (service_address == NULL) {
    PUSH_VALUE (p, 1, A68_INT);
    return;
  }
  if (VALUE (&port) == 0) {
    SIN_PORT (&socket_address) = (uint16_t) (S_PORT (service_address));
  } else {
    SIN_PORT (&socket_address) = (uint16_t) (htons ((uint16_t) (VALUE (&port))));
    if (SIN_PORT (&socket_address) == 0) {
      PUSH_VALUE (p, (errno == 0 ? 1 : errno), A68_INT);
      return;
    }
  }
  struct hostent *host_address = gethostbyname (get_transput_buffer (DOMAIN_BUFFER));
  if (host_address == NULL) {
    PUSH_VALUE (p, (errno == 0 ? 1 : errno), A68_INT);
    return;
  }
  COPY (&SIN_ADDR (&socket_address), H_ADDR (host_address), H_LENGTH (host_address));
  struct protoent *protocol = getprotobyname (PROTOCOL);
  if (protocol == NULL) {
    PUSH_VALUE (p, (errno == 0 ? 1 : errno), A68_INT);
    return;
  }
  int socket_id = socket (PF_INET, SOCK_STREAM, P_PROTO (protocol));
  if (socket_id < 0) {
    PUSH_VALUE (p, (errno == 0 ? 1 : errno), A68_INT);
    return;
  }
  int conn = a68_connect (socket_id, (struct sockaddr *) &socket_address, (socklen_t) SIZE_ALIGNED (socket_address), &a68_timeout);
  if (conn < 0) {
    PUSH_VALUE (p, (errno == 0 ? 1 : errno), A68_INT);
    ASSERT (close (socket_id) == 0);
    return;
  }
// Read from host.
  WRITE (socket_id, get_transput_buffer (REQUEST_BUFFER));
  if (errno != 0) {
    PUSH_VALUE (p, errno, A68_INT);
    ASSERT (close (socket_id) == 0);
    return;
  }
// Initialise file descriptor set.
  fd_set set;
  FD_ZERO (&set);
  FD_SET (socket_id, &set);
// Block until server replies or a68_timeout blows up.
  switch (select (FD_SETSIZE, &set, NULL, NULL, &a68_timeout)) {
  case 0: {
      errno = ETIMEDOUT;
      PUSH_VALUE (p, errno, A68_INT);
      ASSERT (close (socket_id) == 0);
      return;
    }
  case -1: {
      PUSH_VALUE (p, errno, A68_INT);
      ASSERT (close (socket_id) == 0);
      return;
    }
  case 1: {
      break;
    }
  default: {
      ABEND (A68_TRUE, ERROR_ACTION, __func__);
    }
  }
  char buffer[CONTENT_BUFFER_SIZE];
  int k;
  while ((k = (int) io_read (socket_id, &buffer, (CONTENT_BUFFER_SIZE - 1))) > 0) {
    add_chars_transput_buffer (p, CONTENT_BUFFER, k, buffer);
  }
  if (k < 0 || errno != 0) {
    PUSH_VALUE (p, (errno == 0 ? 1 : errno), A68_INT);
    ASSERT (close (socket_id) == 0);
    return;
  }
// Convert string.
  *DEREF (A68_REF, &content_string) = c_to_a_string (p, get_transput_buffer (CONTENT_BUFFER), get_transput_buffer_index (CONTENT_BUFFER));
  ASSERT (close (socket_id) == 0);
  PUSH_VALUE (p, errno, A68_INT);
}

#endif
