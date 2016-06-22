/*
 * vi:ts=4:shiftwidth=4:expandtab
 *
 * tools.hpp  -  some utility functions
 *
 * Copyright (C) 2002 by Zhang Le <ejoy@users.sourceforge.net>
 * Begin       : 03-Aug-2002
 * Last Change : 30-May-2004.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef TOOLS_H
#define TOOLS_H

#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "unicode.hpp"

using std::string;
using std::ustring;
using std::vector;

string next_temp_filename(const char* prefix="tempfile");
void split(const string& s,vector<string>& list,const char *delim = " \t\f\v");
// void wsplit(const ustring& s,vector<ustring>& list,const uchar_t *delim = L" \t\f\v");
bool is_punct(uchar_t ch);

/**
 * test if ch is a space(including full width  space)
 */
inline bool is_space(uchar_t ch) {
    return (ch == L' ' || ch == L'\t' || ch == L'\v'
            // XXX: is this right?
#if defined (WORDS_BIGENDIAN)
            || ch == 0x0030);
#else
            || ch == 0x3000);
#endif
}

inline bool is_cjk_symbol(uchar_t ch) {
#if defined (WORDS_BIGENDIAN)
#warning bigendian not implemented yet
#else
    return ch >= 0x3000 && ch <=0x254b;
#endif
}

inline bool is_chinese_char(uchar_t ch) {
#if defined (WORDS_BIGENDIAN)
#warning bigendian not implemented yet
#else
    return ch >= 0x4e02 && ch <=0xfa29;
#endif
}

string current_time();

bool* create_filtering_table(bool space, bool en_punct, bool cjk_punct);
#endif /* ifndef TOOLS_H */

