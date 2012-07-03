/*
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 */

#include <err.h>
#include <stdio.h>
#include <proton/buffer.h>
#include <proton/codec.h>
#include <proton/error.h>
#include <proton/framing.h>
#include "util.h"

int dump(const char *file)
{
  FILE *in = fopen(file, "r");
  if (!in) err(1, "%s", file);

  pn_buffer_t *buf = pn_buffer(1024);
  pn_data_t *data = pn_data(16);
  bool header = false;

  char bytes[1024];
  size_t n;
  while ((n = fread(bytes, 1, 1024, in))) {
    int err = pn_buffer_append(buf, bytes, n);
    if (err) return err;

    while (true) {
      pn_bytes_t available = pn_buffer_bytes(buf);
      if (!available.size) break;

      if (!header) {
        if (available.size >= 8) {
          pn_buffer_trim(buf, 8, 0);
          available = pn_buffer_bytes(buf);
          header = true;
        } else {
          break;
        }
      }

      pn_frame_t frame;
      size_t consumed = pn_read_frame(&frame, available.start, available.size);
      if (consumed) {
        size_t dsize = frame.size;
        pn_data_clear(data);
        err = pn_data_decode(data, frame.payload, &dsize);
        if (err) {
          fprintf(stderr, "Error decoding frame: %s\n", pn_code(err));
          pn_fprint_data(stderr, frame.payload, frame.size);
          fprintf(stderr, "\n");
          return err;
        } else {
          pn_data_print(data);
          printf("\n");
        }
        pn_buffer_trim(buf, consumed, 0);
      } else {
        break;
      }
    }
  }

  if (ferror(in)) err(1, "%s", file);
  if (pn_buffer_size(buf) > 0) {
    fprintf(stderr, "Trailing data: ");
    pn_bytes_t b = pn_buffer_bytes(buf);
    pn_fprint_data(stderr, b.start, b.size);
    fprintf(stderr, "\n");
  }

  fclose(in);

  return 0;
}

int main(int argc, char **argv)
{
  for (int i = 1; i < argc; i++) {
    int err = dump(argv[i]);
    if (err) return err;
  }

  return 0;
}
