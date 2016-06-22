/*
 * vi:ts=4:tw=78:shiftwidth=4:expandtab
 * vim600:fdm=marker
 *
 * vocab.h  -  vocabulary related routines.
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

#ifndef VOCAB_H
#define VOCAB_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string>
#include "itemmap.hpp"

typedef ItemMap<std::string> Vocab;
typedef Vocab::id_type word_id;

// pre-defined word ids, they are gloabl vars!
// TODO: move these ids into a struct
extern word_id g_id_space;
extern word_id g_id_vt;
extern word_id g_id_tab;
extern word_id g_id_peroid;
extern word_id g_id_question;
extern word_id g_id_separator;
extern word_id g_id_intr;
extern word_id g_id_bos;
extern word_id g_id_eos;
extern word_id g_id_special_last;
void init_special_id(Vocab& v); 
void load_vocab(const std::string& file, Vocab& v);
void save_vocab(const std::string& file, const Vocab& v);
#endif /* ifndef VOCAB_H */

