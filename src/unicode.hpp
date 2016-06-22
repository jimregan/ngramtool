/*
 * vi:ts=4:tw=78:shiftwidth=4:expandtab
 * vim600:fdm=marker
 *
 * unicode.hpp  - This file defines a set of unicode types that can be used
 * safely on several platforms that lacks wchar_t or wstring support.
 * The following types are defined:
 *
 * uchar_t    : a unicode char [default to UCS16]
 * ustring    : a basic_string<uchar_t>
 *
 * Copyright (C) 2003 by Zhang Le <ejoy@users.sourceforge.net>
 * Begin       : 08-Aug-2003
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

#ifndef UNICODE_H
#define UNICODE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

typedef unsigned short uchar_t;
typedef unsigned short uint_t;

#define SIZEOF_UCHAR_T 2

// custom traits class for coping with system lacking char_traits for wchar_t
namespace std {
template <class _CharT, class _IntT> class __char_traits_base {
public:
  typedef _CharT char_type;
  typedef _IntT int_type;

  static void  assign(char_type& __c1, const char_type& __c2) { __c1 = __c2; }
  static bool  eq(const _CharT& __c1, const _CharT& __c2) 
    { return __c1 == __c2; }
  static bool  lt(const _CharT& __c1, const _CharT& __c2) 
    { return __c1 < __c2; }

  static int  compare(const _CharT* __s1, const _CharT* __s2, size_t __n) {
    for (size_t __i = 0; __i < __n; ++__i)
      if (!eq(__s1[__i], __s2[__i]))
        return __s1[__i] < __s2[__i] ? -1 : 1;
    return 0;
  }

  static size_t  length(const _CharT* __s) {
    const _CharT _NullChar = _CharT();
    size_t __i;
    for (__i = 0; !eq(__s[__i], _NullChar); ++__i)
      {}
    return __i;
  }

  static const _CharT*  find(const _CharT* __s, size_t __n, const _CharT& __c) {
    for ( ; __n > 0 ; ++__s, --__n)
      if (eq(*__s, __c))
        return __s;
    return 0;
  }


  static _CharT*  move(_CharT* __s1, const _CharT* __s2, size_t _Sz) {    
    return (_Sz == 0 ? __s1 : (_CharT*)memmove(__s1, __s2, _Sz * sizeof(_CharT)));
  }
  
  static _CharT*  copy(_CharT* __s1, const _CharT* __s2, size_t __n) {
    return (__n == 0 ? __s1 :
	    (_CharT*)memcpy(__s1, __s2, __n * sizeof(_CharT)));
    } 

  static _CharT*  assign(_CharT* __s, size_t __n, _CharT __c) {
    for (size_t __i = 0; __i < __n; ++__i)
      __s[__i] = __c;
    return __s;
  }

  static int_type  not_eof(const int_type& __c) {
    return !eq_int_type(__c, eof()) ? __c : (int_type)0;
  }

  static char_type  to_char_type(const int_type& __c) {
    return (char_type)__c;
  }

  static int_type  to_int_type(const char_type& __c) {
    return (int_type)__c;
  }

  static bool  eq_int_type(const int_type& __c1, const int_type& __c2) {
    return __c1 == __c2;
  }

  static int_type  eof() {
    return (int_type)-1;
  }
};
} // namespace std

namespace std{
    class uchar_traits
        : public __char_traits_base<uchar_t, uint_t>
        {};
    class char_traits<unsigned>
        : public __char_traits_base<unsigned, unsigned>
        {};
    typedef basic_string<uchar_t, uchar_traits> ustring;
}

#endif /* ifndef UNICODE_H */

