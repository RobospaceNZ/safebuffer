/*
 * safebuffer.c
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

#include <zephyr/kernel.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include "safebuffer.h"

int safebuffer_init(safebuffer_t *safebuffer, uint8_t *buf, uint32_t len) {
    if (safebuffer == NULL) {
        return -EFAULT;
    }
    safebuffer->buf = buf;
    safebuffer->len = len;
    safebuffer->full = false;
    safebuffer->overflow = false;
    safebuffer->write_index = 0;
    safebuffer->read_index = 0;
    return 0;
}

// Initialise a safe buffer to point to a constant buffer
int safebuffer_init_const(safebuffer_t *safebuffer, const uint8_t *buf, uint32_t len)
{
    if (safebuffer == NULL) {
        return -EFAULT;
    }
    if (buf == NULL) {
        return -EINVAL;
    }
    safebuffer->buf = (uint8_t*)buf;
    safebuffer->len = len;
    safebuffer->full = true;
    safebuffer->overflow = false;
    safebuffer->write_index = len;
    safebuffer->read_index = 0;
    return 0;
}

// Add data to a safe buffer. Return 0 if all data could be added
int safebuffer_add_data(safebuffer_t *safebuffer, uint8_t *data, uint32_t len)
{
    uint32_t size_left;
    bool all_data_added = true;

    if (safebuffer == NULL) {
        return -EFAULT;
    }
    size_left = safebuffer->len - safebuffer->write_index;
    if (size_left < len) {
        // Not all the data will fit in the buffer
        all_data_added = false;
        len = size_left;
        safebuffer->overflow = true;
    }
    memcpy(&safebuffer->buf[safebuffer->write_index], data, len);
    safebuffer->write_index += len;
    if (safebuffer->write_index >= safebuffer->len) {
        safebuffer->full = true;
    }
    if (all_data_added) {
        return 0;
    } else {
        return -ENOMEM;
    }
}

// Add data to a safe buffer from another safe buffer
int safebuffer_add_safebuffer(safebuffer_t *safebuffer, safebuffer_t *safebuffer_in) {
  return safebuffer_add_data(safebuffer, safebuffer_in->buf, safebuffer_in->write_index);
}

// Add a zero terminated string to a safe buffer
int safebuffer_add_string(safebuffer_t *safebuffer, uint8_t *p_str) {
    uint32_t size_left;
    bool all_data_added = true;

    if (safebuffer == NULL) {
        return -EFAULT;
    }
    size_left = safebuffer->len - safebuffer->write_index;
    while (*p_str != 0) {
        if(size_left) {
            size_left--;
            safebuffer->buf[safebuffer->write_index++] = *p_str;
        } else {
            safebuffer->overflow = true;
            all_data_added = false;
            break;
        }
        p_str++;
    }
    if (!size_left) {
        safebuffer->full = true;
    }
    if (all_data_added) {
        return 0;
    } else {
        return -ENOMEM;
    }
}

// Add character to a safe buffer. Return 0 if character could be added
int safebuffer_add_char(safebuffer_t *safebuffer, uint8_t c) {
    uint32_t size_left;
    bool all_data_added = true;

    if (safebuffer == NULL) {
        return -EFAULT;
    }
    size_left = safebuffer->len - safebuffer->write_index;
    if (!size_left) {
        //character will not fit in buffer
        all_data_added = false;
        safebuffer->overflow = true;
    } else {
        safebuffer->buf[safebuffer->write_index++] = c;
    }
    if (safebuffer->write_index >= safebuffer->len) {
        safebuffer->full = true;
    }
    if (all_data_added) {
        return 0;
    } else {
        return -ENOMEM;
    }
}

// Get the number of bytes still left in buffer
uint32_t safebuffer_get_data_left_count(safebuffer_t *safebuffer) {
    uint32_t size_left = 0;

    if (safebuffer == NULL) {
        return size_left;
    }
    size_left = safebuffer->len - safebuffer->write_index;
    return size_left;
}

// Write a printf style message to a safe buffer
int safebuffer_snprintf(safebuffer_t *safebuffer, const char* msg, ...) {
    va_list args;
    int32_t wsize;
    char *c;
    uint32_t size_left = 0;
    int error = 0;

    if (safebuffer == NULL) {
        return -EFAULT;
    }
    size_left = safebuffer->len - safebuffer->write_index - 1;      // Minus one for terminating zero
    va_start (args, msg);
    c = (char*)&safebuffer->buf[safebuffer->write_index];
    wsize = vsnprintf(c, size_left, msg, args);
    va_end (args);
    if (wsize > size_left) {
        // Not all the data was written
        safebuffer->write_index = safebuffer->len;
        safebuffer->overflow = true;
        safebuffer->full = true;
        error = -ENOMEM;
    } else if (wsize == size_left) {
        // Buffer was exactly the correct size
        safebuffer->write_index = safebuffer->len;
        safebuffer->overflow = false;
        safebuffer->full = true;
    } else {
        // Buffer still have space left
        safebuffer->write_index += wsize;  // Point to terminating zero so it can be overwritten
        safebuffer->overflow = false;
        safebuffer->full = false;
    }
    return error;
}

// Read a character from a safe buffer if available
int safebuffer_read_char(safebuffer_t *safebuffer, uint8_t *c) {
    if (safebuffer == NULL) {
        return -EFAULT;
    }
    if (safebuffer->read_index >= safebuffer->write_index) {
        // ReadIndex is not in buffer bounds
        return -ENOSPC;
    }
    *c = safebuffer->buf[safebuffer->read_index++];
    return 0;
}

// Reset all the variables for a safe buffer so it is empty
int safebuffer_reset(safebuffer_t *safebuffer) {
    if (safebuffer == NULL) {
        return -EFAULT;
    }
    safebuffer->full = false;
    safebuffer->overflow = false;
    safebuffer->write_index = 0;
    safebuffer->read_index = 0;
    return 0;
}

safebuffer_t *safebuffer_malloc(uint32_t len) {
    safebuffer_t *safebuffer = NULL;
    uint8_t *buf = NULL;

    if (len == 0) {
        return NULL;
    }
    buf = k_malloc(len);
    if (buf == NULL) {
        return NULL;
    }
    safebuffer = k_malloc(sizeof(safebuffer_t));
    if (safebuffer == NULL) {
        k_free(buf);
        return NULL;
    }
    safebuffer_init(safebuffer, buf, len);
    return safebuffer;
}

int safebuffer_free(safebuffer_t *safebuffer) {
    if (safebuffer == NULL) {
        return -EFAULT;
    }
    k_free(safebuffer->buf);
    k_free(safebuffer);
    return 0;
}
