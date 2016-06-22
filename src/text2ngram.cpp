/*
 * vi:ts=4:shiftwidth=4:expandtab
 * vim600:fdm=marker
 *
 * text2ngram.cpp  -  A CLI ngram extraction tool.
 *
 * Copyright (C) 2002 by Zhang Le <ejoy@users.sourceforge.net>
 * Begin       : 02-Aug-2002
 * Last Change : 23-Apr-2004.
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
#include <algorithm>
#include <stdexcept>
#include <cwchar>
#include <wctype.h>
#include <boost/ref.hpp>
#include <boost/tokenizer.hpp>
#include <boost/shared_array.hpp>

#include "tools.hpp"
#include "iconvert.hpp"
#include "itemmap.hpp"
#include "text2ngram_cmdline.h"
#include "ngramstat.hpp"
#include "vocab.hpp"

using namespace std;

Vocab g_vocab;

boost::shared_array<bool> g_filtering_table;

// important: all word ids in the map must start from 1 to avoid the confusion
// with g_id_terminal (0)
static word_id g_id_terminal = word_id();

// debug
vector<string> id2word(const basic_string<word_id>& s) {
    vector<string> vec;
    for (size_t i = 0; i < s.size(); ++i) {
            vec.push_back(g_vocab[s[i]]);
    }
    return vec;
}

bool has_punct(const ustring& s);
bool has_punct(const basic_string<word_id>& s);
bool is_punct(word_id ch);
bool is_space(word_id ch);
bool chinese_char_only(const ustring& ws);
void check_args(const gengetopt_args_info& args,unsigned N,unsigned M,unsigned freq);
string next_temp_ptable_filename();

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
//        if (m_is_exclude_space && ws.find(L' ') != ustring::npos)
//            return;

//        if (m_is_nopunct && !chinese_char_only(ws))
//            return;

        for (size_t i = 0; i < ws.size(); ++i)
            if (g_filtering_table[ws[i]])
                return;

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

bool has_punct(const basic_string<word_id>& s) {
    for (unsigned i = 0;i < s.size();++i) {
        if (is_punct(s[i]))
            return true;
    }
    return false;
}

//test if ch is a punct in g_punct_table
// FIXME: fix punct test function
bool is_punct(word_id ch) {
    return false;
    // return binary_search(g_special_ids.begin(),g_special_ids.end(),ch);
}

bool is_space(word_id ch) {
    return (ch == g_id_space || ch == g_id_vt || ch == g_id_tab);
}

/**
 *
 * preprocess text in buf
 * according to the following ruls:
 * 1 space and control chars are converted into one blank (space) ' '
 * TODO:2 pad BOS|EOS before|after each sentence
 * 3 make sure there is one and only one blanks between  each token
 */
void preprocess_char(const ustring& buf,ustring& ws) {
    ws.clear();
    unsigned i;

    for (i = 0; i < buf.size();++i) {
//        if (is_punct(buf[i])) {
//            /*
//               if (!ws.empty() && ws[ws.size() - 1] != L' ') {
//               ws += L' ';
//               }
//               */

//            ws += buf[i];
//            ws += L' ';

//        } else 
            if (is_space(buf[i])) {
            if(!ws.empty() && ws[ws.size() - 1] != L' ') {
                //do not append duplicate blanks
                ws += L' ';
            }
        } else {
//            if (i + 1 == buf.size()) {
//                this sentence does not end with punctuations (?:!.)
//                this is the last char of the sentence
//                so we add a blank and pad n EOS after this sentence
//                to prevent from claiming some false (character) N-grams:
//                for example:     S1 = ......X, S2 = Y.........
//                will be thought as XY if we do not put a blank after X
//                ws += buf[i];
//                ws += L' ';

//            } else {
                ws += buf[i];
        }
    }// end of for loop

    // for debug
    /*
       string s;
       conv.convert(buf,s);
       cerr << s;

       buf = ws;

       conv.convert(buf,s);
       cerr <<"-->"<< s<<endl;
       */

//    FIXME: wchar_t and uchar_t
//    assert(ws.find(L"  ") == ustring::npos);
//    assert(ws.find(L"   ") == ustring::npos);
//    assert(ws.find(L"    ") == ustring::npos);
//    assert(ws.find(L"     ") == ustring::npos);
//    assert(ws.size() > 0);
}

