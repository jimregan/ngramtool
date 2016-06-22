/*
 * vi:ts=4:shiftwidth=4:expandtab
 * vim600:fdm=marker
 *
 * strreduction.cpp  -  Implementation of four Statistical Substring Reduction
 * described in (LV 2003): Research of E-Chunk Acquisition and Application in
 * Machine Translation. Ph.D.  dissertation, Northeastern Univ. P.R.China.
 *
 * Also see (Zhang et al. 2003): A Statistical Approach to Extract Chinese
 * Chunk Candidates from Large Corpora for an application.
 *
 * Copyright (C) 2002 by Zhang Le <ejoy@users.sourceforge.net>
 * Begin       : 05-Aug-2002
 * Last Change : 08-Apr-2004.
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

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <utility>
#include <cwchar>
#include <algorithm>
#include <functional>

#include <boost/tokenizer.hpp>
#include <boost/progress.hpp>
#include <boost/scoped_ptr.hpp>

#include "vocab.hpp"
#include "iconvert.hpp"
#include "strreduction.hpp"
#include "strreduction_cmdline.h"
#include "itemmap.hpp"

using namespace std;
using namespace boost;

//typedef ItemMap<const wchar_t*, hash_wchar, equ_wchar>::id_type word_id;
//ItemMap<const wchar_t*, hash_wchar, equ_wchar> g_vocab;
Vocab g_vocab;

typedef basic_string<word_id> WordString; //Chunk of words

//lexicographical compare string
template<typename StringT>
struct lexicographical_cmp_string {
    bool operator()(const pair<StringT,int>& w1,const pair<StringT,int>& w2) const {
        return w1.first < w2.first;
    }
};

//specialize version to compare according a wordmap
struct cmp_wordid {
    bool operator()(const word_id id1,const word_id id2) const {
        // TODO: optimize with a macro
        return strcmp(g_vocab[id1].c_str(), g_vocab[id2].c_str()) < 0;
    }
};

template<>
struct lexicographical_cmp_string<WordString> {
    bool operator()(const pair<WordString,int>& w1,const pair<WordString,int>& w2) const {
        return lexicographical_compare(w1.first.begin(),w1.first.end(),
                w2.first.begin(),w2.first.end(),cmp_wordid());
                //bind(wcscmp, var(g_vocab)[_1],var(g_vocab)[_2]));
    }
};

scoped_ptr<IConvert> g_iconv_from;
scoped_ptr<IConvert> g_iconv_to;

//load <ustring,freq> into a vector
//template<typename StringT>
//void load_word_from_stream<StringT>(vector<pair<StringT,int> >& v,istream& in) {}
// for character ngram
template<typename StringT>
void load_word_from_stream(vector<pair<ustring, int> >& v,istream& in) {
    v.clear();
    string s;
    pair<ustring,int> t;
    while (in) {
        in >> s;
        in >> t.second;
        if (!in.eof()) {
            if (g_iconv_from->convert(s,t.first) == false) {
                cerr << "error:" << s << "can not be converted into unicode" << endl;
                continue;
            }
            v.push_back(t);
        }
    }
}

//load <WordString,int> into a vector
// for word ngram, we map each word into a unique word_id
// FIXME: should we store word in unicode format?
template<typename StringT>
void load_word_from_stream(vector<pair<WordString,int> >& v,istream& in) {
    v.clear();

    pair<WordString,int> t;
    WordString& words = t.first;

    string s;

    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    boost::char_separator<char> sep(" \t\f\v\r\n");
    tokenizer tokens(s, sep);

    while (getline(in,s)) {
        assert(!in.eof());

        //get N-Gram freq first
        size_t pos = s.rfind(' ');
        if (pos == string::npos)
            pos = s.rfind('\t');
        if (pos == string::npos) {
            cerr << "error:" << s << "find N-Gram frequence in this line" << endl;
            continue;
        }

        t.second = atoi(s.substr(pos + 1).c_str());

        words.clear();
        s = s.substr(0, pos);
        tokens.assign(s);
        for (tokenizer::iterator it = tokens.begin(); it != tokens.end();
                ++it) {
            words.push_back(g_vocab.add(it->c_str()));
        }

        v.push_back(t);
    }
}

//load ustring into hash_map
//for character ngram
template<typename StringT>
void load_word_from_stream(StringHashTable(ustring) & h,istream& in) {
    h.clear();

    string   s;
    ustring  word;
    int freq;

    while (in) {
        in >> s;
        in >> freq;
        if (!in.eof()) {
            if (g_iconv_from->convert(s,word) == false) {
                cerr << "error:" << s << "can not be converted into unicode" << endl;
            } else {
                h[word] = make_pair(freq,false); //freq and merged flag
            }
        }
    }
}

//load WordString into hash_map
// for word ngram, we map each word into a unique word_id
template<typename StringT>
void load_word_from_stream(StringHashTable(WordString) & h,istream& in) {
    h.clear();

    string     s;
    WordString words;
    int   freq;

    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    boost::char_separator<char> sep(" \t\f\v\r\n");
    tokenizer tokens(s, sep);

    while (getline(in,s)) {
        assert(!in.eof());

        //get N-Gram freq first
        size_t pos = s.rfind(' ');
        if (pos == string::npos)
            pos = s.rfind('\t');
        if (pos == string::npos) {
            cerr << "error:" << s << "find N-Gram frequence in this line" << endl;
            continue;
        }

        freq = atoi(s.substr(pos + 1).c_str());
        words.clear();
        s = s.substr(0, pos);
        tokens.assign(s);
        for (tokenizer::iterator it = tokens.begin(); it != tokens.end(); ++it)
            words.push_back(g_vocab.add(it->c_str()));
        h[words] = make_pair(freq,false); //freq and merged flag
    }
}

//output a word to ostream out
//do unicode->output encoding conversion when output

template<typename StringT>
void output(const StringT& s,int freq,ostream& out) {
    assert(!"do not use this function directly, write your own traits function instead");
}
template<>
//template<typename StringT>
void output(const ustring& ws,int freq,ostream& out) {
    static string s;
    if (g_iconv_to->convert(ws,s))
        out << s << ' ' << freq << endl;
}

template<>
//template<typename StringT>
void output(const WordString& ws,int freq,ostream& out) {
    static string s;
    s.clear();
    for (unsigned i = 0;i < ws.size(); ++i) {
        s += g_vocab[ws[i]];
        s += ' ';
    }
    out << s << freq << endl;
}

template<typename StringT>
void output1(const pair<StringT,int>& x,ostream& out) {
    output(x.first,x.second,out);
}

template<typename StringT>
int do_reduction(const gengetopt_args_info& args_info,istream& in,ostream& out, bool timer) {
    bool sort_result = args_info.sort_flag;
    int freq         = args_info.freq_arg;
    int algo         = args_info.algorithm_arg;

    if (!args_info.algorithm_given) {
        if (args_info.freq_arg == 1)
            algo = 3;
        cerr << "Algorithm parameter not given, using algorithm " << algo << endl;
    }

    if (algo < 1 || algo > 4) {
        cerr << "you input wrong algorithm type:";
        cerr << args_info.algorithm_arg << endl;
        cerr << "valid type is [1 - 4]" << endl;
        return EXIT_FAILURE;
    }

    typedef vector<pair<StringT,int> > StringVector;
    StringVector v;
    typedef typename StringVector::iterator _iterator;
    _iterator end;
    if (algo < 4) {
        load_word_from_stream<StringT>(v,in);

        {
            scoped_ptr<progress_timer> tm;
            if (timer)
                tm.reset(new progress_timer(std::cerr));
            switch (algo) {
                case 1:
                    end = reduction1<_iterator,StringT>(v.begin(),v.end(),freq);
                    break;
                case 2:
                    end = reduction2<_iterator,StringT>(v.begin(),v.end(),freq);
                    break;
                case 3:
                    if (freq != 1) {
                        cerr << "algorithm 3 only accepts --freq 1" << endl;
                        return EXIT_FAILURE;
                    }
                    end = reduction3<_iterator,StringT>(v.begin(),v.end(),freq);
                    break;
            }
        }

        if (sort_result)
            sort(v.begin(),end,lexicographical_cmp_string<StringT>());
        //for_each(v.begin(),end,bind(output1<StringT>(),_1,var(out)));
        for (_iterator it = v.begin();it != end; ++it)
            output(it->first,it->second,out);
    } else {
        if (!args_info.m1_given) {
            cerr << "algorithm 4 need -m option,which is not given,assuming m1=1 by default" << endl;
        }

        typedef StringHashTable(StringT) HashTableT;
        HashTableT ht;
        load_word_from_stream<StringT>(ht,in);

        {
            scoped_ptr<progress_timer> tm;
            if (timer)
                tm.reset(new progress_timer(std::cerr));
            reduction4(ht,args_info.m1_arg,freq);
        }
        if (sort_result) {
            v.clear();
            typename StringHashTable(StringT)::iterator it;
            for (it = ht.begin();it != ht.end(); ++it) {
                if (it->second.second == false) {
                    v.push_back(make_pair(it->first,it->second.first));
                }
            }
            sort(v.begin(),v.end(),lexicographical_cmp_string<StringT>());
            for (_iterator it = v.begin();it != v.end(); ++it)
                output(it->first,it->second,out);
            //for_each(v.begin(),v.end(),output(_1.first,_1.second,out));
        } else {
            for (typename HashTableT::iterator  it = ht.begin();it != ht.end(); ++it)
                if (it->second.second == false)
                    output(it->first,it->second.first,out);
            //            for_each(ht.begin(),ht.end(),
            //                    if_then(_1.second.second == false,
            //                        output(_1.first,_1.second.first,out)));
        }
    }
    return EXIT_SUCCESS;
}

int main(int argc,char* argv[]) {
    try {
        istream* in = &cin;
        ostream* out = &cout;
        ifstream ifile;
        ofstream ofile;
        gengetopt_args_info args_info;

        /* let's call our CMDLINE Parser */
        if (cmdline_parser (argc, argv, &args_info) != 0)
            return EXIT_FAILURE;

        g_iconv_from.reset(new IConvert(args_info.from_arg));
        g_iconv_to.reset(new IConvert(args_info.to_arg));

        if (args_info.inputs_num > 0) {
            if (args_info.inputs_num > 1) {
                cerr << "Warning: more than 1 files given on the command line,";
                cerr << "only use the first file and ignore the rest!" << endl;
            }
            ifile.open(args_info.inputs[0]);
            if (!ifile) {
                cerr << "unable to open file:" << args_info.inputs[0] << " to read!" << endl;
                return EXIT_FAILURE;
            }
            in = &ifile;
        }

        if (args_info.output_given) {
            ofile.open(args_info.output_arg);
            if (!ofile) {
                cerr << "unable to open file:" << args_info.output_arg << " to write!" << endl;
                return EXIT_FAILURE;
            }
            out = &ofile;
        }

        int rc;
        if (args_info.char_flag) { // character ngram mode
            rc = do_reduction<ustring>(args_info,*in,*out, args_info.time_flag);
        } else { // word ngram mode
            rc = do_reduction<WordString>(args_info,*in,*out, args_info.time_flag);
        }

        //cerr << "end at: " << current_time();

        return rc;
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
}
