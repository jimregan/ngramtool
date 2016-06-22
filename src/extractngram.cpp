/*
 * vi:ts=4:shiftwidth=4:expandtab
 * vim600:fdm=marker
 *
 * extractngram.cpp  -  description
 *
 * Copyright (C) 2002 by Zhang Le <ejoy@users.sourceforge.net>
 * Begin       : 02-Aug-2002
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

#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <iostream>
#include <string>
#include <cwchar>
#include <cwctype>
#include <algorithm>
#include <boost/ref.hpp>
#include <boost/shared_array.hpp>

#include "tools.hpp"
#include "itemmap.hpp"
#include "vocab.hpp"
#include "iconvert.hpp"
#include "extractngram_cmdline.h"
#include "ngramstat.hpp"

using namespace std;

Vocab g_vocab;

boost::shared_array<bool> g_filtering_table;

void check_args(const gengetopt_args_info& args, unsigned N,unsigned M,unsigned freq);
bool is_punct(word_id ch);
bool has_punct(const basic_string<word_id>& s);
bool chinese_char_only(const ustring& ws);

//helper output function object
template<typename CharT, typename Traits = std::char_traits<CharT> >
struct OutputHelper{
    OutputHelper(ostream& os,bool exclude_space,bool word_only,const string& encoding)
    :m_os(&os),
    m_is_exclude_space(exclude_space),
    m_is_nopunct(word_only),
    m_iconv(encoding){}

    void operator()(const basic_string<CharT, Traits>& s,unsigned count) const {
        assert(!"re-implement your own operator<CharT>()");
    }

    protected:
    ostream* m_os;
    bool m_is_exclude_space;
    bool m_is_nopunct;
    mutable IConvert m_iconv;
};

struct CharOutputHelper:public OutputHelper<uchar_t, uchar_traits> {
    CharOutputHelper(ostream& os,bool exclude_space,bool word_only,const string& encoding)
        :OutputHelper<uchar_t, uchar_traits>(os,exclude_space,word_only,encoding){}
    void operator()(const ustring& ws,unsigned count) const {
        assert(!ws.empty());

        static string s;
        for (size_t i = 0; i < ws.size(); ++i)
            if (g_filtering_table[ws[i]])
                return;
//        if (m_is_exclude_space && ws.find(L' ') != ustring::npos)
//            return;

//        if (m_is_nopunct && !chinese_char_only(ws))
//            return;

        if (m_iconv.convert(ws,s))
            (*m_os) << s << ' ' << count << endl;
    }

};

struct WordOutputHelper:public OutputHelper<word_id> {
    WordOutputHelper(ostream& os,bool exclude_space,bool word_only,const string& encoding)
        :OutputHelper<word_id>(os,exclude_space,word_only,encoding){}
    void operator()(const basic_string<word_id>& ws,unsigned count) const {
        static string str;
        assert(!ws.empty());

        if (m_is_exclude_space && ws.find(g_id_space) != basic_string<word_id>::npos)
            return;

        if (m_is_nopunct && has_punct(ws))
            return;

        str.clear();
        for (unsigned i = 0;i < ws.size(); ++i) {
            str += g_vocab[ws[i]];
            str += ' ';
        }
        (*m_os) << str << count << endl;
    }
};

//helper count function object
struct CharCountHelper:public OutputHelper<uchar_t, uchar_traits>{
    CharCountHelper(bool exclude_space,bool word_only)
        :OutputHelper<uchar_t, uchar_traits>(cout,exclude_space,word_only,"UTF-8"),
    m_count(0)
    {}

    void operator()(const ustring& ws,unsigned count) {
        assert(!ws.empty());
        for (size_t i = 0; i < ws.size(); ++i)
            if (g_filtering_table[ws[i]])
                return;
        ++m_count;
    }
    unsigned count() const { return m_count; }

    private:
    unsigned m_count;
};

struct WordCountHelper:public OutputHelper<word_id>{
    WordCountHelper(bool exclude_space,bool word_only)
        :OutputHelper<word_id>(cout,exclude_space,word_only,"UTF-8"),
    m_count(0)
    {}

    void operator()(const basic_string<word_id>& s,unsigned count) {
        assert(!s.empty());
        if (m_is_exclude_space && s.find(g_id_space) != basic_string<word_id>::npos)
            return;

        if (m_is_nopunct && has_punct(s))
            return;
        ++m_count;
    }
    unsigned count() const { return m_count; }

    private:
    unsigned m_count;
};

//test if ch is a punct in g_punct_table
//FIXME: fix the punct problem
bool is_punct(word_id ch) {
    return false;
    // return binary_search(g_special_ids.begin(),g_special_ids.end(),ch);
}


bool has_punct(const basic_string<word_id>& s) {
    for (unsigned i = 0;i < s.size();++i) {
        if (is_punct(s[i]))
            return true;
    }

    return false;
}


void check_args(const gengetopt_args_info& args, unsigned N,unsigned M,unsigned freq) {
    if (!(N >= 1 && M >=1 && N <= M && M <= 255 && freq >= 1)) {
        cerr << "wrong args given" << endl;
        cerr << "accepted value:" << endl;
        cerr << "1 <= N <= M <= 255" << endl;
        cerr << "freq >= 1" << endl;
        exit(EXIT_FAILURE);
    }

    if (!args.char_flag && args.nopunct_flag) {
            cerr << "punctuation filtering is only supported in character n-gram mode" << endl;
            exit(EXIT_FAILURE);
    }
}

int main(int argc,char* argv[]) {
    gengetopt_args_info args_info;

    /* let's call our cmdline parser */
    if (cmdline_parser (argc, argv, &args_info) != 0)
        return EXIT_FAILURE;

    unsigned N;      //min ngram
    unsigned M;      //max ngram
    unsigned freq;   //freq threshold
    N = args_info.min_n_arg;
    M = args_info.max_n_given ? args_info.max_n_arg : N;
    freq = args_info.freq_arg;

    check_args(args_info, N,M,freq);

    cerr << "start at: " << current_time();

    try {
        if (args_info.char_flag) { // character ngram
            g_filtering_table.reset(create_filtering_table(true,
                        args_info.nopunct_flag, args_info.nopunct_flag));

            NGramStat<uchar_t, uchar_traits> ngram(10,
                    args_info.input_arg,
                    args_info.mmap_flag);

            CharOutputHelper out(cout,true,args_info.nopunct_flag,
                    args_info.to_arg);
            CharCountHelper count(true,args_info.nopunct_flag);
            NGramStat<uchar_t, uchar_traits>::OutputFunc f;
            if (args_info.count_flag)
                f = boost::ref(count);
            else
                f = boost::ref(out);

            ngram.extract_ngram(N,M,freq,f);
            if (args_info.count_flag)
                cout << count.count() << endl;
        } else { // word ngrams
            string vocab = string(args_info.input_arg) + ".vocab";
            if (access(vocab.c_str(), R_OK)) {
                string msg = string("Vocab file ");
                msg += vocab;
                msg += " not found, are you missing -c (char ngram) flag?";
                throw runtime_error(msg.c_str());
            }
            load_vocab(string(args_info.input_arg) + ".vocab", g_vocab);
            NGramStat<word_id> ngram(10,
                    args_info.input_arg,
                    args_info.mmap_flag);

            WordOutputHelper out(cout, true, args_info.nopunct_flag,
                    args_info.to_arg);
            WordCountHelper count(true,args_info.nopunct_flag);
            NGramStat<word_id>::OutputFunc f;
            if (args_info.count_flag)
                f = boost::ref(count);
            else
                f = boost::ref(out);

            ngram.extract_ngram(N,M,freq,f);
            if (args_info.count_flag)
                cout << count.count() << endl;
        }

    } catch (bad_alloc& e) {
        cerr << "std::bad_alloc caught: out of memory" << endl;
        return EXIT_FAILURE;
    } catch (runtime_error& e) {
        cerr << "runtime_error caught:" << e.what() << endl;
        return EXIT_FAILURE;
    } catch (...) {
        cerr << "unknown exception caught!" << endl;
        return EXIT_FAILURE;
    }

    cerr << endl << "end at: " << current_time();
    cerr << endl;

   return EXIT_SUCCESS;
}

