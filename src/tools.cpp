/*
 * vi:ts=4:shiftwidth=4:expandtab
 *
 * tools.cpp  -  description
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

#if defined(WIN32)
    #include <windows.h>
#else
    #include <sys/utsname.h>
#endif

#include <cstdio>
#include <iostream>
#include <ctime>
#include <algorithm>
#include <cassert>
#include "tools.hpp"

using namespace std;

// punct table {{{
char en_punct_table[] = {
    041, /*``!''*/
    042, /*``"''*/
    043, /*``#''*/
    044, /*``$''*/
    045, /*``%'' */
    046, /*``&''*/
    047, /*``'''*/
    050, /*``(''*/
    051, /*``)''*/
    052, /*``*''*/
    053, /*``+''*/
    054, /*``,''*/
    055, /*``-''*/
    056, /*``.''*/
    057, /*``/''*/
    072, /*``:''*/
    073, /*``;''*/
    074, /*``<''*/
    075, /*``=''*/
    076, /*``>''*/
    077, /*``?''*/
    0100, /*``@''*/
    0133, /*``[''*/
    0134, /*``\''*/
    0135, /*``]''*/
    0136, /*``^''*/
    0137, /*``_''*/
    0140, /*```''*/
    0173, /*``{''*/
    0174, /*``|''*/
    0175, /*``}''*/
    0176 /*``~''*/
};

unsigned short space_table[] = { // from unicode 4.0 specification
0x0009, //  White_Space # Cc   [5] <control-0009>..<control-000D>
0x000A, 
0x000B, 
0x000C, 
0x000D,
0x0020, //           ; White_Space # Zs       SPACE
0x0085, //           ; White_Space # Cc       <control-0085>
0x00A0, //           ; White_Space # Zs       NO-BREAK SPACE
0x1680, //           ; White_Space # Zs       OGHAM SPACE MARK
0x180E, //           ; White_Space # Zs       MONGOLIAN VOWEL SEPARATOR
0x2000, //           ; White_Space # Zs  [11] EN QUAD..HAIR SPACE
0x2001, 
0x2002, 
0x2003, 
0x2004, 
0x2005, 
0x2006, 
0x2007, 
0x2008, 
0x2009, 
0x200A, 
0x2028, //           ; White_Space # Zl       LINE SEPARATOR
0x2029, //           ; White_Space # Zp       PARAGRAPH SEPARATOR
0x202F, //           ; White_Space # Zs       NARROW NO-BREAK SPACE
0x205F, //           ; White_Space # Zs       MEDIUM MATHEMATICAL SPACE
0x3000  //           ; White_Space # Zs       IDEOGRAPHIC SPACE
};

