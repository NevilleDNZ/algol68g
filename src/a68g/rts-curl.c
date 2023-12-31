//! @file rts-curl.c
//! @author J. Marcel van der Veer

//! @section Copyright
//!
//! This file is part of Algol68G - an Algol 68 compiler-interpreter.
//! Copyright 2001-2024 J. Marcel van der Veer [algol68g@xs4all.nl].

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
//! HTTP/HTTPS client.

#include "a68g.h"
#include "a68g-genie.h"
#include "a68g-prelude.h"
#include "a68g-transput.h"

#if defined (HAVE_CURL)

#if defined (HAVE_CURL_CURL_H)
#  include <curl/curl.h>
#endif

typedef struct DATA_T DATA_T;
struct DATA_T {
  char *ref;
  size_t len;
};

#define NO_DATA ((DATA_T *) NULL)

// Callback function concatenating received data.

static size_t a68_curl_concat (void *data, size_t len, size_t n, void *buf)
{
// Sanity checks.
  if ((char *) data == NO_TEXT || (DATA_T *) buf == NO_DATA) {
    return 0;
  } else if (n == 0 || len == 0) {
    return 0;
  }
  ABEND (len >= (2 * GIGABYTE) / n, ERROR_OUT_OF_CORE, NO_TEXT);
  size_t new_len = n * len;
  ABEND (new_len + 1 > 2 * GIGABYTE - ((DATA_T *) buf)->len, ERROR_OUT_OF_CORE, NO_TEXT);
  char *stale = ((DATA_T *) buf)->ref;
  ((DATA_T *) buf)->ref = malloc (((DATA_T *) buf)->len + new_len + 1);
  ABEND (((DATA_T *) buf)->ref == NO_TEXT, ERROR_OUT_OF_CORE, NO_TEXT);
  if (stale != NO_TEXT) {
    MOVE (((DATA_T *) buf)->ref, stale, ((DATA_T *) buf)->len + 1);
    free (stale);
  }
  MOVE (& (((DATA_T *) buf)->ref[((DATA_T *) buf)->len]), data, new_len);
  ((DATA_T *) buf)->len += new_len;
  ((DATA_T *) buf)->ref[((DATA_T *) buf)->len] = NULL_CHAR;
  return new_len;
}

void genie_curl_content (NODE_T * p, char *protocol)
{
  A68_REF path_string, domain_string, content_string;
  A68_INT port;
  errno = 0;
// Pop arguments.
  POP_OBJECT (p, &port, A68_INT);
  CHECK_INIT (p, INITIALISED (&port), M_INT); // Unused for now.
  POP_REF (p, &path_string);
  CHECK_INIT (p, INITIALISED (&path_string), M_STRING);
  POP_REF (p, &domain_string);
  CHECK_INIT (p, INITIALISED (&domain_string), M_STRING);
  POP_REF (p, &content_string);
  CHECK_REF (p, content_string, M_REF_STRING);
  *DEREF (A68_REF, &content_string) = empty_string (p);
// Set buffers.
  reset_transput_buffer (DOMAIN_BUFFER);
  add_a_string_transput_buffer (p, DOMAIN_BUFFER, (BYTE_T *) & domain_string);
  reset_transput_buffer (PATH_BUFFER);
  add_a_string_transput_buffer (p, PATH_BUFFER, (BYTE_T *) & path_string);
// Compose request.
  reset_transput_buffer (REQUEST_BUFFER);
  if (protocol != NO_TEXT) {
    add_string_transput_buffer (p, REQUEST_BUFFER, protocol);
  }
  add_string_transput_buffer (p, REQUEST_BUFFER, get_transput_buffer (DOMAIN_BUFFER));
  add_string_transput_buffer (p, REQUEST_BUFFER, get_transput_buffer (PATH_BUFFER));
// cURL connects to host, negotiates and collects data.
  DATA_T data = {NO_TEXT, 0};
  curl_global_init (CURL_GLOBAL_ALL);
  CURL *handle = curl_easy_init ();
  curl_easy_setopt (handle, CURLOPT_URL, get_transput_buffer (REQUEST_BUFFER));
  curl_easy_setopt (handle, CURLOPT_WRITEFUNCTION, a68_curl_concat);
  curl_easy_setopt (handle, CURLOPT_WRITEDATA, (void *) &data);
  curl_easy_setopt (handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
  CURLcode rc = curl_easy_perform (handle);
// Wrap it up.
  if (rc != CURLE_OK) {
    errno = rc;
  } else {
    *DEREF (A68_REF, &content_string) = c_to_a_string (p, data.ref, data.len);
  }
  if (data.ref != NO_TEXT) {
    free (data.ref);
  }
  curl_easy_cleanup (handle);
  curl_global_cleanup ();
  PUSH_VALUE (p, errno, A68_INT);
}

//! @brief PROC (REF STRING, STRING, STRING, INT) INT http content 

void genie_http_content (NODE_T * p)
{
   genie_curl_content (p, "http://");
}

//! @brief PROC (REF STRING, STRING, STRING, INT) INT https content 

void genie_https_content (NODE_T * p)
{
   genie_curl_content (p, "https://");
}

#endif
