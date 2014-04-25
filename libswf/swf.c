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

#include "internal.h"
#include <stdlib.h>

SWF *swf_init(void){
    return calloc(1, sizeof(SWF));
}

void swf_tag_free(SWFTag *tag){
    if(!tag)
        return;
    if(tag->payload){
        // TODO: Free any deeper data structures if necessary
        free(tag->payload);
        tag->payload = NULL;
    }
}

void swf_free(SWF *swf){
    if(!swf)
        return;
    if(swf->tags){
        for(int i = 0; i < swf->nb_tags; i++){
            swf_tag_free(swf->tags + i);
        }
        free(swf->tags);
        swf->tags = NULL;
    }
    if(swf->sprites){
        free(swf->sprites);
        swf->sprites = NULL;
    }
    if(swf->JPEG_tables){
        free(swf->JPEG_tables);
        swf->JPEG_tables = NULL;
    }
}
