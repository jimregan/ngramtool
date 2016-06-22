/*
 * vi:ts=4:shiftwidth=4:expandtab
 *
 * iconvert.h  -  simple iconv() wrapper class
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

#ifndef ICONVERT_H
#define ICONVERT_H

#include <iconv.h>
#include <boost/utility.hpp>

#include "unicode.hpp"


using std::string;
using std::ustring;

class IConvert: boost::noncopyable {
    public:
        IConvert(const string& encoding);
        //IConvert(const IConvert& rhs);
        ~IConvert();
        bool convert(const string& from,ustring& to);
        bool convert(const ustring& from,string& to);

    private:
        static const size_t BUFFER_SIZE = 1024*100*sizeof(uchar_t);
        char*    m_buffer;
        string   m_encoding;
        iconv_t  m_iconv_from_unicode;
        iconv_t  m_iconv_to_unicode;
};

#endif /* ifndef ICONVERT_H */

