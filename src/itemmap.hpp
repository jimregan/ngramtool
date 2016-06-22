/*
 * vi:ts=4:shiftwidth=4:expandtab
 * vim600:fdm=marker
 *
 * itemmap.hpp  -  generic item <--> id map class
 *
 * Copyright (C) 2002 by Zhang Le <ejoy@users.sourceforge.net>
 * Begin       : 31-Dec-2002
 * Last Change : 14-Oct-2003.
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

#ifndef ITEMMAP_H
#define ITEMMAP_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>
#include <vector>
#include <string>
#include <functional>

#include <ext/hash_map>
using namespace __gnu_cxx;
namespace __gnu_cxx {
    struct hash<std::string> {
        size_t operator()(const std::string& s) const {
            unsigned long __h = 0;
            for (unsigned i = 0;i < s.size();++i)
                __h ^= (( __h << 5) + (__h >> 2) + s[i]);

            return size_t(__h);
        }
    };
};
// #endif

template <typename T, typename HashFunc = hash<T>, typename EqualKey = std::equal_to<T> >
class ItemMap {
    public:
        typedef T      item_type;
        typedef size_t id_type;
        typedef hash_map <T, id_type, HashFunc, EqualKey>         hash_map_type;
        // static const size_t null_id = ~(ItemMap::id_type)0;
        static const size_t null_id;
        typedef typename std::vector<T>::iterator       iterator;
        typedef typename std::vector<T>::const_iterator const_iterator;

        ItemMap(){}

        ~ItemMap();

        iterator begin() { return m_index.begin(); }

        iterator end() { return m_index.end(); }

        const_iterator begin() const { return m_index.begin(); }

        const_iterator end() const { return m_index.end(); }

        size_t size() const { return m_index.size(); }

        bool empty() const { return m_index.empty(); }

        void clear();

        /**
         * add a item into dict return new item's id
         * if the item already exists simply return its id
         */
        id_type add(const T& f);

        /**
         * get a item's id (index in dict)
         * if the item does not exist return null_id
         */
        id_type id(const T& f) const {
            typename hash_map_type::const_iterator it = m_hashdict.find(f);
            if (it == m_hashdict.end())
                return null_id;
            return it->second;
            // return has_item(f) ? m_hashdict[f] : null_id;
        }

        bool has_item(const T& f) const {
            return m_hashdict.find(f) != m_hashdict.end();
        }

        const T& operator[](id_type id) const {
            return m_index[id];
        }

    private:
        mutable hash_map_type m_hashdict;
        std::vector<T>        m_index;
};

template <typename T, typename HashFunc, typename EqualKey >
const size_t ItemMap<T, HashFunc, EqualKey>::null_id =
~(ItemMap<T, HashFunc, EqualKey>::id_type)0; // -1 is null_id
#include "itemmap.tcc"
#endif /* ifndef ITEMMAP_H */