unsigned short unicode_punct_table[] = {
0x0021, //           ; Terminal_Punctuation # Po       EXCLAMATION MARK
0x002C, //           ; Terminal_Punctuation # Po       COMMA
0x002E, //           ; Terminal_Punctuation # Po       FULL STOP
0x003A, //           ; Terminal_Punctuation # Po   [2] COLON..SEMICOLON
0x003B, 
0x003F, //           ; Terminal_Punctuation # Po       QUESTION MARK
0x037E, //           ; Terminal_Punctuation # Po       GREEK QUESTION MARK
0x0387, //           ; Terminal_Punctuation # Po       GREEK ANO TELEIA
0x0589, //           ; Terminal_Punctuation # Po       ARMENIAN FULL STOP
0x05C3, //           ; Terminal_Punctuation # Po       HEBREW PUNCTUATION SOF PASUQ
0x060C, //           ; Terminal_Punctuation # Po       ARABIC COMMA
0x061B, //           ; Terminal_Punctuation # Po       ARABIC SEMICOLON
0x061F, //           ; Terminal_Punctuation # Po       ARABIC QUESTION MARK
0x06D4, //           ; Terminal_Punctuation # Po       ARABIC FULL STOP
0x0700, // ..070A    ; Terminal_Punctuation # Po  [11] SYRIAC END OF PARAGRAPH..SYRIAC CONTRACTION
0x0701, 
0x0702, 
0x0703, 
0x0704, 
0x0705, 
0x0706, 
0x0707, 
0x0708, 
0x0709, 
0x070A, 
0x070C, //           ; Terminal_Punctuation # Po       SYRIAC HARKLEAN METOBELUS
0x0964, // ..0965    ; Terminal_Punctuation # Po   [2] DEVANAGARI DANDA..DEVANAGARI DOUBLE DANDA
0x0965, 
0x0E5A, // ..0E5B    ; Terminal_Punctuation # Po   [2] THAI CHARACTER ANGKHANKHU..THAI CHARACTER KHOMUT
0x0E5B, 
0x0F08, //           ; Terminal_Punctuation # Po       TIBETAN MARK SBRUL SHAD
0x0F0D, // ..0F12    ; Terminal_Punctuation # Po   [6] TIBETAN MARK SHAD..TIBETAN MARK RGYA GRAM SHAD
0x0F0E, 
0x0F0F, 
0x0F10, 
0x0F11, 
0x0F12, 
0x104A, // ..104B    ; Terminal_Punctuation # Po   [2] MYANMAR SIGN LITTLE SECTION..MYANMAR SIGN SECTION
0x104B, 
0x1361, // ..1368    ; Terminal_Punctuation # Po   [8] ETHIOPIC WORDSPACE..ETHIOPIC PARAGRAPH SEPARATOR
0x1362, 
0x1363, 
0x1364, 
0x1365, 
0x1366, 
0x1367, 
0x1368, 
0x166D, // ..166E    ; Terminal_Punctuation # Po   [2] CANADIAN SYLLABICS CHI SIGN..CANADIAN SYLLABICS FULL STOP
0x166E, 
0x16EB, // ..16ED    ; Terminal_Punctuation # Po   [3] RUNIC SINGLE PUNCTUATION..RUNIC CROSS PUNCTUATION
0x16EC, 
0x16ED, 
0x17D4, // ..17D6    ; Terminal_Punctuation # Po   [3] KHMER SIGN KHAN..KHMER SIGN CAMNUC PII KUUH
0x17D5, 
0x17D6, 
0x17DA, //           ; Terminal_Punctuation # Po       KHMER SIGN KOOMUUT
0x1802, // ..1805    ; Terminal_Punctuation # Po   [4] MONGOLIAN COMMA..MONGOLIAN FOUR DOTS
0x1803, 
0x1804, 
0x1805, 
0x1808, // ..1809    ; Terminal_Punctuation # Po   [2] MONGOLIAN MANCHU COMMA..MONGOLIAN MANCHU FULL STOP
0x1809, 
0x1944, // ..1945    ; Terminal_Punctuation # Po   [2] LIMBU EXCLAMATION MARK..LIMBU QUESTION MARK
0x1945, 
0x203C, // ..203D    ; Terminal_Punctuation # Po   [2] DOUBLE EXCLAMATION MARK..INTERROBANG
0x203D, 
0x2047, // ..2049    ; Terminal_Punctuation # Po   [3] DOUBLE QUESTION MARK..EXCLAMATION QUESTION MARK
0x2048, 
0x2049, 
0x3001, // ..3002    ; Terminal_Punctuation # Po   [2] IDEOGRAPHIC COMMA..IDEOGRAPHIC FULL STOP
0x3002, 
0xFE50, // ..FE52    ; Terminal_Punctuation # Po   [3] SMALL COMMA..SMALL FULL STOP
0xFE51, 
0xFE52, 
0xFE54, // ..FE57    ; Terminal_Punctuation # Po   [4] SMALL SEMICOLON..SMALL EXCLAMATION MARK
0xFE55, 
0xFE56, 
0xFE57, 
0xFF01, //           ; Terminal_Punctuation # Po       FULLWIDTH EXCLAMATION MARK
0xFF0C, //           ; Terminal_Punctuation # Po       FULLWIDTH COMMA
0xFF0E, //           ; Terminal_Punctuation # Po       FULLWIDTH FULL STOP
0xFF1A, // ..FF1B    ; Terminal_Punctuation # Po   [2] FULLWIDTH COLON..FULLWIDTH SEMICOLON
0xFF1B, 
0xFF1F, //           ; Terminal_Punctuation # Po       FULLWIDTH QUESTION MARK
0xFF61, //           ; Terminal_Punctuation # Po       HALFWIDTH IDEOGRAPHIC FULL STOP
0xFF64 //           ; Terminal_Punctuation # Po       HALFWIDTH IDEOGRAPHIC COMMA
};

