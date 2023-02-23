/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 2021, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

/***************************************************************************
 *
 * Non-backend-specific ECH support code belongs here, such as functions
 * - to check ECH-related libcurl options
 *   for correctness and consistency
 * - to parse and display ECH data
 *
 * Backend-specific ECH support code belongs as additional
 * backend-interface code in one of the existing vlts backend
 * interface source files or in an ECH-specific source file
 * associated with one of these existing files.
 *
 ***************************************************************************/

#include "curl_setup.h"

#ifdef USE_ECH
#include <curl/curl.h>
#include "urldata.h"
#include "sendf.h"
#include "vtls/vtls.h"
#include "ech.h"

/* The last 3 #include files should be in this order */
#include "curl_printf.h"
#include "curl_memory.h"
#include "memdebug.h"

/**
 * Check completeness of ECH parameter data present in easy handle
 *
 * @param data is the Curl_easy handle to inspect
 * @return TRUE if complete, FALSE otherwise
 *
 * TODO: consider whether ECH parameter data needs to be per-connection
 */
bool Curl_ech_ready(struct Curl_easy *data)
{
  bool ready = TRUE;
  if(!data)
    return FALSE;               /* NULL handle: surely not ready! */

  if(data->set.tls_enable_ech) {
    /* ECH enabled: look for what will be needed */
    if(!data->set.str[STRING_ECH_CONFIG]) {
      infof(data, "WARNING: missing value for STRING_ECH_CONFIG\n");
      ready = FALSE;
    }
    else {
      infof(data, "ECH: found STRING_ECH_CONFIG:\n");
      infof(data, " %s\n", data->set.str[STRING_ECH_CONFIG]);
      if(data->set.str[STRING_ECH_PUBLIC]) {
        /* Optional STRING_ECH_PUBLIC: report if set */
        infof(data, "ECH: found STRING_ECH_PUBLIC:\n");
        infof(data, " %s\n", data->set.str[STRING_ECH_PUBLIC]);
      }
    }

    /* TODO: review completeness of inspection above */
  }

  /* Nothing missing, or ECH not required */
  return ready;
}

#endif  /* USE_ECH */