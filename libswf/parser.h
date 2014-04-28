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

#include "swf.h"
#include "lzma/LzmaDec.h"
#include "buffer.h"

#ifdef HAVE_LIBZ
#include <zlib.h>
#endif

/**
 * \brief Current state of an SWF parser
 */
typedef enum {
    PARSER_STARTED,     ///< Reading uncompressed portion of header
    PARSER_HEADER,      ///< Reading compressed portion of header
    PARSER_BODY,        ///< Reading body data
    PARSER_FINISHED,    ///< Read an END tag; finished
} SWFParserState;

/**
 * \brief Private data used when parsing an SWF file
 */
struct SWF_Parser {
    SWFErrorDesc err;       ///< Last error that occurred on this SWFParser
    SWFParserState state;   ///< Current state of parser
    SWF *swf;               ///< SWF being decoded to
    Buffer buf;             ///< Temporary buffer for uncompressed data
    SWFParserCallbacks callbacks; ///< User-provided callbacks
    union {
        CLzmaDec lzma;      ///< LZMA decoder struct
        z_stream zstrm;     ///< Zlib decoder struct
    };
};
