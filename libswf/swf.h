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

/**
 * \file
 * Defines all externally-accessible types and functions for libswf
 * \publicsection
 */

/**
 * \mainpage
 * \ref swf.h You probably want the main header
 */

#pragma once

#include <inttypes.h>
#include <stddef.h>

/**
 * \brief Errors and other return values from libswf functions.
 * 0 is a simple "OK", values >0 are non-failures that include additional
 * information, and values <0 are failures.
 */
typedef enum {
    SWF_OK = 0,             ///< Success; nothing special to report
    SWF_NEED_MORE_DATA = 1, ///< More data is required to finish an attempted operation
    SWF_FINISHED = 2,       ///< We're completely finished with a multi-call operation
    SWF_INVALID = -127,     ///< Invalid data was provided
    SWF_UNIMPLEMENTED,      ///< A feature you're trying to use is unimplemented
    SWF_UNKNOWN,            ///< Some error occurred, and we're not sure of the specifics
    SWF_INTERNAL_ERROR,     ///< This means there's a bug in libswf. Patches welcome.
    SWF_NOMEM,              ///< Not enough memory was available to complete the operation.
                            ///< You should be able to retry the operation after freeing some.
    SWF_RECOMPILE           ///< You attempted to use a feature that requires an external
                            ///< library that this libswf was not built with.
} SWFError;

/**
 * \brief Description of the last error that occurred within a SWF, SWFParser, or SWFWriter
 */
typedef struct {
    SWFError code;      ///< SWFError code
    const char *text;   ///< Static string with a human-readable error description
                        /// MUST NOT be free'd
} SWFErrorDesc;

/**
 * \brief SWF tag types
 * \see http://wwwimages.adobe.com/www.adobe.com/content/dam/Adobe/en/devnet/swf/pdf/swf-file-format-spec.pdf
 */
