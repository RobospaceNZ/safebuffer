/*
 * safebuffer.h
 *
 * Copyright (c) 2025 Hubert Jetschko
 *
 * This file is licensed under the MIT License.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef SAFEBUFFER_H_
#define SAFEBUFFER_H_

#include "stdbool.h"

typedef struct
{
  uint8_t *buf;                 // Pointer to the start of the buffer
  uint32_t len;                 // Length of the buffer
  uint32_t write_index;         // Points to the end of the data currently in buffer
  uint32_t read_index;          // This can be used by the user for reading from the buffer
  bool full;                    // True if the buffer is full
  bool overflow;                // True if data written to the buffer was lost
}
safebuffer_t;

int safebuffer_init(safebuffer_t *safebuffer, uint8_t *buf, uint32_t len);
int safebuffer_init_const(safebuffer_t *safebuffer, const uint8_t *buf, uint32_t len);
int safebuffer_add_data(safebuffer_t *safebuffer, uint8_t *data, uint32_t len);
int safebuffer_add_safebuffer(safebuffer_t *safebuffer, safebuffer_t *safebuffer_in);
int safebuffer_add_string(safebuffer_t *safebuffer, uint8_t *p_str);
int safebuffer_add_char(safebuffer_t *safebuffer, uint8_t c);
uint32_t safebuffer_get_data_left_count(safebuffer_t *safebuffer);
int safebuffer_snprintf(safebuffer_t *safebuffer, const char* msg, ...);
int safebuffer_read_char(safebuffer_t *safebuffer, uint8_t *c);
int safebuffer_reset(safebuffer_t *safebuffer);
safebuffer_t *safebuffer_malloc(uint32_t len);
int safebuffer_free(safebuffer_t *safebuffer);

#endif /* SAFEBUFFER_H_ */
