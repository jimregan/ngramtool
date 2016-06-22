/*
 * vi:ts=4:shiftwidth=4:expandtab
 * vim600:fdm=marker
 *
 * ngramstat.hpp  -  Implements Nagao 1994 N-gram extraction algorithm (unicode
 * version)
 *
 * This class implements Nagao 1994's N-gram extraction algorithm (with minor
 * improvements). All the Characters are encoded in unicode (uchar_t)
 * internally so we can handle oriental languages like Chinese and Japanese
 * gracefully.
 *
 * Copyright (C) 2002 by Zhang Le <ejoy@users.sourceforge.net>
 * Begin       : 30-Oct-2002
 * Last Change : 22-Apr-2004.
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

#ifndef NGRAMSTAT_H
#define NGRAMSTAT_H

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <cwchar>
#include <boost/utility.hpp>
#include <boost/function.hpp>

#include "unicode.hpp"

using std::basic_string;
using std::char_traits;
using std::string;
using std::ustring;
using std::vector;

template <typename CharT, typename Traits = char_traits<CharT> >

/**
 * This class implements Nagao 1994 N-gram extraction algorithm.
 *
 * All characters are represented with unicode (uchar_t) internally for better
 * handling of oriental languages.
 */
class NGramStat : boost::noncopyable {
    public:
        typedef basic_string<CharT, Traits> string_type;
        typedef boost::function<void(const string_type& ngram, 
                unsigned count)> OutputFunc;

        NGramStat(unsigned memory, const string& file_name_base = "",
                bool use_mmap = false);
        ~NGramStat();

        void clear();
//        void parse(const vector<string>& files, const string& encoding = "UTF-8");
//        void parse(const string& str, const string& encoding = "UTF-8");
        void parse_begin();
        void parse_end();
        void parse_buf(const string_type& buf);
//        void parse(const string_type& str);
        void extract_ngram(unsigned N,
                unsigned M,
                unsigned freq,
                OutputFunc& output) ;
//        void extract_ngram(unsigned N,
//                unsigned M,
//                unsigned freq,
//                std::ostream& os = std::cout,
//                const string& encoding = "UTF-8");
        void set_temp_dir(const string& dir){};

    private: //{{{

        struct NGram{
            string_type  m_text;
            unsigned m_count;
        };

        /**
         * compare two wide string pointed by ptable index
         * if the first 255 chars of the two string are equal
         * we define the string with lower index is smaller
         */
        struct cmp_ptable {
            cmp_ptable(const CharT* buf):m_buf(buf){}
            bool operator()(const unsigned& lhs,const unsigned& rhs) const {
                int rc = Traits::compare((const CharT*)&m_buf[lhs],
                        (const CharT*)&m_buf[rhs],255);
                return (rc < 0 || (rc == 0 && lhs < rhs));
            }
            private:
            const CharT* m_buf;
        };

        void alloc_mem();
//        void preprocess_w(ustring& buf) const;
//        string_type preprocess(ustring& buf)
//            const {return string_type();}
        void calc_ltable();
        void save_temp_buffer();
        void write_ltable() const;
        void write_ltable(const CharT* ngram_table,const string ptable_filename) const;
        void write_ptable(const string& name,unsigned start_offset) const;
        void merge_ptables();
        void add_ptable_node(unsigned start,unsigned end);
        unsigned char calc_common_words(const CharT* s1,const CharT* s2) const;
        string next_temp_ptable_filename() const;
        void fetch_ngram(unsigned N,CharT* s,string_type& ngram) const;
        void fetch_ngrams(unsigned N,unsigned M,CharT* s,vector<NGram>& ngrams) const;
        unsigned ptable_entry(unsigned* ptable,FILE* fp,unsigned pos) const;
        unsigned char ltable_entry(unsigned char* ltable,FILE* fp,unsigned pos) const;
        unsigned ptable_entry(unsigned pos);
        unsigned char ltable_entry(unsigned pos);
//        bool is_punct_w(uchar_t ch) const;
//        bool has_punct(const string_type& s) const;
//        void utf8Output(const ustring& ws,unsigned count) const;

        //extra buffer to hold enough room for at most 255 words/chars
        //(assume wordlen == 20)
        //beyond m_buffersize,these words will be copy to
        //the beginning of m_buffer after flushing out m_buffer
        //to ngram file and writing temp ptable
        static const unsigned m_extra_buffersize = 20 * 255; //about 20k

        unsigned       m_start_offset;    //in-memory buffer's offset in the whole nmram file in terms of uchar_t
        unsigned       m_buffer_offset;   //current offset in in-memory buffer,in terms of uchar_t where next char is written to
        unsigned       m_last_word_end;   //last valid word's end(word boundary + 1)

        bool           m_is_use_mmap;
        string         m_filename_base;
        std::ofstream  m_ngramfile;
        unsigned       m_mem_size;        //in kb
        unsigned       m_buffersize;      //in-memory buffer size in terms of uchar_t
        CharT*         m_buffer;
        string_type        m_buf_remain;
        vector<unsigned>      *m_ptable;
        vector<unsigned char> *m_ltable;
        vector<string>         m_tempfiles;
        static const CharT s_terminal;
//}}}
};


#include "ngramstat.tcc"
#endif /* ifndef NGRAMSTAT_H */