/**
 *
 * preprocess text in buf and convert each word into is
 * word_id, return a word string(ws) of word_id
 * 1 space and control chars are converted into one blank (space) ' '
 * TODO:2 pad BOS|EOS before|after each sentence
 * 3 make sure there is one and only one blanks between
 * each token
 */
void preprocess_word(const NGramStat<word_id>::string_type& buf,
        NGramStat<word_id>::string_type& ws) {
    ws.clear();
    unsigned i, j;
    unsigned gPad_num = 0;

    //pad n BOS before this sentence
    for (j = 0; j < gPad_num; ++j) {
        ws += g_id_bos;
    }

    /*
       if (ws.empty())
       ws += L' ';
       */

    for (i = 0; i < buf.size();++i) {
//        if (is_punct(buf[i])) {
//            /*
//               if (!ws.empty() && ws[ws.size() - 1] != L' ') {
//               ws += L' ';
//               }
//               */

//            ws += buf[i];
//            ws += g_id_space;

//            regard these punctuations as sentence end mark
//            and replace them with EOS
//            if (buf[i] == g_id_peroid || buf[i] == g_id_question ||
//                    buf[i] == g_id_separator || buf[i] == g_id_intr) {
//                 pad n EOS after this sentence
//                for (j = 0; j < gPad_num; ++j) {
//                    ws += g_id_eos;
//                }

//                pad n BOS before next sentence if this is not
//                the last char of the string
//                if (i + 1 < buf.size()) {
//                    for (j = 0; j < gPad_num; ++j) {
//                        ws += g_id_bos;
//                    }
//                }
//            }
//        } else
            if (is_space(buf[i])) {
            if(!ws.empty() && ws[ws.size() - 1] != g_id_space) {
                //do not append duplicate blanks
                ws += g_id_space;
            }
        } else {
//            if (i + 1 == buf.size()) {
//                this sentence does not end with ?:!.
//                this is the last char of the sentence
//                so we add a blank and pad n EOS after this sentence
//                to prevent from claiming some false (character)N-grams:
//                for example:     S1 = ......X, S2 = Y.........
//                will be thought as XY if we do not put a blank after X
//                ws += buf[i];
//                ws += g_id_space;

//                for (j = 0; j < gPad_num; ++j) {
//                    ws += g_id_eos;
//                }
//           }
                ws += buf[i];
        }
    }// end of for loop

#ifndef NDEBUG
    static word_id d_space[] = {g_id_space,g_id_space, g_id_terminal};
    assert(ws.find(d_space) == NGramStat<word_id>::string_type::npos);
    assert(ws.size() > 0);
#endif
}

/**
 * parse several files in given encoding
 */
void parse_files(NGramStat<uchar_t, uchar_traits>& ngram,
        const vector<string>& files, const string& encoding){
    IConvert iconv(encoding);
    string  s;
    ustring buf;
    ustring buf2;

    ngram.parse_begin();

    for (unsigned i = 0;i < files.size(); ++i) {
        ifstream f(files[i].c_str());
        if (!f)
            throw runtime_error("unable to open file for parsing");

        cerr << "Parsing file: " << files[i] << endl;
        while (getline(f,s)) {
            if (s.empty())
                continue;

            if (iconv.convert(s,buf) == false) {
                cerr << '\"' << s << "\" can not be converted into UNICODE" << endl;
                continue;
            }

            preprocess_char(buf,buf2);
            ngram.parse_buf(buf2);
        }
    }

    ngram.parse_end();
}

/**
 * parse several files in given encoding
 * treat the input file as a sequence of words and map words into word_ids
 */
void parse_files(NGramStat<word_id>& ngram,const vector<string>& files, const
        string& encoding){
    IConvert iconv(encoding);
    string  s;
    string buf;
    NGramStat<word_id>::string_type words;
    NGramStat<word_id>::string_type buf2;

    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    boost::char_separator<char> sep(" \t\f\v\r\n");
    tokenizer tokens(buf, sep);

    ngram.parse_begin();

    for (unsigned i = 0;i < files.size(); ++i) {
        ifstream f(files[i].c_str());
        if (!f)
            throw runtime_error("unable to open file for parsing");

        cerr << "Parsing file: " << files[i] << endl;
        while (getline(f,s)) {
            if (s.empty())
                continue;

            words.clear();
            tokens.assign(s);
            for (tokenizer::iterator it = tokens.begin(); it != tokens.end();
                    ++it)
                words.push_back(g_vocab.add(it->c_str()));

            preprocess_word(words,buf2);
            ngram.parse_buf(buf2);
        }
    }

    ngram.parse_end();
}