typedef enum {
    SWF_END                     = 0, ///< This tag must end every SWF file. We stop parsing when we see it.
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

/**
 * \brief Compression methods used in SWF files.
 * The first 8 bytes of every SWF file are always uncompressed. The first 3
 * are a signature, and the first byte is an ASCII character representing the
 * compression type used by the rest of the file.
 * \see http://wwwimages.adobe.com/www.adobe.com/content/dam/Adobe/en/devnet/swf/pdf/swf-file-format-spec.pdf
 */
typedef enum {
    SWF_UNCOMPRESSED    = 'F', ///< No compression.
    SWF_ZLIB            = 'C', ///< Zlib DEFLATE compression (requires zlib).
    SWF_LZMA            = 'Z', ///< LZMA compression (builtin to libswf).
} SWFCompression;

/**
 * \brief SWF tag structure
 */
typedef struct {
    SWFTagType type;    ///< Tag type
    uint32_t size;      ///< Size of payload (NOT the total size of the tag in-file)
    uint8_t *payload;   ///< Pointer to a buffer containing the contents of the tag
    uint16_t id;        ///< 16-bit ID pulled from tag; 0 indicates no ID. This
                        ///< value is not included in the payload.
} SWFTag;

/**
 * \brief Parsed SWF sprite
 */
typedef struct {
    uint16_t frame_count;   ///< Number of frames in sprite
    SWFTag* tags;           ///< Pointer to array of sub-tags
    unsigned nb_tags;       ///< Number of SWFTags
    unsigned max_tags;      ///< Number of tags that can fit in the space allocated
} SWFSprite;

/**
 * \brief Parsed SWF rectangle
 */
typedef struct {
    int32_t x_min;
    int32_t x_max;
    int32_t y_min;
    int32_t y_max;
} SWFRect;

/**
 * \brief SWF data, either parsed from a file or to be written to one
 */
typedef struct {
    SWFErrorDesc err;       ///< Last error that occurred on this SWF
    SWFTag *tags;           ///< Pointer to array of Tags.
                            ///< This should be considered read-only to the user.
    unsigned nb_tags;       ///< Number of SWFTags
                            ///< This should be considered read-only to the user.
    unsigned max_tags;      ///< \protected Number of tags that can fit in the space allocated

    SWFCompression compression; ///< Type of compression used in file.
                                ///< For input, this is set by the parser, and MUST NOT
                                ///< be changed. For output, this is set by the consumer,
                                ///< and 0 is taken to mean "best available for this SWF version".
    uint8_t version;        ///< SWF version. Defaults to latest if not set when writing.
    uint32_t size;          ///< File length in bytes, decompressed. Read-only.
                            ///< This is set by the library during both parsing
                            ///< and writing.
    SWFRect frame_size;     ///< Frame size in twips; min_x and min_y are ignored.
    uint16_t frame_rate;    ///< Frame delay in 8.8 fixed-point
    uint16_t frame_count;   ///< Number of frames in file

    uint8_t *JPEG_tables;   ///< \protected JPEG tables used by DefineBits tags.
                            ///< This MUST be set before attempting to write a DefineBits.
} SWF;

/**
 * \brief Opaque struct containing data used by the SWF parser.
 */
typedef struct SWF_Parser SWFParser;

/**
 * \brief Function prototype used by callbacks
 * \param[in] parser SWF Parser in use
 * \param[in] data   Data relevant to callback; cast to whichever type it should be
 * \param[in] ctx    SWFParserCallbacks->ctx
 * \return    SWFError indicating if something went wrong (often ignored).
 */
typedef SWFError (*SWFParserCallback)(SWFParser *parser, void *data, void *ctx);

/**
 * \brief Callbacks used by the SWF parser.
 */
typedef struct {
    SWFParserCallback tag_cb;       ///< Called when a tag is parsed. data=SWFTag*
                                    ///< If this callback is provided, the SWFTag
                                    ///< will not be added to swf->tags unless the
                                    ///< user calls swf_add_tag(swf, data), and
                                    ///< if swf_add_tag is not called, the user
                                    ///< is responsible for calling swf_tag_free.
                                    ///< This callback is not called for END tags.
    SWFParserCallback header_cb;    ///< Called when the uncompressed header is parsed. data=NULL
    SWFParserCallback header2_cb;   ///< Called when the compressed header is parsed. data=NULL
    SWFParserCallback end_cb;       ///< Called when an END tag is parsed.
                                    ///< No additional tags are parsed after this.
    void* ctx;                      ///< User-provided pointer, passed as the
                                    ///< third argument to all callbacks.
} SWFParserCallbacks;

/**
 * \brief Allocates an SWFParser and accompanying SWF.
 * \return Pointer if the parser and SWF could be allocated; NULL otherwise.
 */
SWFParser* swf_parser_init(void);
/**
 * \brief Appends data to the parser's buffer.
 * The data provided will be decompressed if necessary, and the parser will
 * parse as many tags from the file as possible.
 * \param[in] parser SWFParser to append to
 * \param[in] buf    Buffer to parse
 * \param[in] len    Size of buf
 * \return SWF_OK if at least one tag could be parsed.
 * SWF_NEED_MORE_DATA if no tags could be parsed because buf wasn't long enough.
 * SWF_FINISHED if an END tag has been parsed.
 * SWFError < 0 if something went wrong.
 */
SWFError swf_parser_append(SWFParser *parser, const void *buf, size_t len);
/**
 * \brief Gets the SWF from a parser
 * \param[in] parser Parser to get an SWF from
 * \return Pointer to SWF from parser
 */
SWF* swf_parser_get_swf(SWFParser *parser);
/**
 * \brief Gets the SWFErrorDesc from a parser
 * \param[in] parser Parser to get an error from
 * \return Pointer to SWFErrorDesc from parser
 */
SWFErrorDesc *swf_parser_get_error(SWFParser *parser);
/**
 * \brief Allocates a SWF
 * \return Pointer if the SWF could be allocated; NULL otherwise.
 */
SWF* swf_init(void);
/**
 * \brief Frees an SWF and all associated data
 * \param[in] swf SWF to free
 */
void swf_free(SWF *swf);
/**
 * \brief Frees an SWFTag's associated data. Does not call free() on the tag itself.
 * \param[in] tag SWFtag to free
 */
void swf_tag_free(SWFTag *tag);
/**
 * \brief Frees an SWFParser and all associated data.
 * \param[in] parser SWFParser to free
 */
void swf_parser_free(SWFParser *parser);
/**
 * \brief Sets callbacks for an SWFParser.
 * \param[in] parser    SWFParser to set callbacks for
 * \param[in] callbacks SWFParserCallbacks to set
 */
void swf_parser_set_callbacks(SWFParser *parser, SWFParserCallbacks *callbacks);
/**
 * \brief Adds an SWFTag to an SWF
 * \param[in] swf SWF to add a tag to
 * \param[in] tag tag to add
 * \return < 0 if something went wrong.
 */
SWFError swf_add_tag(SWF *swf, SWFTag *tag);
