/*!
\file http.c
\brief HTTP support
*/

/*
This file is part of Algol68G - an Algol 68 interpreter.
Copyright (C) 2001-2008 J. Marcel van der Veer <algol68g@xs4all.nl>.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "algol68g.h"
#include "genie.h"
#include "transput.h"

#if defined ENABLE_HTTP

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define PROTOCOL "tcp"
#define SERVICE "http"

#define CONTENT_BUFFER_SIZE (4 * KILOBYTE)
#define TIMEOUT_INTERVAL 15

/*!
\brief send GET request to server and yield answer (TCP/HTTP only)
\param p position in syntax tree, should not be NULL
**/

void genie_http_content (NODE_T * p)
{
  A68_REF path_string, domain_string, content_string;
  A68_INT port_number;
  int socket_id, conn, k;
  fd_set set;
  struct timeval timeout;
  struct servent *service_address;
  struct hostent *host_address;
  struct protoent *protocol;
  struct sockaddr_in socket_address;
  char buffer[CONTENT_BUFFER_SIZE];
  RESET_ERRNO;
/* Pop arguments. */
  POP_OBJECT (p, &port_number, A68_INT);
  CHECK_INIT (p, INITIALISED (&port_number), MODE (INT));
  POP_REF (p, &path_string);
  CHECK_INIT (p, INITIALISED (&path_string), MODE (STRING));
  POP_REF (p, &domain_string);
  CHECK_INIT (p, INITIALISED (&domain_string), MODE (STRING));
  POP_REF (p, &content_string);
  CHECK_INIT (p, INITIALISED (&content_string), MODE (REF_STRING));
  CHECK_NIL (p, content_string, MODE (REF_STRING));
  *(A68_REF *) ADDRESS (&content_string) = empty_string (p);
/* Reset buffers. */
  reset_transput_buffer (DOMAIN_BUFFER);
  reset_transput_buffer (PATH_BUFFER);
  reset_transput_buffer (REQUEST_BUFFER);
  reset_transput_buffer (CONTENT_BUFFER);
  add_a_string_transput_buffer (p, DOMAIN_BUFFER, (BYTE_T *) & domain_string);
  add_a_string_transput_buffer (p, PATH_BUFFER, (BYTE_T *) & path_string);
/* Make request. */
  add_string_transput_buffer (p, REQUEST_BUFFER, "GET ");
  add_string_transput_buffer (p, REQUEST_BUFFER, get_transput_buffer (PATH_BUFFER));
  add_string_transput_buffer (p, REQUEST_BUFFER, " HTTP/1.0\n\n");
/* Connect to host. */
  FILL (&socket_address, 0, sizeof (socket_address));
  socket_address.sin_family = AF_INET;
  service_address = getservbyname (SERVICE, PROTOCOL);
  if (service_address == NULL) {
    PUSH_PRIMITIVE (p, 1, A68_INT);
    return;
  };
  if (port_number.value == 0) {
    socket_address.sin_port = service_address->s_port;
  } else {
    socket_address.sin_port = htons ((unsigned short) port_number.value);
    if (socket_address.sin_port == 0) {
      PUSH_PRIMITIVE (p, (errno == 0 ? 1 : errno), A68_INT);
      return;
    };
  }
  host_address = gethostbyname (get_transput_buffer (DOMAIN_BUFFER));
  if (host_address == NULL) {
    PUSH_PRIMITIVE (p, (errno == 0 ? 1 : errno), A68_INT);
    return;
  };
  COPY (&socket_address.sin_addr, host_address->h_addr, host_address->h_length);
  protocol = getprotobyname (PROTOCOL);
  if (protocol == NULL) {
    PUSH_PRIMITIVE (p, (errno == 0 ? 1 : errno), A68_INT);
    return;
  };
  socket_id = socket (PF_INET, SOCK_STREAM, protocol->p_proto);
  if (socket_id < 0) {
    PUSH_PRIMITIVE (p, (errno == 0 ? 1 : errno), A68_INT);
    return;
  };
  conn = connect (socket_id, (const struct sockaddr *) &socket_address, ALIGNED_SIZEOF (socket_address));
  if (conn < 0) {
    PUSH_PRIMITIVE (p, (errno == 0 ? 1 : errno), A68_INT);
    close (socket_id);
    return;
  };
/* Read from host. */
  io_write_string (socket_id, get_transput_buffer (REQUEST_BUFFER));
  if (errno != 0) {
    PUSH_PRIMITIVE (p, errno, A68_INT);
    close (socket_id);
    return;
  };
/* Initialise file descriptor set. */
  FD_ZERO (&set);
  FD_SET (socket_id, &set);
/* Initialise the timeout data structure. */
  timeout.tv_sec = TIMEOUT_INTERVAL;
  timeout.tv_usec = 0;
/* Block until server replies or timeout blows up. */
  switch (select (FD_SETSIZE, &set, NULL, NULL, &timeout)) {
  case 0:
    {
      errno = ETIMEDOUT;
      PUSH_PRIMITIVE (p, errno, A68_INT);
      close (socket_id);
      return;
    }
  case -1:
    {
      PUSH_PRIMITIVE (p, errno, A68_INT);
      close (socket_id);
      return;
    }
  case 1:
    {
      break;
    }
  default:
    {
      ABNORMAL_END (A68_TRUE, "unexpected result from select", NULL);
    }
  }
  while ((k = io_read (socket_id, &buffer, (CONTENT_BUFFER_SIZE - 1))) > 0) {
    buffer[k] = NULL_CHAR;
    add_string_transput_buffer (p, CONTENT_BUFFER, buffer);
  }
  if (k < 0 || errno != 0) {
    PUSH_PRIMITIVE (p, (errno == 0 ? 1 : errno), A68_INT);
    close (socket_id);
    return;
  };
/* Convert string. */
  *(A68_REF *) ADDRESS (&content_string) = c_to_a_string (p, get_transput_buffer (CONTENT_BUFFER));
  close (socket_id);
  PUSH_PRIMITIVE (p, errno, A68_INT);
}

