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

#include "config.h"
#include "swf.h"
#include "internal.h"
#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static inline int check_header(SWFParser *parser) {
    SWF *swf = parser->swf;
    return ((swf->compression != SWF_UNCOMPRESSED) &&
            (swf->compression != SWF_ZLIB) &&
            (swf->compression != SWF_LZMA)) ||
           get_8(&parser->buf) != 'W' ||
           get_8(&parser->buf) != 'S';
}

static SWFError setup_decompression(SWFParser *parser) {
    SWF* swf = parser->swf;
    int ret;
    switch (swf->compression) {
        case SWF_ZLIB:
#if HAVE_LIBZ
            switch ((ret = inflateInit(&parser->zstrm))) {
            case Z_OK:
                return SWF_OK;
            case Z_MEM_ERROR:
                return set_error(parser, SWF_NOMEM, "setup_decompression: inflateInit returned Z_MEM_ERROR");
            case Z_VERSION_ERROR:
            case Z_STREAM_ERROR:
                return set_error(parser, SWF_INTERNAL_ERROR, "setup_decompression: inflateInit returned an our-fault error");
            default:
                return set_error(parser, SWF_UNKNOWN, "setup_decompression: inflateInit returned an unknown error");
            }
#else
            return set_error(parser, SWF_RECOMPILE, "setup_decompression: ZLIB compression requires ZLIB");
#endif
        case SWF_LZMA:
            parser->state = PARSER_LZMA_HEADER;
            LzmaDec_Construct(&parser->lzma);
        default:
            return SWF_OK;
    }
}

static SWFError parse_swf_header(SWFParser *parser) {
    if (parser->buf.size < 8) {
        return SWF_NEED_MORE_DATA;
    }
    SWF* swf = parser->swf;

    swf->compression = get_8(&parser->buf);
    if (check_header(parser))
        return set_error(parser, SWF_INVALID, "parse_swf_header: check_header reported an invalid header");
    swf->version = get_8(&parser->buf);
    swf->size = get_32(&parser->buf);
    parser->state = PARSER_HEADER;
    if (parser->callbacks.header_cb) {
        parser->callbacks.header_cb(parser, NULL, parser->callbacks.ctx);
    }
    return setup_decompression(parser);
}

static SWFError parse_swf_rect(SWFParser *parser, SWFRect* rect) {
    Buffer *buf = &parser->buf;
    if (buf->size < 1)
        return SWF_NEED_MORE_DATA;
    int size = buf_get_bits(buf, 5);
    if (buf->size < (size * 4 + 5 + 7) >> 3)
        return SWF_NEED_MORE_DATA;
    rect->x_min = buf_get_sbits(buf, size);
    rect->x_max = buf_get_sbits(buf, size);
    rect->y_min = buf_get_sbits(buf, size);
    rect->y_max = buf_get_sbits(buf, size);
    buf_finish_bit_access(buf);
    return SWF_OK;
}

static SWFError parse_compressed_header(SWFParser *parser) {
    buf_clear_rollback(&parser->buf);
    SWFError ret = SWF_OK;
    SWF *swf = parser->swf;
    if ((ret = parse_swf_rect(parser, &swf->frame_size)) != SWF_OK)
        return ret;
    if (parser->buf.size < 4) {
        buf_rollback(&parser->buf);
        return SWF_NEED_MORE_DATA;
    }
    swf->frame_rate = get_16(&parser->buf);
    swf->frame_count = get_16(&parser->buf);
    parser->state = PARSER_BODY;
    if (parser->callbacks.header2_cb) {
        parser->callbacks.header2_cb(parser, NULL, parser->callbacks.ctx);
    }
    return SWF_OK;
}

SWFError swf_add_tag(SWF *swf, SWFTag *tag) {
    if (swf->nb_tags == swf->max_tags) {
        if (swf->max_tags == 0) {
            swf->max_tags = 16;
        }
        SWFTag *new_tags = realloc(swf->tags, swf->max_tags * 2 * sizeof(SWFTag));
        if (!new_tags)
            return set_error(swf, SWF_NOMEM, "swf_add_tag: Not enough memory to expand SWFTag array");
        swf->max_tags *= 2;
        swf->tags = new_tags;
    }
    memcpy(swf->tags + swf->nb_tags++, tag, sizeof(SWFTag));
    return SWF_OK;
}

static SWFError parse_payload(SWFParser *parser, SWFTag *tag) {
    if (!tag->size)
        // Short-circuit if the tag was just an ID (probably invalid)
        return SWF_OK;
    tag->payload = malloc(tag->size);
    if (!tag->payload)
        return set_error(parser, SWF_NOMEM, "parse_payload: malloc failed");
    memcpy(tag->payload, parser->buf.ptr, tag->size);
    buf_advance(&parser->buf, tag->size);
    return SWF_OK;
}

