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

static inline void advance_buf(SWFParser *parser, int bytes){
    parser->buf += bytes;
    parser->buf_size -= bytes;
    parser->last_advance += bytes;
}

static inline void rollback_buf(SWFParser *parser){
    advance_buf(parser, -parser->last_advance);
}

static inline uint8_t get_8(SWFParser *parser){
    uint8_t tmp = parser->buf[0];
    advance_buf(parser, 1);
    return tmp;
}

static inline uint16_t get_16(SWFParser *parser){
    uint16_t tmp = read_16(parser->buf);
    advance_buf(parser, 2);
    return tmp;
}

static inline uint32_t get_32(SWFParser *parser){
    uint32_t tmp = read_32(parser->buf);
    advance_buf(parser, 4);
    return tmp;
}

static inline int check_header(SWFParser *parser){
    SWF *swf = parser->swf;
    return ((swf->compression != SWF_UNCOMPRESSED) &&
#ifdef HAVE_LIBZ
            (swf->compression != SWF_ZLIB) &&
#endif
            (swf->compression != SWF_LZMA)) ||
           get_8(parser) != 'W' ||
           get_8(parser) != 'S';
}

static SWFError setup_decompression(SWFParser *parser){
    SWF* swf = parser->swf;
    int ret;
    switch(swf->compression){
        case SWF_ZLIB:
#ifdef HAVE_LIBZ
            ret = inflateInit(&parser->zstrm);
            switch(ret){
                case Z_OK:
                    return SWF_OK;
                case Z_MEM_ERROR:
                    return SWF_NOMEM;
                case Z_VERSION_ERROR:
                case Z_STREAM_ERROR:
                    return SWF_INTERNAL_ERROR;
            }
#else
            return SWF_RECOMPILE;
#endif
        case SWF_LZMA:
            return SWF_UNIMPLEMENTED;
            /*
            LzmaDec_Construct(&parser->lzma);
            ret = LzmaDec_Allocate(&parser->lzma, header, LZMA_PROPS_SIZE, &g_Alloc);
            if (ret != SZ_OK)
                return ret; */
        default:
            return SWF_OK;
    }
}

static SWFError parse_swf_header(SWFParser *parser){
    if(parser->buf_size < 8){
        return SWF_NEED_MORE_DATA;
    }
    SWF* swf = parser->swf;

    swf->compression = get_8(parser);
    if(check_header(parser))
        return SWF_INVALID;
    swf->version = get_8(parser);
    swf->size = get_32(parser);
    parser->state = PARSER_HEADER;
    if(parser->callbacks.header_cb){
        parser->callbacks.header_cb(swf, NULL, parser->callbacks.priv);
    }
    parser->state = PARSER_HEADER;
    return setup_decompression(parser);
}

static SWFError parse_swf_rect(SWFParser *parser, SWFRect* rect){
    SWFBitPointer ptr = init_bit_pointer(parser->buf, 0);
    if(parser->buf_size < 1)
        return SWF_NEED_MORE_DATA;
    int size = get_bits(&ptr, 5),
        total_size = (size * 4 + 5 + 7) >> 3;
    if(parser->buf_size < total_size)
        return SWF_NEED_MORE_DATA;
    rect->x_min = get_sbits(&ptr, size);
    rect->x_max = get_sbits(&ptr, size);
    rect->y_min = get_sbits(&ptr, size);
    rect->y_max = get_sbits(&ptr, size);
    advance_buf(parser, total_size);
    return SWF_OK;
}

static SWFError parse_swf_compressed_header(SWFParser *parser){
    parser->last_advance = 0;
    SWFError ret = SWF_OK;
    SWF *swf = parser->swf;
    if((ret = parse_swf_rect(parser, &swf->frame_size)) != SWF_OK)
        return ret;
    if(parser->buf_size < 4){
        rollback_buf(parser);
        return SWF_NEED_MORE_DATA;
    }
    swf->frame_rate = get_16(parser);
    swf->frame_count = get_16(parser);
    parser->state = PARSER_BODY;
    return SWF_OK;
}

static SWFError add_tag(SWFParser *parser, SWFTag *tag){
    SWF *swf = parser->swf;
    if(swf->nb_tags == swf->max_tags){
        if(swf->max_tags == 0){
            swf->max_tags = 16;
        }
        SWFTag *new_tags = realloc(swf->tags, swf->max_tags * 2 * sizeof(SWFTag));
        if(!new_tags)
            return SWF_NOMEM;
        swf->max_tags *= 2;
        swf->tags = new_tags;
    }
    memcpy(swf->tags + swf->nb_tags++, tag, sizeof(SWFTag));
    return SWF_OK;
}