void check_args(const gengetopt_args_info& args,unsigned N,unsigned M,unsigned freq) {
    if (args.output_given)  {
        if (args.min_n_given || args.max_n_given || args.freq_given || args.nopunct_given) {
            cerr << "You can only extract N-gram from in-memory ptable and ltable." << endl;
            cerr << "Use extractngram utility to extract N-gram from external ngram file." << endl;
            exit(EXIT_FAILURE);
        }
    } else {
        if (!args.min_n_given) {
            cerr << "You must provide a ngram file name to store ptable and ltable" << endl;
            exit(EXIT_FAILURE);
        } else if (!(N >= 1 && M >=1 && N <= M && M <= 255 && freq >= 1)) {
            cerr << "wrong args given" << endl;
            cerr << "accepted value:" << endl;
            cerr << "1 <= N <= M <= 255" << endl;
            cerr << "freq >= 1" << endl;
            exit(EXIT_FAILURE);
        }
    }

    if (!args.char_flag && args.nopunct_flag) {
            cerr << "punctuation filtering is only supported in character n-gram mode" << endl;
            exit(EXIT_FAILURE);
    }
}


//return next temp ptable filename
string next_temp_ptable_filename() {
    return next_temp_filename("text2ngram");
}

int main(int argc,char* argv[]) {
    gengetopt_args_info args_info;

    /* let's call our cmdline parser */
    if (cmdline_parser (argc, argv, &args_info) != 0)
        return EXIT_FAILURE;

    unsigned N;      //min ngram
    unsigned M;      //max ngram
    unsigned freq;   //freq threshold
    N = args_info.min_n_given ? args_info.min_n_arg : 0;
    M = args_info.max_n_given ? args_info.max_n_arg : N;
    freq = args_info.freq_arg;

    check_args(args_info, N, M, freq);

    cerr << "start at: " << current_time();
    cerr << "N-Gram type:     " << (args_info.char_flag ? "Character" :
        "Word") << endl;
    if (args_info.char_flag) {
        cerr << "Input  Encoding: " << args_info.from_arg << endl;
        cerr << "Output Encoding: " << args_info.to_arg << endl;
    }

    try {
        vector<string> files;
        if (args_info.inputs_num > 0) {
            for ( unsigned i = 0 ; i < args_info.inputs_num ; ++i )
                files.push_back(args_info.inputs[i]);
        } else {
            //reading from stdin not support yet
            throw runtime_error("read from stdin not support yet");
        }

        string encoding = args_info.from_arg;

        if (args_info.char_flag) { // character ngrams
            g_filtering_table.reset(create_filtering_table(true,
                        args_info.nopunct_flag, args_info.nopunct_flag));

            NGramStat<uchar_t, uchar_traits> ngram(args_info.mem_arg * 1024,
                    args_info.output_arg?args_info.output_arg:"",
                    args_info.mmap_flag);

            parse_files(ngram, files, encoding);

            if (N) {
                CharOutputHelper out(cout,true,args_info.nopunct_flag,
                        args_info.to_arg);
                NGramStat<uchar_t, uchar_traits>::OutputFunc f =
                    boost::ref(out);

                ngram.extract_ngram(N,M,freq,f);
            }
        } else {
            init_special_id(g_vocab);

            NGramStat<word_id> ngram(args_info.mem_arg * 1024,
                    args_info.output_arg?args_info.output_arg:"",
                    args_info.mmap_flag);

            parse_files(ngram, files, encoding);

            if (N) {
                WordOutputHelper out(cout,true,args_info.nopunct_flag,
                        args_info.to_arg);
                NGramStat<word_id>::OutputFunc f = boost::ref(out);

                ngram.extract_ngram(N,M,freq,f);
            }

            if (args_info.output_arg) {
                save_vocab(string(args_info.output_arg) + ".vocab", g_vocab);
            }
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

    cerr << "Done!" << endl;
    cerr << "end at: " << current_time();

    return EXIT_SUCCESS;
}
