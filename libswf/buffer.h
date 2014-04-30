/*
 * Copyright (C) 2014 Rodger Combs <rodger.combs@gmail.com>
 *
 * This file is part of libswf.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#pragma once

#include "internal.h"
#include <string.h>
#include <stdlib.h>

typedef struct {
    SWFErrorDesc err;
    uint8_t *ptr;
    uint8_t *alloc_ptr;
    size_t size;
    size_t alloc_size;
    size_t rollback;
    int index;
} Buffer;

static inline void buf_free(Buffer *buffer){
    if(buffer->alloc_ptr){
        free(buffer->alloc_ptr);
    }
    memset(buffer, 0, sizeof(Buffer));
}

static inline SWFError buf_init(Buffer *buffer, size_t size){
    buf_free(buffer);
    uint8_t *new_buf = malloc(size);
    if(!new_buf)
        return SWF_NOMEM;
    buffer->alloc_ptr = buffer->ptr = new_buf;
    buffer->alloc_size = size;
    buffer->size = 0;
    buffer->index = 0;
    return SWF_OK;
}

static inline uint64_t buf_get_bits(Buffer *buf, unsigned nb_bits){
    uint64_t tmp = 0;
    assert(nb_bits <= 56);
    if(nb_bits == 0)
        return 0;
    unsigned bytes = buf->index >> 3,
             bits = buf->index & 7,
             read_bytes = (nb_bits >> 3) + 2,
             inv_read = 8 - read_bytes;
    for(unsigned i = 7; i >= inv_read; --i){
        if(i == inv_read && (nb_bits & 7) <= 8 - bits)
            break;
        tmp |= (int64_t)buf->ptr[bytes++] << (i << 3);
    }

    buf->index += nb_bits;

    return (tmp << bits) >> (64 - nb_bits);
}

static inline int64_t buf_get_sbits(Buffer *buf, int nb_bits){
    uint64_t tmp = buf_get_bits(buf, nb_bits);
    if(tmp >> (nb_bits - 1))
        tmp |= (0xFFFFFFFFFFFFFFFF >> nb_bits) << nb_bits;
    return (int64_t)tmp;
}

static inline void buf_append_raw(Buffer *buffer, const uint8_t *add, size_t size){
    memcpy(buffer->ptr + buffer->size, add, size);
    buffer->size += size;
}

static inline SWFError buf_init_with_data(Buffer *buffer, const uint8_t *data, size_t size){
    SWFError ret = SWF_OK;
    if((ret = buf_init(buffer, size)))
        return ret;
    buf_append_raw(buffer, data, size);
    return SWF_OK;
}

static inline SWFError buf_grow_to(Buffer *buffer, size_t size){
    if(size < buffer->size)
        return set_error(buffer, SWF_INTERNAL_ERROR, "buf_grow_to: size < buffer->size");
    uint8_t *new_buf = malloc(size);
    if(!new_buf)
        return set_error(buffer, SWF_NOMEM, "buf_grow_to: malloc failed");
    memcpy(new_buf, buffer->ptr, buffer->size);
    free(buffer->alloc_ptr);
    buffer->alloc_ptr = buffer->ptr = new_buf;
    buffer->alloc_size = size;
    return SWF_OK;
}

static inline SWFError buf_grow_by(Buffer *buffer, size_t add){
    return buf_grow_to(buffer, buffer->size + add);
}

/**
 * \brief Shifts a buffer to not have unused space at the front.
 * \param buffer[in] Buffer to shift
 * \return number of free bytes in buffer
 */
static inline size_t buf_shift(Buffer *buffer){
    if(buffer->ptr > buffer->alloc_ptr){
        if(buffer->size)
            memmove(buffer->alloc_ptr, buffer->ptr, buffer->size);
        buffer->ptr = buffer->alloc_ptr;
    }
    return buffer->alloc_size - buffer->size;
}

static inline SWFError buf_append(Buffer *buffer, const uint8_t *add, size_t size){
    SWFError ret = SWF_OK;
    if(!buffer->alloc_ptr)
        if((ret = buf_init(buffer, size)))
            return ret;
    uint8_t *end = buffer->alloc_ptr + buffer->alloc_size;
    if(buffer->ptr + buffer->size + size <= end){
        buf_append_raw(buffer, add, size);
        return SWF_OK;
    }
    if(buffer->alloc_ptr + buffer->size + size <= end){
        // We have a buffer, and our new data will fit after shuffling.
        buf_shift(buffer);
        buf_append_raw(buffer, add, size);
        return SWF_OK;
    }
    // We have a buffer, but it's not big enough. Allocate a new one.
    if((ret = buf_grow_by(buffer, size)))
        return ret;
    buf_append_raw(buffer, add, size);
    return SWF_OK;
}

static inline void buf_clear(Buffer *buffer){
    buffer->ptr = buffer->alloc_ptr;
    buffer->size = 0;
}

static inline void buf_advance(Buffer *buffer, int bytes){
    buffer->ptr += bytes;
    buffer->size -= bytes;
    buffer->rollback += bytes;
}

static inline void buf_finish_bit_access(Buffer *buf){
    buf_advance(buf, (buf->index + 7) >> 3);
    buf->index = 0;
}

static inline SWFError buf_grow(Buffer *buffer, unsigned factor){
    if(!factor)
        return SWF_INTERNAL_ERROR;
    return buf_grow_to(buffer, buffer->alloc_size * factor);
}

static inline void buf_clear_rollback(Buffer *buffer){
    buffer->rollback = 0;
}

static inline void buf_rollback(Buffer *buffer){
    buf_advance(buffer, -buffer->rollback);
}

static inline uint8_t get_8(Buffer *buffer){
    uint8_t tmp = buffer->ptr[0];
    buf_advance(buffer, 1);
    return tmp;
}

static inline uint16_t get_16(Buffer *buffer){
    uint16_t tmp = read_16(buffer->ptr);
    buf_advance(buffer, 2);
    return tmp;
}

static inline uint32_t get_32(Buffer *buffer){
    uint32_t tmp = read_32(buffer->ptr);
    buf_advance(buffer, 4);
    return tmp;
}
