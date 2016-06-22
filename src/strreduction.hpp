/*
 * vi:ts=4:shiftwidth=4:expandtab
 * vim600:fdm=marker
 *
 * strreduction.hpp  -  Four Statistical Substring Reduction Algorithms
 *
 * Implementation of four Statistical Substring Reduction
 * described in (LV 2003): Research of E-Chunk Acquisition and Application in
 * Machine Translation. Ph.D. dissertation, Northeastern Univ. P.R.China.
 *
 * Also see (Zhang et al. 2003): A Statistical Approach to Extract Chinese
 * Chunk Candidates from Large Corpora for an application.
 *
 * Copyright (C) 2002 by Zhang Le <ejoy@users.sourceforge.net>
 * Begin       : 05-Aug-2002
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

#ifndef STRREDUCTION_H
#define STRREDUCTION_H

#include <cstdlib>
#include <string>
#include <utility>
#include <cwchar>
#include <algorithm>
#include <functional>
#if defined(_STLPORT_VERSION)
#include <hash_map>
using std::hash_map;
#else
#include <ext/hash_map>
using namespace __gnu_cxx;
#endif

#include <boost/concept_check.hpp>

using namespace std;
using namespace boost;

template<typename StringT>
struct zero_item {
    bool operator()(pair<StringT,int>& lhs) const {
        return lhs.second  < 0;
    }
};

template<typename StringT>
struct StringHasher {
    size_t operator()(const StringT& s) const {
        unsigned long __h = 0;
        for (unsigned i = 0;i < s.size();++i)
            __h ^= (( __h << 5) + (__h >> 2) + s[i]);

        return size_t(__h);
    }
};

template<typename StringT>
struct cmp_string {
    bool operator()(const pair<StringT,int>& w1,const pair<StringT,int>& w2) const {
        return w1.first < w2.first;
    }
};


//hash: string-->pair<freq,merged-flag>
//template<typename StringT>
#define StringHashTable(StringT) hash_map<StringT,pair<int,bool>,StringHasher<StringT> > 

//check whether s1 is s2's left substring
//note:s1 is not itself's substring
template<typename StringT>
inline bool is_lsubstr(const StringT& s1,const StringT& s2) {
    typedef typename StringT::traits_type Traits;
    return s1.size() < s2.size() && Traits::compare(s1.data(),s2.data(),s1.size()) == 0;
}

//check whether s1 is s2's substring
//note:s1 is not itself's substring
template<typename StringT>
inline bool is_substr(const StringT& s1,const StringT& s2) {
    return s1.size() < s2.size() && s2.find(s1) != StringT::npos;
}

//first substring reduction algorithm (traditional algorithm, very slow)
//complexity:O(N^2)
template <typename RandomAccessIterator,typename StringT>
RandomAccessIterator reduction1(RandomAccessIterator first,RandomAccessIterator last, int f0) {
    function_requires< RandomAccessIteratorConcept<RandomAccessIterator> >();
    //function_requires< IntegerConcept<a->second> >();
    size_t n = last - first;
    size_t i,j;
    RandomAccessIterator a = first;
    for (i = 0;i < n; ++i) {
        for (j = 0;j < n; ++j)  {
            if ((abs(a[i].second) - abs(a[j].second)< f0) && is_substr(a[i].first,a[j].first)) {
                if (a[i].second > 0)
                    a[i].second = - a[i].second;
                break;
            }
        }
    }
    //return remove_if(first,last,bind(std_functor(select2nd<pair<StringT,int> >()),_1) < 0);
    return remove_if(first,last,zero_item<StringT>());
}

//second substring reduction algorithm (pretty fast)
//complexity:O(Nlog(N))
template <typename RandomAccessIterator,typename StringT>
RandomAccessIterator reduction2(RandomAccessIterator first,RandomAccessIterator last, int f0) {
    function_requires< RandomAccessIteratorConcept<RandomAccessIterator> >();

    size_t n = last - first;
    size_t i;
    //vector<bool> flags(n,false);
    RandomAccessIterator a;

    //step 1
    sort(first,last,cmp_string<StringT>());
    a = first;
    //step 2
    for (i = 0; i < n - 1;++i)
        if ((abs(a[i].second) - abs(a[i + 1].second) < f0) && is_lsubstr(a[i].first,a[i + 1].first))
            if (a[i].second > 0)
                a[i].second = - a[i].second;
            //a[i].second = -abs(a[i].second);
    //step 3
    for (i = 0; i < n ;++i)
        reverse(a[i].first.begin(),a[i].first.end());
    //step 4
    sort(first,last,cmp_string<StringT>());
    a = first;
    //step 5
    for (i = 0; i < n - 1;++i)
        if ((abs(a[i].second) - abs(a[i + 1].second) < f0) && is_lsubstr(a[i].first,a[i + 1].first))
            if (a[i].second > 0)
                a[i].second = - a[i].second;
            //a[i].second = -abs(a[i].second);
    //step 6
    for (i = 0; i < n ;++i)
        reverse(a[i].first.begin(),a[i].first.end());

    //step 8
    //return remove_if(first,last,bind(std_functor(select2nd<pair<StringT,int> >()),_1) < 0);
    return remove_if(first,last,zero_item<StringT>());
    //remove_if(first,last,merged_str<StringT>());
//    remove_if(first,last,
//            bind(select2nd<pair<StringT,
//                int> >,
//                _1) < 0);
}

//3th substring reduction algorithm
//complexity:O(Nlog(N))
//a little faster than above algorithm 2
template <typename RandomAccessIterator,typename StringT>
RandomAccessIterator reduction3(RandomAccessIterator first,RandomAccessIterator last, int f0) {
    function_requires< RandomAccessIteratorConcept<RandomAccessIterator> >();

    size_t n = last - first;
    size_t i,delete_pos;
    //vector<bool> flags(n,false);
    RandomAccessIterator a;

    //step 1
    sort(first,last,cmp_string<StringT>());
    a = first;

    //step 2
    //we delete a element by moving the following elements up
    //find the first element's position to be deleted
    for (i = 0;i < n - 1;++i) {
        if ((abs(a[i].second) - abs(a[i + 1].second) < f0) && is_lsubstr(a[i].first,a[i + 1].first))
            break;
    }

    delete_pos = i;

    for (;i < n - 1;++i) {
        //check if we need delete a element
        if (!((abs(a[i].second) - abs(a[i + 1].second) < f0) && is_lsubstr(a[i].first,a[i + 1].first)))
            a[delete_pos++] = a[i];
    }

    a[delete_pos] = a[n - 1];

    n = delete_pos + 1;

    //step 3
    for (i = 0; i < n ;++i)
        reverse(a[i].first.begin(),a[i].first.end());

    //step 4
    sort(a,a + n,cmp_string<StringT>());

    //step 5
    //we delete a element by moving the following elements up
    //find the first element's position to be deleted
    for (i = 0;i < n - 1;++i) {
        if ((abs(a[i].second) - abs(a[i + 1].second) < f0) && is_lsubstr(a[i].first,a[i + 1].first))
            break;
    }

    delete_pos = i;

    for (;i < n - 1;++i) {
        //check if we need delete a element
        if (!((abs(a[i].second) - abs(a[i + 1].second) < f0) && is_lsubstr(a[i].first,a[i + 1].first)))
            a[delete_pos++] = a[i];
    }

    a[delete_pos] = a[n - 1];

    n = delete_pos + 1;

    //step 6
    for (i = 0; i < n ;++i)
        reverse(a[i].first.begin(),a[i].first.end());
    //step 8
    return a + n;
}


//4th substring reduction algorithm
//need a hashtable
//complexity:O(N)
template <typename StringT>
void reduction4(StringHashTable(StringT)& ht,unsigned m1,int f0) {
    unsigned i;
    size_t len;
    const StringT* word;
    typename StringHashTable(StringT)::const_iterator it;
    typename StringHashTable(StringT)::iterator it2;

    for (it = ht.begin();it != ht.end(); ++it) {
        //find all substring whose length is in the range: [m1,word.size())
        word = &(it->first);

        for (len = m1;len < word->size();++len) {
            for (i = 0;i + len <= word->size(); ++i) {
                it2 = ht.find(word->substr(i,len));
                if (it2 != ht.end() && (it2->second.first - it->second.first < f0))
                    it2->second.second = true;
            }
        }
    }
}

#endif /* ifndef STRREDUCTION_H */