static SWFError parse_JPEG_tables(SWFParser *parser, SWFTag *tag) {
    SWF *swf = parser->swf;
    SWFError ret = parse_payload(parser, tag);
    if (ret != SWF_OK)
        return ret;
    uint8_t *tables = malloc(tag->size);
    if (!tables)
        return set_error(parser, SWF_NOMEM, "parse_JPEG_tables: malloc failed");
    memcpy(tables, tag->payload, tag->size);
    swf->JPEG_tables = tables;
    return SWF_OK;
}

static SWFError parse_id_payload(SWFParser *parser, SWFTag *tag) {
    // This function handles types that start with a 2-byte ID, then have
    // some complex payload that isn't worth parsing right now.
    tag->id = get_16(&parser->buf);
    tag->size -= 2;
    return parse_payload(parser, tag);
}

static SWFError parse_tag(SWFParser *parser) {
    buf_clear_rollback(&parser->buf);
    if (parser->buf.size < 2)
        return SWF_NEED_MORE_DATA;
    uint16_t code_and_length = get_16(&parser->buf);
    uint32_t len = code_and_length & 0x3F;
    uint16_t code = code_and_length >> 6;
    if (parser->buf.size < len) {
        buf_rollback(&parser->buf);
        return SWF_NEED_MORE_DATA;
    }
    if (len == 0x3F) {
        len = get_32(&parser->buf);
        if (len > parser->buf.size) {
            buf_rollback(&parser->buf);
            return SWF_NEED_MORE_DATA;
        }
    }
    SWFTag tag = {
        .type = code,
        .payload = NULL,
        .size = len,
        .id = 0,
    };
    SWFError ret = SWF_OK;
    switch (code) {
    case SWF_JPEG_TABLES:
        if ((ret = parse_JPEG_tables(parser, &tag))) {
            buf_rollback(&parser->buf);
            return ret;
        }
        break;
    case SWF_DEFINE_SHAPE:
    case SWF_DEFINE_BITS:
    case SWF_DEFINE_BUTTON:
    case SWF_DEFINE_FONT:
    case SWF_DEFINE_TEXT:
    case SWF_DEFINE_SOUND:
    case SWF_DEFINE_BITS_LOSSLESS:
    case SWF_DEFINE_BITS_JPEG_2:
    case SWF_DEFINE_SHAPE_2:
    case SWF_DEFINE_SHAPE_3:
    case SWF_DEFINE_TEXT_2:
    case SWF_DEFINE_BUTTON_2:
    case SWF_DEFINE_BITS_JPEG_3:
    case SWF_DEFINE_BITS_LOSSLESS_2:
    case SWF_DEFINE_EDIT_TEXT:
    case SWF_DEFINE_SPRITE:
    case SWF_DEFINE_MORPH_SHAPE:
    case SWF_DEFINE_FONT_2:
    case SWF_DEFINE_VIDEO_STREAM:
    case SWF_DEFINE_FONT_3:
    case SWF_DEFINE_SHAPE_4:
    case SWF_DEFINE_MORPH_SHAPE_2:
    case SWF_DEFINE_BITS_JPEG_4:
        if ((ret = parse_id_payload(parser, &tag))) {
            buf_rollback(&parser->buf);
            return ret;
        }
        break;
    case SWF_END:
        parser->state = PARSER_FINISHED;
        buf_advance(&parser->buf, len);
        if (parser->callbacks.end_cb) {
            parser->callbacks.end_cb(parser, NULL, parser->callbacks.ctx);
        }
        return SWF_FINISHED;
    default:
        if ((ret = parse_payload(parser, &tag))) {
            buf_rollback(&parser->buf);
            return ret;
        }
        break;
    }
    if (parser->callbacks.tag_cb) {
        return parser->callbacks.tag_cb(parser, &tag, parser->callbacks.ctx);
    }
    return copy_error(parser, parser->swf, swf_add_tag(parser->swf, &tag));
}

static SWFError parse_buf_inc(SWFParser *parser) {
    switch (parser->state) {
    case PARSER_HEADER:
        return parse_compressed_header(parser);
    case PARSER_BODY:
        return parse_tag(parser);
    case PARSER_FINISHED:
        return SWF_FINISHED;
    default:
        return set_error(parser, SWF_INVALID, "parse_buf_inc: Invalid SWFParser->state");
    }
}

