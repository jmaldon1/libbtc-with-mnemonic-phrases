/*

 The MIT License (MIT)

 Copyright (c) 2015 Douglas J. Bakkum

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the "Software"),
 to deal in the Software without restriction, including without limitation
 the rights to use, copy, modify, merge, publish, distribute, sublicense,
 and/or sell copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef __LIBBTC_RANDOM_H__
#define __LIBBTC_RANDOM_H__

#include "btc.h"

LIBBTC_BEGIN_DECL

typedef struct btc_rnd_mapper_ {
    void (*btc_random_init)(void);
    btc_bool (*btc_random_bytes)(uint8_t* buf, uint32_t len, const uint8_t update_seed);
} btc_rnd_mapper;

// set's a custom random callback mapper
// this function is _not_ thread safe and should be called before anything else
LIBBTC_API void btc_rnd_set_mapper(const btc_rnd_mapper mapper);
LIBBTC_API void btc_rnd_set_mapper_default();

LIBBTC_API void btc_random_init(void);
LIBBTC_API btc_bool btc_random_bytes(uint8_t* buf, uint32_t len, const uint8_t update_seed);

LIBBTC_END_DECL

#endif // __LIBBTC_TX_H__