/*!
\brief send request to server and yield answer (TCP only)
\param p position in syntax tree, should not be NULL
**/

void genie_tcp_request (NODE_T * p)
{
  A68_REF path_string, domain_string, content_string;
  A68_INT port_number;
  int socket_id, conn, k;
  fd_set set;
  struct timeval timeout;
  struct servent *service_address;
  struct hostent *host_address;
  struct protoent *protocol;
  struct sockaddr_in socket_address;
  char buffer[CONTENT_BUFFER_SIZE];
  RESET_ERRNO;
/* Pop arguments. */
  POP_OBJECT (p, &port_number, A68_INT);
  CHECK_INIT (p, INITIALISED (&port_number), MODE (INT));
  POP_REF (p, &path_string);
  CHECK_INIT (p, INITIALISED (&path_string), MODE (STRING));
  POP_REF (p, &domain_string);
  CHECK_INIT (p, INITIALISED (&domain_string), MODE (STRING));
  POP_REF (p, &content_string);
  CHECK_INIT (p, INITIALISED (&content_string), MODE (REF_STRING));
  CHECK_NIL (p, content_string, MODE (REF_STRING));
  *(A68_REF *) ADDRESS (&content_string) = empty_string (p);
/* Reset buffers. */
  reset_transput_buffer (DOMAIN_BUFFER);
  reset_transput_buffer (PATH_BUFFER);
  reset_transput_buffer (REQUEST_BUFFER);
  reset_transput_buffer (CONTENT_BUFFER);
  add_a_string_transput_buffer (p, DOMAIN_BUFFER, (BYTE_T *) & domain_string);
  add_a_string_transput_buffer (p, PATH_BUFFER, (BYTE_T *) & path_string);
/* Make request. */
  add_string_transput_buffer (p, REQUEST_BUFFER, get_transput_buffer (PATH_BUFFER));
/* Connect to host. */
  FILL (&socket_address, 0, sizeof (socket_address));
  socket_address.sin_family = AF_INET;
  service_address = getservbyname (SERVICE, PROTOCOL);
  if (service_address == NULL) {
    PUSH_PRIMITIVE (p, 1, A68_INT);
    return;
  };
  if (port_number.value == 0) {
    socket_address.sin_port = service_address->s_port;
  } else {
    socket_address.sin_port = htons ((unsigned short) port_number.value);
    if (socket_address.sin_port == 0) {
      PUSH_PRIMITIVE (p, (errno == 0 ? 1 : errno), A68_INT);
      return;
    };
  }
  host_address = gethostbyname (get_transput_buffer (DOMAIN_BUFFER));
  if (host_address == NULL) {
    PUSH_PRIMITIVE (p, (errno == 0 ? 1 : errno), A68_INT);
    return;
  };
  COPY (&socket_address.sin_addr, host_address->h_addr, host_address->h_length);
  protocol = getprotobyname (PROTOCOL);
  if (protocol == NULL) {
    PUSH_PRIMITIVE (p, (errno == 0 ? 1 : errno), A68_INT);
    return;
  };
  socket_id = socket (PF_INET, SOCK_STREAM, protocol->p_proto);
  if (socket_id < 0) {
    PUSH_PRIMITIVE (p, (errno == 0 ? 1 : errno), A68_INT);
    return;
  };
  conn = connect (socket_id, (const struct sockaddr *) &socket_address, ALIGNED_SIZEOF (socket_address));
  if (conn < 0) {
    PUSH_PRIMITIVE (p, (errno == 0 ? 1 : errno), A68_INT);
    close (socket_id);
    return;
  };
/* Read from host. */
  io_write_string (socket_id, get_transput_buffer (REQUEST_BUFFER));
  if (errno != 0) {
    PUSH_PRIMITIVE (p, errno, A68_INT);
    close (socket_id);
    return;
  };
/* Initialise file descriptor set. */
  FD_ZERO (&set);
  FD_SET (socket_id, &set);
/* Initialise the timeout data structure. */
  timeout.tv_sec = TIMEOUT_INTERVAL;
  timeout.tv_usec = 0;
/* Block until server replies or timeout blows up. */
  switch (select (FD_SETSIZE, &set, NULL, NULL, &timeout)) {
  case 0:
    {
      errno = ETIMEDOUT;
      PUSH_PRIMITIVE (p, errno, A68_INT);
      close (socket_id);
      return;
    }
  case -1:
    {
      PUSH_PRIMITIVE (p, errno, A68_INT);
      close (socket_id);
      return;
    }
  case 1:
    {
      break;
    }
  default:
    {
      ABNORMAL_END (A68_TRUE, "unexpected result from select", NULL);
    }
  }
  while ((k = io_read (socket_id, &buffer, (CONTENT_BUFFER_SIZE - 1))) > 0) {
    buffer[k] = NULL_CHAR;
    add_string_transput_buffer (p, CONTENT_BUFFER, buffer);
  }
  if (k < 0 || errno != 0) {
    PUSH_PRIMITIVE (p, (errno == 0 ? 1 : errno), A68_INT);
    close (socket_id);
    return;
  };
/* Convert string. */
  *(A68_REF *) ADDRESS (&content_string) = c_to_a_string (p, get_transput_buffer (CONTENT_BUFFER));
  close (socket_id);
  PUSH_PRIMITIVE (p, errno, A68_INT);
}

#endif
