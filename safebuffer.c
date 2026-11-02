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

// Number of bytes that can still be written. Users can change write_index directly, so it can be past the end.
static uint32_t space_left(const safebuffer_t *safebuffer) {
    if (safebuffer->write_index >= safebuffer->len) {
        return 0;
    }
    return safebuffer->len - safebuffer->write_index;
}

int safebuffer_init(safebuffer_t *safebuffer, uint8_t *buf, uint32_t len) {
    if (safebuffer == NULL) {
        return -EFAULT;
    }
    safebuffer->buf = buf;
    safebuffer->len = len;
    safebuffer->full = false;
    safebuffer->overflow = false;
    safebuffer->read_only = false;
    safebuffer->write_index = 0;
    safebuffer->read_index = 0;
    return 0;
}

// Initialise a safe buffer to point to a constant buffer. The buffer is read only, data can't be added to it.
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
    safebuffer->read_only = true;
    safebuffer->write_index = len;
    safebuffer->read_index = 0;
    return 0;
}

// Add data to a safe buffer. Return 0 if all data could be added
int safebuffer_add_data(safebuffer_t *safebuffer, const void *data, uint32_t len)
{
    uint32_t size_left;
    bool all_data_added = true;

    if (safebuffer == NULL) {
        return -EFAULT;
    }
    if (safebuffer->read_only) {
        return -EACCES;
    }
    if ((data == NULL) && len) {
        return -EINVAL;
    }
    size_left = space_left(safebuffer);
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
int safebuffer_add_safebuffer(safebuffer_t *safebuffer, const safebuffer_t *safebuffer_in) {
    uint32_t len;

    if (safebuffer_in == NULL) {
        return -EFAULT;
    }
    // Don't copy past the end of the input buffer if its write_index was changed directly
    len = (safebuffer_in->write_index < safebuffer_in->len) ? safebuffer_in->write_index : safebuffer_in->len;
    return safebuffer_add_data(safebuffer, safebuffer_in->buf, len);
}

// Add a zero terminated string to a safe buffer, the terminating zero is not added
int safebuffer_add_string(safebuffer_t *safebuffer, const char *str) {
    uint32_t size_left;
    bool all_data_added = true;

    if (safebuffer == NULL) {
        return -EFAULT;
    }
    if (safebuffer->read_only) {
        return -EACCES;
    }
    if (str == NULL) {
        return -EINVAL;
    }
    size_left = space_left(safebuffer);
    while (*str != 0) {
        if(size_left) {
            size_left--;
            safebuffer->buf[safebuffer->write_index++] = (uint8_t)*str;
        } else {
            safebuffer->overflow = true;
            all_data_added = false;
            break;
        }
        str++;
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
    if (safebuffer->read_only) {
        return -EACCES;
    }
    size_left = space_left(safebuffer);
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
uint32_t safebuffer_get_data_left_count(const safebuffer_t *safebuffer) {
    if (safebuffer == NULL) {
        return 0;
    }
    return space_left(safebuffer);
}

// Write a printf style message to a safe buffer. A terminating zero is written after the message, write_index points
// to it so the next data added overwrites it. The terminating zero needs a byte, so a message of n characters needs
// n + 1 bytes of space.
// Return 0 if the whole message was added, -ENOMEM if it was cut short. Then the buffer is full, it holds as much of
// the message as fits with the terminating zero in the last byte.
int safebuffer_snprintf(safebuffer_t *safebuffer, const char *msg, ...) {
    va_list args;
    int wsize;
    char *c;
    uint32_t size_left;

    if (safebuffer == NULL) {
        return -EFAULT;
    }
    if (safebuffer->read_only) {
        return -EACCES;
    }
    if (msg == NULL) {
        return -EINVAL;
    }
    size_left = space_left(safebuffer);
    va_start (args, msg);
    if (!size_left) {
        // No space left, not even for the terminating zero. Only check if the message would have added characters.
        wsize = vsnprintf(NULL, 0, msg, args);
        va_end (args);
        if (wsize < 0) {
            return -EINVAL;
        }
        if (wsize > 0) {
            safebuffer->overflow = true;
            return -ENOMEM;
        }
        return 0;
    }
    c = (char*)&safebuffer->buf[safebuffer->write_index];
    wsize = vsnprintf(c, size_left, msg, args);
    va_end (args);
    if (wsize < 0) {
        // Formatting error, nothing was added
        *c = 0;
        return -EINVAL;
    }
    if ((uint32_t)wsize >= size_left) {
        // Not all the message was written, vsnprintf wrote size_left - 1 characters and the terminating zero
        safebuffer->write_index = safebuffer->len;
        safebuffer->overflow = true;
        safebuffer->full = true;
        return -ENOMEM;
    }
    // All the message was written. Overflow is not cleared, so an earlier loss of data is not hidden.
    safebuffer->write_index += wsize;  // Point to terminating zero so it can be overwritten
    return 0;
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

// Reset all the variables for a safe buffer so it is empty. A read only buffer keeps its data, only reading starts
// from the beginning again.
int safebuffer_reset(safebuffer_t *safebuffer) {
    if (safebuffer == NULL) {
        return -EFAULT;
    }
    if (safebuffer->read_only) {
        safebuffer->read_index = 0;
        return 0;
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
