/*
 * vi:ts=4:shiftwidth=4:expandtab
 *
 * iconvert.cpp  -  simple iconv() wrapper class
 *
 * Copyright (C) 2002 by Zhang Le <ejoy@users.sourceforge.net>
 * Begin       : 02-Aug-2002
 * Last Change : 24-Apr-2004.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cassert>
#include <stdexcept>
#include <cstdio>

#include "iconvert.hpp"

#ifndef ICONV_CONST
    #warning ICONV_CONST not defined in config.h?. guessing from platform
    #if defined(__CYGWIN__) || defined(__FreeBSD__) || defined(WIN32) // mingw defines WIN32
    #define ICONV_CONST const
    #endif

    #if defined(__linux__) 
    #define ICONV_CONST 
    #endif
#endif // ICONV_CONST

#if defined (WORDS_BIGENDIAN)
    #if (SIZEOF_UCHAR_T == 2)
        #define UCS_INTERNAL "UCS-2BE"
    #elif (SIZEOF_UCHAR_T == 4)
        #define UCS_INTERNAL "UCS-4BE"
    #else
        #warning unknown uchar_t size
    #endif
#else // default is LITTLE-ENDIAN
    #if (SIZEOF_UCHAR_T == 2)
        #define UCS_INTERNAL "UCS-2LE"
    #elif (SIZEOF_UCHAR_T == 4)
        #define UCS_INTERNAL "UCS-4LE"
    #else
        #warning unknown uchar_t size
    #endif
#endif

IConvert::IConvert(const string& encoding) {
    m_iconv_from_unicode = iconv_open(encoding.c_str(),UCS_INTERNAL);
    m_iconv_to_unicode = iconv_open(UCS_INTERNAL,encoding.c_str());

    if (m_iconv_from_unicode == (iconv_t)(-1) ||
            m_iconv_to_unicode == (iconv_t)(-1)) {
        perror("iconv_open() failed");
        throw std::runtime_error("iconv_open() failed");
    }
    m_buffer = new char[BUFFER_SIZE];
}

//IConvert::IConvert(const IConvert& rhs) {
//    m_encoding = rhs.m_encoding;
//    m_iconv_from_unicode = iconv_open(m_encoding.c_str(),UCS_INTERNAL);
//    m_iconv_to_unicode = iconv_open(UCS_INTERNAL,m_encoding.c_str());
//
//    if (m_iconv_from_unicode == (iconv_t)(-1) ||
//            m_iconv_to_unicode == (iconv_t)(-1)) {
//        perror("iconv_open() failed");
//        throw std::runtime_error("iconv_open() failed");
//    }
//    m_buffer = new char[BUFFER_SIZE];
//}

IConvert::~IConvert() {
    iconv_close(m_iconv_from_unicode);
    iconv_close(m_iconv_to_unicode);
    delete[] m_buffer;
}

bool IConvert::convert(const string& from, ustring& to) {
	size_t insize = 0;
	size_t outsize = 0;

    iconv(m_iconv_to_unicode, NULL, &insize, NULL, &outsize);

	insize = from.size();
	outsize = BUFFER_SIZE;

    size_t ret;
    ICONV_CONST char* psrc = (ICONV_CONST char*)from.c_str();
    char* pdest = m_buffer;

    ret = iconv(m_iconv_to_unicode, &psrc, &insize, &pdest, &outsize);

	if (ret == (size_t)(-1)) {
        return false;
    } else {
        to.assign((uchar_t*)m_buffer,(uchar_t*)pdest);
        //to.assign((wchar_t*)m_buffer,(BUFFER_SIZE - outsize)/(sizeof(wchar_t)*2));
        return true;
    }
}

bool IConvert::convert(const ustring& from,string& to) {
    assert(!from.empty());
	size_t insize = 0;
	size_t outsize = 0;

    iconv(m_iconv_from_unicode, NULL, &insize, NULL, &outsize);

	insize = from.size() * sizeof(uchar_t);
	outsize = BUFFER_SIZE;

    size_t ret;
    ICONV_CONST char* psrc = (ICONV_CONST char*)from.data();
    char* pdest = m_buffer;

    ret = iconv(m_iconv_from_unicode, &psrc, &insize, &pdest, &outsize);

	if (ret == (size_t)(-1))
        return false;
    else {
        to.assign(m_buffer,pdest);
        //to.assign(m_buffer,(BUFFER_SIZE - outsize)/sizeof(char));
        return true;
    }
}
