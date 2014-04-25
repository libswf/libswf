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

#include <stdio.h>
#include "libswf/swf.h"

SWFError tag_cb(SWFParser *parser, void *tag_in, void *priv){
    SWFTag *tag = tag_in;
    unsigned *i = priv;
    printf("Tag #%u: Type: %2i; ID: %4x; Size: %"PRIu32"\n", (*i)++, tag->type, tag->id, tag->size);
    swf_tag_free(tag);
    return SWF_OK;
}

SWFError header_cb(SWFParser *parser, void *none, void *priv){
    SWF *swf = swf_parser_get_swf(parser);
    char *cmpstr = swf->compression == SWF_ZLIB ? "ZLIB" :
                  (swf->compression == SWF_LZMA ? "LZMA" : "None");
    printf("Parsed header. Compression: %s; Version: %i; "
           "Uncompressed size: %"PRIu32"\n",
           cmpstr, swf->version, swf->size);
    return SWF_OK;
}

SWFError header2_cb(SWFParser *parser, void *none, void *priv){
    SWF *swf = swf_parser_get_swf(parser);
    printf("Parsed compressed header. Frame size: %ix%i; "
           "Frame rate: %"PRIu16"; Frame count: %"PRIu16"\n",
           swf->frame_size.x_max, swf->frame_size.y_max,
           swf->frame_rate, swf->frame_count);
    return SWF_OK;
}

int main(int argc, char *argv[]){
    if(argc < 2){
        fprintf(stderr, "NOT ENOUGH ARGUMENTS\n");
        return 1;
    }
    FILE *file = fopen(argv[1], "r");
    if(!file){
        fprintf(stderr, "BAD FILE\n");
        return 1;
    }
    SWFParser *parser = swf_parser_init();
    unsigned i = 0;
    SWFParserCallbacks callbacks = {
        .priv = &i,
        .tag_cb = tag_cb,
        .header_cb = header_cb,
        .header2_cb = header2_cb,
    };
    swf_parser_set_callbacks(parser, &callbacks);
    SWF *swf = swf_parser_get_swf(parser);
    uint8_t data[1024 * 1024];
    while(1){
        size_t read = fread(data, 1, sizeof(data), file);
        if(read == 0){
            int err = ferror(file);
            if(err){
                fprintf(stderr, "FERROR: %i\n", err);
            }else{
                fprintf(stderr, "EOF: %i\n", feof(file));
            }
            break;
        }
        SWFError ret = swf_parser_append(parser, data, read);
        if(ret < 0){
            fprintf(stderr, "ERROR: %i\n", ret);
            return 1;
        }else if(ret == SWF_FINISHED){
            break;
        }
    }
    swf_free(swf);
    swf_parser_free(parser);
    fclose(file);
    printf("Finished.\n");
    return 0;
}
