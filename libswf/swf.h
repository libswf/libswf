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

#include <inttypes.h>
#include <stddef.h>

typedef enum {
    SWF_OK = 0,
    SWF_NEED_MORE_DATA = 1,
    SWF_FINISHED = 2,
    SWF_INVALID = -127,
    SWF_UNIMPLEMENTED,
    SWF_UNKNOWN,
    SWF_INTERNAL_ERROR,
    SWF_NOMEM
} SWFError;

typedef enum {
    SWF_END                     = 0,
    SWF_SHOW_FRAME              = 1,
    SWF_DEFINE_SHAPE            = 2,
    SWF_PLACE_OBJECT            = 4,
    SWF_REMOVE_OBJECT           = 5,
    SWF_DEFINE_BITS             = 6,
    SWF_DEFINE_BUTTON           = 7,
    SWF_JPEG_TABLES             = 8,
    SWF_SET_BACKGROUND_COLOR    = 9,
    SWF_DEFINE_FONT             = 10,
    SWF_DEFINE_TEXT             = 11,
    SWF_DO_ACTION               = 12,
    SWF_DEFINE_FONT_INFO        = 13,
    SWF_DEFINE_SOUND            = 14,
    SWF_START_SOUND             = 15,
    SWF_DEFINE_BUTTON_SOUND     = 17,
    SWF_SOUND_STREAM_HEAD       = 18,
    SWF_SOUND_STREAM_BLOCK      = 19,
    SWF_DEFINE_BITS_LOSSLESS    = 20,
    SWF_DEFINE_BITS_JPEG_2      = 21,
    SWF_DEFINE_SHAPE_2          = 22,
    SWF_DEFINE_BUTTON_CXFORM    = 23,
    SWF_PROTECT                 = 24,
    SWF_PLACE_OBJECT_2          = 26,
    SWF_REMOVE_OBJECT_2         = 28,
    SWF_DEFINE_SHAPE_3          = 32,
    SWF_DEFINE_TEXT_2           = 33,
    SWF_DEFINE_BUTTON_2         = 34,
    SWF_DEFINE_BITS_JPEG_3      = 35,
    SWF_DEFINE_BITS_LOSSLESS_2  = 36,
    SWF_DEFINE_EDIT_TEXT        = 37,
    SWF_DEFINE_SPRITE           = 39,
    SWF_FRAME_LABEL             = 43,
    SWF_SOUND_STREAM_HEAD_2     = 45,
    SWF_DEFINE_MORPH_SHAPE      = 46,
    SWF_DEFINE_FONT_2           = 48,
    SWF_EXPORT_ASSETS           = 56,
    SWF_IMPORT_ASSETS           = 57,
    SWF_ENABLE_DEBUGGER         = 58,
    SWF_DO_INIT_ACTION          = 59,
    SWF_DEFINE_VIDEO_STREAM     = 60,
    SWF_VIDEO_FRAME             = 61,
    SWF_DEFINE_FONT_INFO_2      = 62,
    SWF_ENABLE_DEBUGGER_2       = 64,
    SWF_SCRIPT_LIMITS           = 65,
    SWF_SET_TAB_INDEX           = 66,
    SWF_FILE_ATTRIBUTES         = 69,
    SWF_PLACE_OBJECT_3          = 70,
    SWF_IMPORT_ASSETS_2         = 71,
    SWF_DEFINE_FONT_ALIGN_ZONES = 73,
    SWF_CSM_TEXT_SETTINGS       = 74,
    SWF_DEFINE_FONT_3           = 75,
    SWF_SYMBOL_CLASS            = 76,
    SWF_METADATA                = 77,
    SWF_DEFINE_SCALING_GRID     = 78,
    SWF_DO_ABC                  = 82,
    SWF_DEFINE_SHAPE_4          = 83,
    SWF_DEFINE_MORPH_SHAPE_2    = 84,
    SWF_DEFINE_SCENE_AND_FRAME_LABEL_DATA = 86,
    SWF_DEFINE_BINARY_DATA      = 87,
    SWF_DEFINE_FONT_NAME        = 88,
    SWF_START_SOUND_2           = 89,
    SWF_DEFINE_BITS_JPEG_4      = 90,
    SWF_DEFINE_FONT_4           = 91,
    SWF_ENABLE_TELEMETRY        = 93
} SWFTagType;

typedef enum {
    SWF_UNCOMPRESSED    = 'F',
    SWF_ZLIB            = 'C',
    SWF_LZMA            = 'Z'
} SWFCompression;

typedef struct {
    SWFTagType type;
    size_t size;
    void *payload;
    uint16_t id;
} SWFTag;

typedef struct {
    uint16_t id;
    uint16_t frame_count;
    SWFTag* tags;
    int nb_tags;
} SWFSprite;

typedef struct {
    int32_t x_min;
    int32_t x_max;
    int32_t y_min;
    int32_t y_max;
} SWFRect;

typedef struct {
    SWFTag *tags;           // Pointer to array of Tags
    int nb_tags;            // Number of SWFTags
    int max_tags;

    SWFCompression compression; // What type of compression should we use?

    // Sprite data is provided for convenience when decoding.
    // When encoding, it is ignored.

    SWFSprite **sprites;    // Pointer to array of Sprite pointers; null if none
    int num_sprites;        // Number of SWFSprites
    int version;            // SWF version
    uint32_t size;          // File length in bytes, decompressed
                            // This is set by the library during both parsing
                            // and writing.
    SWFRect frame_size;     // Frame size in twips; min_x and min_y are ignored
    uint16_t frame_rate;    // Frame delay in 8.8 fixed-point
    uint16_t frame_count;   // Number of frames in file

    uint8_t *JPEG_tables;   // JPEG tables used by DefineBits tags
} SWF;

typedef SWFError (*SWFParserCallback)(SWF*, void*, void*);

typedef struct {
    SWFParserCallback tag_cb;
    SWFParserCallback header_cb;
    SWFParserCallback end_cb;
    void* priv;
} SWFParserCallbacks;

typedef struct SWF_Parser SWFParser;

SWFParser* swf_parser_init(void);
SWFError swf_parser_append(SWFParser *parser, const uint8_t *buf, size_t len);
SWF* swf_parser_get_swf(SWFParser *parser);
SWF* swf_init(void);