static SWFError parse_payload(SWFParser *parser, SWFTag *tag){
    if(!tag->size){
        // Short-circuit if the tag was just an ID (probably invalid)
        return SWF_OK;
    }
    tag->payload = malloc(tag->size);
    if(!tag->payload)
        return SWF_NOMEM;
    memcpy(tag->payload, parser->buf, tag->size);
    advance_buf(parser, tag->size);
    return SWF_OK;
}

static SWFError parse_JPEG_tables(SWFParser *parser, SWFTag *tag){
    SWF *swf = parser->swf;
    SWFError ret = parse_payload(parser, tag);
    if(ret != SWF_OK)
        return ret;
    uint8_t *tables = malloc(tag->size);
    if(!tables)
        return SWF_NOMEM;
    memcpy(tables, tag->payload, tag->size);
    swf->JPEG_tables = tables;
    return SWF_OK;
}

static SWFError parse_id_payload(SWFParser *parser, SWFTag *tag){
    // This function handles types that start with a 2-byte ID, then have
    // some complex payload that isn't worth parsing right now.
    tag->id = get_16(parser);
    tag->size -= 2;
    return parse_payload(parser, tag);
}

static SWFError parse_swf_tag(SWFParser *parser){
    parser->last_advance = 0;
    SWF *swf = parser->swf;
    if(parser->buf_size < 2)
        return SWF_NEED_MORE_DATA;
    uint16_t code_and_length = get_16(parser);
    uint32_t len = code_and_length & 0x3F;
    uint16_t code = code_and_length >> 6;
    if(parser->buf_size < len){
        rollback_buf(parser);
        return SWF_NEED_MORE_DATA;
    }
    if(len == 0x3F){
        len = get_32(parser);
        if(len > parser->buf_size){
            rollback_buf(parser);
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
    switch(code){
    case SWF_JPEG_TABLES:
        if((ret = parse_JPEG_tables(parser, &tag))){
            rollback_buf(parser);
            return ret;
        }
        break;
    case SWF_DEFINE_BITS:
    case SWF_DEFINE_BITS_JPEG_2:
    case SWF_DEFINE_BITS_JPEG_3:
    case SWF_DEFINE_BITS_JPEG_4:
    case SWF_DEFINE_BITS_LOSSLESS:
    case SWF_DEFINE_BITS_LOSSLESS_2:
        if((ret = parse_id_payload(parser, &tag))){
            rollback_buf(parser);
            return ret;
        }
        break;
    case SWF_END:
        parser->state = PARSER_FINISHED;
        advance_buf(parser, len);
        if(parser->callbacks.end_cb){
            parser->callbacks.end_cb(swf, NULL, parser->callbacks.priv);
        }
        break;
    default:
        if((ret = parse_payload(parser, &tag))){
            rollback_buf(parser);
            return ret;
        }
        break;
    }
    if(parser->callbacks.tag_cb){
        return parser->callbacks.tag_cb(swf, &tag, parser->callbacks.priv);
    }
    return add_tag(parser, &tag);
}

static SWFError parse_buf_inc(SWFParser *parser){
    switch(parser->state){
    case PARSER_HEADER:
        return parse_swf_compressed_header(parser);
    case PARSER_BODY:
        return parse_swf_tag(parser);
    case PARSER_FINISHED:
        return SWF_FINISHED;
    default:
        return SWF_INVALID;
    }
}

static SWFError parse_buf(SWFParser *parser){
    int was_ok = 0;
    SWFError ret = SWF_OK;
    while(ret == SWF_OK){
        ret = parse_buf_inc(parser);
        if(ret == SWF_OK)
            was_ok = 1;
    }
    if(ret == SWF_NEED_MORE_DATA && was_ok)
        return SWF_OK;
    return ret;
}

SWFError swf_parser_append(SWFParser *parser, const uint8_t *buf, size_t len){
    SWF *swf = parser->swf;
    if(parser->state == PARSER_STARTED){
        if(!parser->buf_ptr){
            parser->buf_ptr = parser->buf = malloc(8);
            parser->buf_size_total = 8;
            parser->buf_size = 0;
        }
        // This is a really stupid setup to make sure we don't crash
        // if the first few packets are <8 bytes
        uint8_t bytes_left = 8 - parser->buf_size;
        bytes_left = bytes_left > 8 ? 8 : bytes_left;
        bytes_left = len < bytes_left ? len : bytes_left;
        memcpy(parser->buf + parser->buf_size, buf, bytes_left);
        parser->buf_size += bytes_left;
        if(parser->buf_size < 8)
            return SWF_OK;
        SWFError ret = parse_swf_header(parser);
        if(ret != SWF_OK)
            return ret;
        buf += bytes_left;
        len -= bytes_left;
        free(parser->buf_ptr);
        parser->buf_ptr = parser->buf = NULL;
        parser->buf_size = parser->buf_size_total = 0;
    }
    switch(swf->compression){
    case SWF_UNCOMPRESSED:
        if(!parser->buf_ptr){
            // No buffer yet. Allocate one, and fill it.
            parser->buf = parser->buf_ptr = malloc(len);
            if(!parser->buf_ptr)
                return SWF_NOMEM;
            parser->buf_size = parser->buf_size_total = len;
            memcpy(parser->buf, buf, len);
            return parse_buf(parser);
        }
        uint8_t *end = parser->buf_ptr + parser->buf_size_total;
        if(parser->buf + parser->buf_size + len < end){
            // We have a buffer, and our new data fits in the free space.
            memcpy(parser->buf + parser->buf_size, buf, len);
            parser->buf_size += len;
            return parse_buf(parser);
        }
        if(parser->buf_ptr + parser->buf_size + len < end){
            // We have a buffer, and our new data will fit after shuffling.
            memmove(parser->buf_ptr, parser->buf, parser->buf_size);
            parser->buf = parser->buf_ptr;
            memcpy(parser->buf + parser->buf_size, buf, len);
            parser->buf_size += len;
            return parse_buf(parser);
        }
        // We have a buffer, but it's not big enough. Allocate a new one.
        uint8_t *new_buf = malloc(parser->buf_size + len);
        if(!new_buf)
            return SWF_NOMEM;
        memcpy(new_buf, parser->buf, parser->buf_size);
        memcpy(new_buf + parser->buf_size, buf, len);
        free(parser->buf_ptr);
        parser->buf_size_total = parser->buf_size += len;
        parser->buf_ptr = parser->buf = new_buf;
        return parse_buf(parser);
    case SWF_ZLIB:
        if(!parser->buf_ptr){
            // No buffer yet. Allocate one, wild-guessing at the size
            parser->buf = parser->buf_ptr = malloc(len * 4);
            if(!parser->buf_ptr)
                return SWF_NOMEM;
            parser->buf_size_total = len * 4;
            parser->buf_size = 0;
        }
        int increase_space = 0;
        SWFError ret = SWF_OK;
        parser->zstrm.avail_in = len;
        parser->zstrm.next_in = (uint8_t*)buf;
        do{
            if(parser->buf > parser->buf_ptr){
                if(parser->buf_size)
                    memmove(parser->buf_ptr, parser->buf, parser->buf_size);
                parser->buf = parser->buf_ptr;
            }
            size_t avail_size = parser->buf_size_total - parser->buf_size;
            parser->zstrm.avail_out = avail_size;
            parser->zstrm.next_out = parser->buf + parser->buf_size;
            int z_ret = inflate(&parser->zstrm, Z_NO_FLUSH);
            parser->buf_size += (avail_size - parser->zstrm.avail_out);
            switch(z_ret){
            case Z_STREAM_END:
                return parse_buf(parser);
            case Z_DATA_ERROR:
                return SWF_INVALID;
            case Z_STREAM_ERROR:
                return SWF_INTERNAL_ERROR;
            case Z_MEM_ERROR:
                return SWF_NOMEM;
            case Z_BUF_ERROR:
                increase_space = 1;
            case Z_OK:
                // Continue looping
                break;
            default:
                return SWF_UNKNOWN;
            }
            switch(parse_buf(parser)){
            case SWF_OK:
                break;
            case SWF_NEED_MORE_DATA:
                if(increase_space){
                    uint8_t *new_buf = malloc(parser->buf_size_total * 2);
                    if(!new_buf)
                        return SWF_NOMEM;
                    parser->buf_size_total *= 2;
                    memcpy(new_buf, parser->buf, parser->buf_size);
                    free(parser->buf_ptr);
                    parser->buf = parser->buf_ptr = new_buf;
                    increase_space = 0;
                    continue;
                }
            default:
                return ret;
            }
            if(!parser->zstrm.avail_in){
                return ret;
            }
        }while(1);
    default:
        return SWF_UNIMPLEMENTED;
    }
}

SWFParser* swf_parser_init(void){
    SWFParser *out = calloc(1, sizeof(SWFParser));
    if(!out)
        return NULL;
    out->swf = swf_init();
    if(!out->swf){
        free(out);
        return NULL;
    }
    return out;
}

SWF* swf_parser_get_swf(SWFParser *parser){
    return parser->swf;
}

void swf_parser_free(SWFParser *parser){
    if(parser->buf_ptr){
        free(parser->buf_ptr);
        parser->buf_ptr = NULL;
        parser->buf = NULL;
    }
    if(parser->swf->compression == SWF_ZLIB){
        inflateEnd(&parser->zstrm);
    }
}
