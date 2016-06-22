/*
 * vi:ts=4:tw=78:shiftwidth=4:expandtab
 * vim600:fdm=marker
 *
 * vocab.cpp  -  vocabulary related routines
 *
 * Copyright (C) 2004 by Zhang Le <ejoy@users.sourceforge.net>
 * Begin       : 08-Apr-2004
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cassert>

#include <fstream>
#include "vocab.hpp"

using namespace std;

word_id g_id_space;
word_id g_id_vt;
word_id g_id_tab;
word_id g_id_peroid;
word_id g_id_question;
word_id g_id_separator;
word_id g_id_intr;
word_id g_id_bos;
word_id g_id_eos;
word_id g_id_special_last;

//add space chars and punct symbol to wordmap
void init_special_id(Vocab& v) {
    assert(v.size() == 0);
    v.add("__NULL_ID__");
    g_id_space     = v.add(" ");
    g_id_tab       = v.add("\t");
    g_id_vt        = v.add("\v");
    g_id_peroid    = v.add(".");
    g_id_question  = v.add("?");
    g_id_separator = v.add(";");
    g_id_intr      = v.add("!");
    g_id_bos       = v.add("BOS");
    g_id_eos       = v.add("EOS");
    g_id_special_last = g_id_eos;
}

// load a vocab saved as plain text file
void load_vocab(const string& file, Vocab& v) {
    ifstream f(file.c_str());
    if (!f) 
        throw runtime_error("Fail to open vocab file to read");

    assert (v.size() == 0);
    init_special_id(v);

    string s;
    while (getline(f,s)) {
        assert(s.find('\n') == string::npos);
        assert(s.find('\r') == string::npos);
        v.add(s);
    }
    cerr << v.size() << " words loaded" << endl;
}

// save vocab to a plain text file, one word per line.
// the words should not contains line separators like \r \n
void save_vocab(const string& file, const Vocab& vocab) {
    ofstream o(file.c_str());
    if (!o)
        throw runtime_error("Unable to open vocab file to write");
    cerr << "Writing vocab to: " << file << endl;

    assert(vocab.size() >= g_id_special_last + 1);

    for (size_t i = g_id_special_last + 1; i < vocab.size(); ++i) {
        o << vocab[i] << endl;
        assert (vocab[i].find('\n') == string::npos);
        assert (vocab[i].find('\r') == string::npos);
    }
}