unsigned short cjk_punct_table[] = {
0x0021, // '!'
0x0022, // '"'
0x0025, // '%'
0x0028, // '('
0x0029, // ')'
0x002C, // ','
0x002E, // '.'
0x002F, // '/'
0x003A, // ':'
0x003B, // ';'
0x003C, // '<'
0x003E, // '>'
0x003F, // '?'
0x005B, // '['
0x005D, // ']'
0x00B7, // '¡¤'
0x2014, // '¡ª'
0x2018, // '¡®'
0x2019, // '¡¯'
0x201C, // '¡°'
0x201D, // '¡±'
0x2026, // '¡­'
0x2236, // '¡Ã'
0x3000, // '¡¡'
0x3001, // '¡¢'
0x3002, // '¡£'
0x3008, // '¡´'
0x3009, // '¡µ'
0x300A, // '¡¶'
0x300B, // '¡·'
0x300E, // '¡º'
0x300F, // '¡»'
0x3014, // '¡²'
0x3015, // '¡³'
0xFF01, // '£¡'
0xFF05, // '£¥'
0xFF08, // '£¨'
0xFF09, // '£©'
0xFF0C, // '£¬'
0xFF0E, // '£®'
0xFF1A, // '£º'
0xFF1B, // '£»'
0xFF1F // '£¿'
};
 // }}}

void split(const string& s,vector<string>& list,const char *delim) {
    list.clear();

    const char* begin = s.c_str();
    const char* end;

    while (*begin) {
        // skip all delimiters
        while (*begin && strchr(delim,*begin))
            ++begin;

        end = begin;

        while (*end && strchr(delim,*end) == NULL)
            ++end;

        if (*begin)
            list.push_back(string(begin,end - begin));

        begin = end;
    }
}

/*
void wsplit(const ustring& s,vector<ustring>& list,const uchar_t *delim) {
    list.clear();

    const uchar_t* begin = s.data();
    const uchar_t* end;
    const uchar_t* send = begin + s.size();

    while (begin != send) {
        // skip all delimiters
        while (begin != send && wcschr(delim,*begin))
            ++begin;

        end = begin;

        while (end != send && wcschr(delim,*end) == NULL)
            ++end;

        if (begin != send)
            list.push_back(ustring(begin,end - begin));

        begin = end;
    }
}
*/

string next_temp_filename(const char* prefix) {
    char name[100];
    static unsigned s_count;
    ++s_count;
#if defined(WIN32)
    sprintf(name,"%s.%d.tmp",prefix,s_count);
#else
    struct utsname uname_info;

    uname(&uname_info);
    sprintf(name,"%s.%s.%d.%d.tmp",prefix,uname_info.nodename,getpid(),
            s_count);
#endif

    return string(name);
}

//return current in ascii format
string current_time() {
    time_t t;
    time(&t);
    return ctime(&t);
}

// a table of 65536 entries for faster filtering certain unicode chars
bool* create_filtering_table(bool space, bool en_punct, bool cjk_punct) {
    bool* tbl = new bool[65536];
    for (size_t i = 0; i < 65536; ++i)
        tbl[i] = false;

    if (space) {
        for (size_t i = 0; i < sizeof(space_table)/sizeof(unsigned short);
                ++i)
            tbl[space_table[i]] = true;
    }

    if (en_punct) {
        for (size_t i = 0; i < sizeof(en_punct_table)/sizeof(char); ++i)
            tbl[en_punct_table[i]] = true;
    }

    if (cjk_punct) {
        for (size_t i = 0; i < sizeof(unicode_punct_table)/sizeof(unsigned short);
                ++i)
            tbl[unicode_punct_table[i]] = true;
    }

    return tbl;
}
