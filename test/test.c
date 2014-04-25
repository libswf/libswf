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

int main(int argc, char *argv[]){
    SWFParser *parser = swf_parser_init();
    if(argc < 2){
        fprintf(stderr, "NOT ENOUGH ARGUMENTS\n");
        return 1;
    }
    FILE *file = fopen(argv[1], "r");
    if(!file){
        fprintf(stderr, "BAD FILE\n");
        return 1;
    }
    char data[1024 * 1024];
    unsigned i = 0;
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
        i++;
        SWFError ret = swf_parser_append(parser, data, read);
        if(ret < 0){
            fprintf(stderr, "ERROR: %i %i\n", ret, i);
            return 1;
        }else if(ret == SWF_FINISHED){
            break;
        }
    }
    SWF *swf = swf_parser_get_swf(parser);
    fprintf(stderr, "TAGS: %i; BLOCKS: %i\n", swf->nb_tags, i);
    fclose(file);
    return 0;
}
