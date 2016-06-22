/*
 * vi:ts=4:shiftwidth=4:expandtab
 * vim600:fdm=marker
 *
 * itemmap.cpp  -  description
 *
 * Copyright (C) 2002 by Zhang Le <ejoy@users.sourceforge.net>
 * Begin       : 31-Dec-2002
 * Last Change : 15-Sep-2003.
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

#include <cassert>

#include <fstream>
#include <stdexcept>

using namespace std;

//ItemMap<T, HashFun, typename EqualKey>::ItemMap(const string& filename)
//{
//    load(filename);
//}

template<typename T, typename HashFun, typename EqualKey>
ItemMap<T, HashFun, EqualKey>::~ItemMap() {
    clear();
}

template<typename T, typename HashFun, typename EqualKey>
void ItemMap<T, HashFun, EqualKey>::clear() {
    m_index.clear();
    m_hashdict.clear();
}

/**
 * load feature map of T from a binary file.
 */
//void ItemMap<T, HashFun, typename EqualKey>::load(const string& filename) {
//    assert(!filename.empty());
//
//    ifstream in(filename.c_str(),ios::binary);
//    if (!in)
//        throw runtime_error("unable to open featmap file to read");
//
//    load(in);
//}

/*
template<>
void ItemMap<string>::load(istream& is) {
    clear();
    size_t n;
    is.read((char*)&n,sizeof(n));

    char buf[4000];
    size_t len;
    string feat;
    id_type  index = 0;
    for (size_t i = 0;i < n; ++i) {
        is.read((char*)&len,sizeof(len));
        if (len >= 4000)
            throw runtime_error("buffer overflow when loading");
        is.read((char*)buf,sizeof(char) * len);
        buf[len] = '\0';
        //feat = buf;
        m_hashdict[buf] = index;
        m_index.push_back(buf);
        ++index;
    }
}
*/

/**
 * save feature map of T to given file one word per line
 * the feature should can be write through << operator
 */
/*
template<>
void ItemMap<string>::save(ostream& os) {
    size_t n = size();
    os.write((char*)&n, sizeof(n));

    for (size_t i = 0;i < n; ++i) {
        string& s = m_index[i];
        size_t len = s.size();
        os.write((char*)&len,sizeof(len));
        os.write((char*)s.c_str(),sizeof(char) * len);
    }
}
*/

/**
 * save feature map of T to a binary file
 */
//void ItemMap<T, HashFun, typename EqualKey>::save(const string& filename) {
//    assert(!filename.empty());
//
//    ofstream out(filename.c_str(),ios::binary);
//    if (!out)
//        throw runtime_error("unable to open wordmap file to write");
//    save(out);
//}

template<typename T, typename HashFun, typename EqualKey>
typename ItemMap<T, HashFun, EqualKey>::id_type ItemMap<T, HashFun, EqualKey>::add(const T& f) {
    typename hash_map_type::const_iterator it = m_hashdict.find(f);
    if (it != m_hashdict.end())
        return it->second;

    id_type id = m_index.size();
    m_hashdict[f] = id;
    m_index.push_back(f);
    return id;
}