static SWFError parse_buf(SWFParser *parser) {
    int was_ok = 0;
    SWFError ret = SWF_OK;
    while(ret == SWF_OK) {
        ret = parse_buf_inc(parser);
        if (ret == SWF_OK)
            was_ok = 1;
    }
    if (ret == SWF_NEED_MORE_DATA && was_ok)
        return SWF_OK;
    return ret;
}

SWFError swf_parser_append(SWFParser *parser, const void *buf_in, size_t len) {
    SWF *swf = parser->swf;
    const uint8_t *buf = buf_in;
    SWFError ret = SWF_OK;
    if (parser->state == PARSER_STARTED) {
        // This is a really stupid setup to make sure we don't crash
        // if the first few packets are <8 bytes
        size_t bytes_left = 8 - parser->buf.size;
        bytes_left = bytes_left > 8 ? 8 : bytes_left;
        bytes_left = len < bytes_left ? len : bytes_left;
        if (!parser->buf.alloc_ptr &&
           (ret = buf_init_with_data(&parser->buf, buf, bytes_left)))
            return copy_error(parser, &parser->buf, ret);
        if (parser->buf.size < 8)
            return SWF_OK;
        ret = parse_swf_header(parser);
        if (ret != SWF_OK)
            return ret;
        // This buf is really tiny, and probably not worth keeping
        buf_free(&parser->buf);
        buf += bytes_left;
        len -= bytes_left;
        // Fall through to the post-header parsing stage
    }
    if (parser->state == PARSER_LZMA_HEADER) {
#define LZMA_HEADER_SIZE (LZMA_PROPS_SIZE + 4)
        size_t bytes_left = LZMA_HEADER_SIZE - parser->buf.size;
        bytes_left = bytes_left > LZMA_HEADER_SIZE ? LZMA_HEADER_SIZE : bytes_left;
        bytes_left = len < bytes_left ? len : bytes_left;
        if (!parser->buf.alloc_ptr &&
           (ret = buf_init_with_data(&parser->buf, buf, bytes_left)))
            return copy_error(parser, &parser->buf, ret);
        if (parser->buf.size < LZMA_HEADER_SIZE)
            return SWF_OK;
        // Fall through to the post-header parsing stage
        SRes lz_ret = LzmaDec_Allocate(&parser->lzma, parser->buf.ptr + 4, LZMA_PROPS_SIZE, &allocator);
        // This buf is really tiny, and probably not worth keeping
        buf_free(&parser->buf);
        buf += bytes_left;
        len -= bytes_left;
        switch (lz_ret) {
        case SZ_OK:
            LzmaDec_Init(&parser->lzma);
            break;
        case SZ_ERROR_MEM:
            return set_error(parser, SWF_NOMEM, "setup_decompression: LzmaDec_Allocate returned SZ_ERROR_MEM");
        case SZ_ERROR_UNSUPPORTED:
            return set_error(parser, SWF_INTERNAL_ERROR, "setup_decompression: LzmaDec_Allocate returned SZ_ERROR_UNSUPPORTED");
        default:
            return set_error(parser, SWF_UNKNOWN, "setup_decompression: LzmaDec_Allocate returned an unknown error");
        }
        parser->state = PARSER_HEADER;
    }
    int increase_space = 0;
    switch (swf->compression) {
    case SWF_UNCOMPRESSED:
        if ((ret = buf_append(&parser->buf, buf, len)))
            return copy_error(parser, &parser->buf, ret);
        return parse_buf(parser);
#if HAVE_LIBZ
    case SWF_ZLIB:
        if (!parser->buf.alloc_ptr)
            // No buffer yet. Allocate one, wild-guessing at the size
            if ((ret = buf_init(&parser->buf, len * 4)))
                return copy_error(parser, &parser->buf, ret);
        parser->zstrm.avail_in = len;
        parser->zstrm.next_in = (uint8_t*)buf;
        for (;;) {
            size_t avail_size = buf_shift(&parser->buf);
            if (!avail_size) {
                if ((ret = buf_grow_by(&parser->buf, parser->zstrm.avail_in * 4)))
                    return copy_error(parser, &parser->buf, ret);
                avail_size = parser->buf.alloc_size - parser->buf.size;
            }
            parser->zstrm.avail_out = avail_size;
            parser->zstrm.next_out = parser->buf.ptr + parser->buf.size;
            int z_ret = inflate(&parser->zstrm, Z_NO_FLUSH);
            parser->buf.size += (avail_size - parser->zstrm.avail_out);
            switch (z_ret) {
            case Z_STREAM_END:
                return parse_buf(parser);
            case Z_STREAM_ERROR:
                return set_error(parser, SWF_INVALID, parser->zstrm.msg);
            case Z_DATA_ERROR:
                return set_error(parser, SWF_INTERNAL_ERROR, parser->zstrm.msg);
            case Z_MEM_ERROR:
                return set_error(parser, SWF_NOMEM, parser->zstrm.msg);
            case Z_BUF_ERROR:
                increase_space = 1;
            case Z_OK:
                // Continue looping
                break;
            default:
                return set_error(parser, SWF_UNKNOWN, parser->zstrm.msg);
            }
            switch ((ret = parse_buf(parser))) {
            case SWF_OK:
                break;
            case SWF_NEED_MORE_DATA:
                if (increase_space) {
                    if ((ret = buf_grow(&parser->buf, 2)))
                        return copy_error(parser, &parser->buf, ret);
                    increase_space = 0;
                }
                break;
            default:
                return ret;
            }
            if (!parser->zstrm.avail_in) {
                return ret;
            }
        }
#endif
    case SWF_LZMA:
        if (!parser->buf.alloc_ptr)
            // No buffer yet. Allocate one, wild-guessing at the size
            if ((ret = buf_init(&parser->buf, len * 4)))
                return copy_error(parser, &parser->buf, ret);
        size_t avail_in = len;
        const uint8_t *next_in = buf;
        for (;;) {
            size_t in_size = avail_in;
            size_t avail_size = buf_shift(&parser->buf);
            if (!avail_size) {
                if ((ret = buf_grow_by(&parser->buf, avail_in * 4)))
                    return copy_error(parser, &parser->buf, ret);
                avail_size = parser->buf.alloc_size - parser->buf.size;
            }
            size_t avail_out = avail_size;
            uint8_t *next_out = parser->buf.ptr + parser->buf.size;
            ELzmaFinishMode finishMode = LZMA_FINISH_ANY;
            ELzmaStatus status;
            SRes lz_ret = LzmaDec_DecodeToBuf(&parser->lzma, next_out, &avail_out,
                                              next_in, &avail_in, finishMode, &status);
            parser->buf.size += avail_out;
            next_in += avail_in;
            avail_in = in_size - avail_in;
            switch (lz_ret) {
            case SZ_ERROR_OUTPUT_EOF:
                return parse_buf(parser);
            case SZ_ERROR_CRC:
                return set_error(parser, SWF_INVALID, "CRC error in LzmaDec_DecodeToBuf");
            case SZ_ERROR_DATA:
                return set_error(parser, SWF_INVALID, "Data error in LzmaDec_DecodeToBuf");
            case SZ_ERROR_MEM:
                return set_error(parser, SWF_NOMEM, "Memory allocation error in LzmaDec_DecodeToBuf");
            case Z_BUF_ERROR:
                increase_space = 1;
            case SZ_OK:
                // Continue looping
                break;
            default:
                return set_error(parser, SWF_UNKNOWN, "Unknown error in LzmaDec_DecodeToBuf");
            }
            switch ((ret = parse_buf(parser))) {
            case SWF_OK:
                break;
            case SWF_NEED_MORE_DATA:
                if (increase_space) {
                    if ((ret = buf_grow(&parser->buf, 2))) {
                        return copy_error(parser, &parser->buf, ret);
                    }
                    increase_space = 0;
                }
                break;
            default:
                return ret;
            }
            if (!avail_in) {
                return ret;
            }
            fprintf(stderr, "LOOP ENDING: %i\n", (int)avail_in);
        }
    default:
        return set_error(parser, SWF_UNKNOWN, "swf_parser_append: unknown compression method");
    }
}

SWFParser* swf_parser_init(void) {
    SWFParser *out = calloc(1, sizeof(SWFParser));
    if (!out)
        return NULL;
    out->swf = swf_init();
    if (!out->swf) {
        free(out);
        return NULL;
    }
    return out;
}

SWF* swf_parser_get_swf(SWFParser *parser) {
    return parser->swf;
}

SWFErrorDesc *swf_parser_get_error(SWFParser *parser) {
    return &parser->err;
}

void swf_parser_free(SWFParser *parser) {
    buf_free(&parser->buf);
    switch (parser->swf->compression) {
#if HAVE_ZLIB
    case SWF_ZLIB:
        inflateEnd(&parser->zstrm);
#endif
    case SWF_LZMA:
        LzmaDec_Free(&parser->lzma, &allocator);
    default:
        break;
    }
    free(parser);
}

void swf_parser_set_callbacks(SWFParser *parser, SWFParserCallbacks *callbacks) {
    parser->callbacks.tag_cb = callbacks->tag_cb;
    parser->callbacks.header_cb = callbacks->header_cb;
    parser->callbacks.header2_cb = callbacks->header2_cb;
    parser->callbacks.end_cb = callbacks->end_cb;
    parser->callbacks.ctx = callbacks->ctx;
}
