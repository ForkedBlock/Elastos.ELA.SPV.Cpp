//
//  BRBase58.c
//  breadwallet-core
//
//  Created by Aaron Voisine on 9/15/15.
//  Copyright (c) 2015 breadwallet LLC
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#include "BRBase58.h"
#include "BRHash.h"
#include <string.h>

// base58 and base58check encoding: https://en.bitcoin.it/wiki/Base58Check_encoding

static const char base58chars[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

// returns the number of characters written to s including NULL terminator, or total slen needed if s is NULL
size_t BRBase58Encode(char *s, size_t slen, const uint8_t *data, size_t dlen)
{
    size_t i, len, zcount = 0;
    
    while (zcount < dlen && data[zcount] == 0) zcount++; // count leading zeroes

    uint8_t buf[(dlen - zcount)*138/100 + 1]; // log(256)/log(58), rounded up
    
    memset(buf, 0, sizeof(buf));
    
    for (i = zcount; i < dlen; i++) {
        uint32_t carry = data[i];
        
        for (size_t j = sizeof(buf); j > 0; j--) {
            carry += (uint32_t)buf[j - 1] << 8;
            buf[j - 1] = carry % 58;
            carry /= 58;
        }
        
        carry = 0;
    }
    
    i = 0;
    while (i < sizeof(buf) && buf[i] == 0) i++; // skip leading zeroes
    len = (zcount + sizeof(buf) - i) + 1;

    if (s && slen >= len) {
        while (zcount-- > 0) *(s++) = base58chars[0];
        while (i < sizeof(buf)) *(s++) = base58chars[buf[i++]];
        *s = '\0';
    }
    
    memset(buf, 0, sizeof(buf));
    return (! s || slen >= len) ? len : 0;
}

// returns the number of bytes written to data, or total dlen needed if data is NULL
size_t BRBase58Decode(uint8_t *data, size_t dlen, const char *s)
{
    size_t i = 0, len, zcount = 0;
    
    while (*s == base58chars[0]) s++, zcount++; // count leading zeroes
    
    uint8_t buf[strlen(s)*733/1000 + 1]; // log(58)/log(256), rounded up
    
    memset(buf, 0, sizeof(buf));
    
    while (*s) {
        uint32_t carry = *(s++);
        
        switch (carry) {
            case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
                carry -= '1';
                break;
                
            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
                carry += 9 - 'A';
                break;
                
            case 'J': case 'K': case 'L': case 'M': case 'N':
                carry += 17 - 'J';
                break;
                
            case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
            case 'Z':
                carry += 22 - 'P';
                break;
                
            case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
            case 'k':
                carry += 33 - 'a';
                break;
                
            case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u': case 'v':
            case 'w': case 'x': case 'y': case 'z':
                carry += 44 - 'm';
                break;
                
            default:
                carry = UINT32_MAX;
        }
        
        if (carry >= 58) break; // invalid base58 digit
        
        for (size_t j = sizeof(buf); j > 0; j--) {
            carry += (uint32_t)buf[j - 1]*58;
            buf[j - 1] = carry & 0xff;
            carry >>= 8;
        }
        
        carry = 0;
    }
    
    while (i < sizeof(buf) && buf[i] == 0) i++; // skip leading zeroes
    len = zcount + sizeof(buf) - i;

    if (data && dlen >= len) {
        if (zcount > 0) memset(data, 0, zcount);
        memcpy(data + zcount, &buf[i], sizeof(buf) - i);
    }

    memset(buf, 0, sizeof(buf));
    return (! data || dlen >= len) ? len : 0;
}

// returns the number of characters written to s including NULL terminator, or total slen needed if s is NULL
size_t BRBase58CheckEncode(char *s, size_t slen, const uint8_t *data, size_t dlen)
{
    size_t len;
    uint8_t buf[dlen + 256/8];

    memcpy(buf, data, dlen);
    BRSHA256_2(&buf[dlen], data, dlen);
    len = BRBase58Encode(s, slen, buf, dlen + 4);
    memset(buf, 0, sizeof(buf));
    return len;
}

// returns the number of bytes written to data, or total dlen needed if data is NULL
size_t BRBase58CheckDecode(uint8_t *data, size_t dlen, const char *s)
{
    uint8_t buf[strlen(s)*733/1000 + 1], md[256/8];
    size_t len = BRBase58Decode(buf, sizeof(buf), s);
    
    if (len >= 4) {
        len -= 4;
        BRSHA256_2(md, buf, len);
        if (*(uint32_t *)&buf[len] != *(uint32_t *)md) len = 0; // verify checksum
        if (data && dlen >= len) memcpy(data, buf, len);
    }
    else len = 0;
    
    memset(buf, 0, sizeof(buf));
    return (! data || dlen >= len) ? len : 0;
}