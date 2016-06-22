/*
 * vi:ts=4:shiftwidth=4:expandtab
 * vim600:fdm=marker
 *
 * ngramstat.cpp  - Implements Nagao 1994 N-gram extraction algorithm 
 *
 * Copyright (C) 2002 by Zhang Le <ejoy@users.sourceforge.net>
 * Begin       : 30-Oct-2002
 * Last Change : 30-May-2004.
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <cwchar>
#include <cwctype>
#include <algorithm>
#include <stdexcept>
#include <boost/progress.hpp>
#include <boost/ref.hpp>

#include "iconvert.hpp"
#include "mmapfile.hpp"

using namespace std;
using boost::progress_display;

template <typename CharT, typename Traits >
const CharT NGramStat<CharT, Traits>::s_terminal = CharT();

/**
 * Constructor
 *
 * Construct a NGramStat object.
 * @param memory indicate how many memory to use (in MB)
 *        The memory allocated will be freed either by calling
 *        \ref clear() or when the destructor is called.
 * @param file_name_base base name for ptable and ltable file
 *        if empty ltable will be built in memory.
 * @param use_mmap use mmap() call for disk operation.
 */
template <typename CharT, typename Traits>
NGramStat<CharT, Traits>::NGramStat(unsigned memory, 
        const string& file_name_base, bool use_mmap)
:
m_is_use_mmap(use_mmap),
m_filename_base(file_name_base),
m_mem_size(memory),
m_buffersize(0),
m_buffer(0),
m_ptable(0),
m_ltable(0)
{
    alloc_mem();
}

template <typename CharT,typename Traits>
NGramStat<CharT, Traits>::~NGramStat(){
    clear();
}

/**
 * Free all memory allocated to save memory for other processing
 */
template <typename CharT,typename Traits>
void NGramStat<CharT, Traits>::clear() {
    delete[] m_buffer;
    m_buffer = 0;
    delete m_ptable;
    m_ptable = 0;
    delete m_ltable;
    m_ltable = 0;
}

template <typename CharT,typename Traits>
void NGramStat<CharT, Traits>::alloc_mem() {
    assert(!m_buffer);
    assert(!m_ptable);
    assert(!m_ltable);

    cerr << "Try to allocate " << m_mem_size/1024 << " MB for processing" <<
        endl;

    unsigned char_count;

    if (m_filename_base.empty())
        //LTable is in memory
        char_count = m_mem_size * 1024 / (sizeof(CharT) + sizeof(unsigned) +
                sizeof(unsigned char));
    else
        char_count = m_mem_size * 1024 / (sizeof(CharT) + sizeof(unsigned));

    //reserve memory in advance to prevent unexcepted mem allocation
    //when vector grows, which may exceed the system's limit
    m_ptable = new vector<unsigned>;
    m_ptable->reserve(char_count);
    if (m_filename_base.empty()) {
        m_ltable = new vector<unsigned char>;
        m_ltable->reserve(char_count);
    }

    m_buffersize = char_count;
    m_buffer = new CharT[m_buffersize + 1 + m_extra_buffersize];
    m_buffer[m_buffersize + m_extra_buffersize] = s_terminal;

    cerr << "Use "
        << sizeof(CharT) * (m_buffersize + 1 + m_extra_buffersize) / (1024 *
                1024)
        << " MB memory for text buffer" << endl;
    cerr << "Use " << char_count * sizeof(unsigned) / (1024 * 1024)
        << " MB memory for ptable entry" << endl;
    if (m_filename_base.empty())
        cerr << "Use " << char_count * sizeof(char) / (1024 * 1024)
            << " MB memory for ltable entry" << endl;
    if (m_is_use_mmap)
        cerr << "Use mmap() for faster operation if necessary" << endl;
}

/**
 * Indicating the beginning of parsing text stream.
 *
 * This function resets all counter to initial states.
 * It also allocates memory if text buffer is deleted
 * in \ref extract_ngram()
 *
 * This function must be called before \ref parse_buf()
 * The correct steps are:
 * 1. call \ref parse_begin() to initiaze inner text buffers and states
 * 2. call \ref parse_buf() one or more times to to parse text stream(s)
 * 3. call \ref parse_end() to finish parsing text.
 * Now you can extract ngram with \ref extract_ngram()
 */
template <typename CharT,typename Traits>
void NGramStat<CharT, Traits>::parse_begin(){
    if (!m_buffer)
        alloc_mem();

    m_ptable->clear();
    if (m_ltable)
        m_ltable->clear();
    m_start_offset  = 0;
    m_buffer_offset = 0;
    m_last_word_end = 0;
    m_buf_remain.clear();
    m_tempfiles.clear();

    if (!m_filename_base.empty()) {
        m_ngramfile.open((m_filename_base + ".ngram").c_str(),ios::binary);
        if (!m_ngramfile){
            cerr << "unable to open ngramfile:" << m_filename_base
                << ".ngram to write!" << endl;
            throw runtime_error("unable to open ngramfile to write");
        }
    }

}

/**
 * Signal the ending of parsing text stream.
 *
 * Also write/merge PTable/LTable if necessary
 */
template <typename CharT,typename Traits>
void NGramStat<CharT, Traits>::parse_end() {
    cmp_ptable cmp_obj(m_buffer);
    if (m_filename_base.empty()) { //in memory operation
        cerr << "Sorting ptable..." << endl;
        sort(m_ptable->begin(),m_ptable->end(),cmp_obj);
        cerr << "N-gram buffer size(in CharT):" <<(m_buffer_offset + 1)<<endl;
        cerr << "ptable size:" << m_ptable->size() <<endl;
        //LTable is in memory
        calc_ltable();
    } else {
        if (m_buf_remain.size() > 0)
            save_temp_buffer();

        //no disk merme needed {{{
        //save ptable directly
        if (m_tempfiles.empty()) {
            if (m_buffer_offset > m_buffersize) {
                //add ptable nodes in extra buffer
                add_ptable_node(m_last_word_end,m_buffer_offset);
            }
            cerr << "Sorting ptable..." << endl;
            sort(m_ptable->begin(),m_ptable->end(),cmp_obj);
            cerr << "N-gram buffer size(in CharT):" <<(m_buffer_offset + 1)<<endl;
            cerr << "ptable size:" << m_ptable->size() <<endl;

            write_ptable(m_filename_base + ".ptable",0u);
            write_ltable();
            m_ngramfile.write((char*)m_buffer,
                    (m_buffer_offset + 1) * sizeof(CharT)); //includinm the last L'\0'
            m_ngramfile.close();
            clear();
            //}}}
        } else {
            //need disk merge {{{
            //save current buffer and ptable(including extra buffer) to disk
            if (m_buffer_offset > m_buffersize) {
                //add ptable nodes in extra buffer
                add_ptable_node(m_last_word_end,m_buffer_offset);
            }
            sort(m_ptable->begin(),m_ptable->end(),cmp_obj);

            string filename = next_temp_ptable_filename();
            m_tempfiles.push_back(filename);
            write_ptable(filename,m_start_offset);

            m_ngramfile.write((char*)m_buffer,
                    (m_buffer_offset + 1) * sizeof(CharT)); //includinm the last L'\0'
            m_ngramfile.close();

            //delete allocated memory to save memory for
            //disk merge
            clear();

            merge_ptables(); //also calculate ltable here
        }//}}}
    }
}

/**
 * Parse text in buf (CharT) and store it into inner text buffer.
 *
 * update PTable entry for each token and 
 * write temporary ptable to disk if necessary
 */
template <typename CharT,typename Traits>
void NGramStat<CharT, Traits>::parse_buf(const string_type& buf) {
    assert(!buf.empty());

    if (buf.size() > m_buffersize) {
        //damn! the string is too big to be fit in memory
        //ignore it
        cerr << "string too long, try to allocate more memory to solve this problem"  << endl;
        return;
    }

    if (m_buffer_offset + 30 >= m_buffersize + m_extra_buffersize) {
        //text buffer is full
        //save current buffer and ptable to disk
        save_temp_buffer();
        cerr << "Continue parsing..." << endl;
    }


    //copy buf to the end of buffer
    //start from m_buffer_offset
    //may extend to extra buffer
    unsigned start = m_buffer_offset;
    unsigned n = min(buf.size(),m_buffersize + m_extra_buffersize - start - 20);

    Traits::copy(m_buffer + m_buffer_offset,buf.data(),n);
    m_buffer_offset += n;
    m_buffer[m_buffer_offset] = s_terminal;
    if (n < buf.size())
        m_buf_remain.assign(buf,n,buf.size() - n);

    add_ptable_node(start,min(start + buf.size(),m_buffersize));
}

/**
 * save current buffer to disk for disk merging later
 * then move un-processed text to the beginning of text buffer
 */
template <typename CharT,typename Traits>
void NGramStat<CharT, Traits>::save_temp_buffer() {
    assert(!m_ptable->empty());

    if (m_filename_base.empty())
        throw runtime_error("Text Buffer full with no external ngram file name given!");

    cerr << "Sorting temporary ptable for disk merging later..." << endl;
    cmp_ptable cmp_obj(m_buffer);
    sort(m_ptable->begin(),m_ptable->end(),cmp_obj);

    string filename = next_temp_ptable_filename();
    m_tempfiles.push_back(filename);
    //write temp ptable
    write_ptable(filename,m_start_offset);
    m_ptable->clear();

    //not include the last L'\0' when writing
    //temp ngram buffer
    m_ngramfile.write((char*)m_buffer,m_last_word_end * sizeof(CharT));

    //copy the rest N-Gram to the beginning of the buffer
    //wcscpy(m_buffer,m_buffer + m_last_word_end);
    size_t n = Traits::length(m_buffer + m_last_word_end);
    Traits::copy(m_buffer,m_buffer + m_last_word_end,n);
    m_buffer[n] = s_terminal;
    if (m_buf_remain.size() > 0) {
        Traits::copy(m_buffer + n,m_buf_remain.data(),m_buf_remain.size());
        m_buffer[n + m_buf_remain.size()] = s_terminal;
        m_buf_remain.clear();
    }

    m_start_offset += m_last_word_end;
    m_last_word_end = 0;
    m_buffer_offset = Traits::length(m_buffer);

    add_ptable_node(0,m_buffer_offset);
}

/**
 * calculate in memory LTable
 */
template <typename CharT,typename Traits>
void NGramStat<CharT, Traits>::calc_ltable(){
    assert(m_buffer);
    assert(m_ptable);
    assert(m_ltable && m_ltable->empty());
    assert(m_filename_base.empty());

    //set first entry to zero
    m_ltable->push_back(0);

    unsigned char count;
    vector<unsigned>& ptable = *m_ptable;
    for (unsigned i = 1;i < ptable.size(); ++i) {
        assert(ptable[i - 1] < m_buffer_offset && ptable[i] < m_buffer_offset);

        count = calc_common_words(&m_buffer[ptable[i- 1]],&m_buffer[ptable[i]]);
        //m_ltable->push_back(calc_common_words(&m_buffer[ptable->at(i - 1)],&m_buffer[ptable->at(i]]));
        m_ltable->push_back(count);
    }
}

/**
 * calculate and write a ltable into ltable file(*.ltable)
 * this ltable is calculated from in memory ptable and ngram table
 */
template <typename CharT,typename Traits>
void NGramStat<CharT, Traits>::write_ltable() const {
    assert(m_buffer);
    assert(m_ptable);
    assert(!m_filename_base.empty());

    string name = m_filename_base + ".ltable";

    cerr << "Writing ltable: " << name << endl;

    ofstream file(name.c_str(),ios::binary);
    if (!file) {
        cerr << "unable to open ltable file:" << name << "to write!" << endl;
        throw runtime_error("unable to open ltable for writing");
    }

    unsigned char count = 0;
    //set first entry to zero
    file.write((char*)&count,sizeof(unsigned char));

    vector<unsigned>& ptable = *m_ptable;
    for (unsigned i = 1;i < ptable.size(); ++i) {
        assert(ptable[i - 1] < m_buffer_offset && ptable[i] < m_buffer_offset);

        count = calc_common_words(&m_buffer[ptable[i - 1]],
                &m_buffer[ptable[i]]);
        file.write((char*)&count,sizeof(unsigned char));
    }
    file.close();
}

/**
 * calculate and write ltable into ltable file(*.ltable)
 * this ltable is calculated from the given ptable and ngram table
 */
template <typename CharT,typename Traits>
void NGramStat<CharT, Traits>::write_ltable(const CharT* ngram_table,const string ptable_filename) const{
    assert(!m_filename_base.empty());

    string ltable_filename = m_filename_base + ".ltable";
    cerr << "Writing ltable:" << ltable_filename << endl;

    ofstream file(ltable_filename.c_str(),ios::binary);
    if (!file) {
        cerr << "unable to open ltable file:" << ltable_filename << "to write!" << endl;
        throw runtime_error("unable to open ltable for writing");
    }

    unsigned char count = 0;

    //set first entry to zero
    file.write((char*)&count,sizeof(unsigned char));

    if (m_is_use_mmap) {
        unsigned* ptable     = 0;
        unsigned  ptable_size = 0;

        //mmap ptable file for calculating ltable
        MmapFile fm(ptable_filename.c_str());
        if (!fm.open())
            throw runtime_error("mmap call failed");

        ptable = (unsigned*)fm.addr();
        ptable_size = fm.size();

        unsigned* ptable_end = ptable + ptable_size / sizeof(unsigned) - 1;

        for (const unsigned* i = ptable;i < ptable_end;++i) {
            count = calc_common_words(&ngram_table[*i],
                    &ngram_table[*(i+1)]);
            file.write((char*)&count,sizeof(unsigned char));
        }
        fm.close();
    } else {
        /*
           int fd;
           fd = open(ptable_filename.c_str(),O_RDONLY);
           if (fd == -1) {
           perror("unable to open ptable file");
           exit(EXIT_FAILURE);
           }
           */

        FILE* fp;
        fp = fopen(ptable_filename.c_str(),"rb");
        if (fp == NULL) {
            perror("unable to open ptable file");
            exit(EXIT_FAILURE);
        }

        /*
           ifstream fs;
           fs.open(ptable_filename.c_str(),ios::binary);
           if (!fs) {
           perror("unable to open ptable file");
           exit(EXIT_FAILURE);
           }
           */

        struct stat st;
        if (stat(ptable_filename.c_str(),&st) == -1) {
            perror("unable to stat ptable file size");
            exit(EXIT_FAILURE);
        }

        struct {
            unsigned offset1;
            unsigned offset2;
        }offset;

        unsigned end = st.st_size - sizeof(unsigned);

        for (unsigned pos = 0;pos < end; pos += sizeof(unsigned) ) {
            /*
               if (lseek(fd,pos,SEEK_SET) == -1 ||
               read(fd,&offset,sizeof(offset)) != sizeof(offset)) {
               perror("error reading ptable file");
               exit(EXIT_FAILURE);
               }
               */
            if (fseek(fp,pos,SEEK_SET) == -1 ||
                    fread(&offset,sizeof(offset),1,fp) != 1) {
                perror("error reading ptable file");
                exit(EXIT_FAILURE);
            }
            /*
               cerr << fs.seekg(pos,ios::beg);
               cerr << "read:"<< fs.read(&offset,sizeof(offset));
               */

            count = calc_common_words(&ngram_table[offset.offset1],
                    &ngram_table[offset.offset2]);
            file.write((char*)&count,sizeof(unsigned char));
        }

        file.close();
    }
}

/**
 * write in memory ptable into ptable file(*.ptable)
 */
template <typename CharT,typename Traits>
void NGramStat<CharT, Traits>::write_ptable(const string& name,unsigned start_offset) const{
    assert(m_ptable);

    cerr << "Writing ptable: " << name << endl;

    ofstream file(name.c_str(),ios::binary);
    if (!file) {
        cerr << "unable to open ptable file:" << name << "to write!" << endl;
        return;
    }

    unsigned offset;
    vector<unsigned>& ptable = *m_ptable;
    for (unsigned i = 0;i < ptable.size(); ++i) {
        offset = ptable[i] + start_offset;
        file.write((char*)&offset,sizeof(unsigned));
        /* for debug
        ustring ws(&g_buffer[offset],255);
        string s;
        g_iConvert->convert(ws,s);
        cerr << offset << ":" << s << endl;
        */
    }
    file.close();
}

/**
 *fill ptable entry in m_buffer[start,end)
 */
template <typename CharT,typename Traits>
void NGramStat<CharT, Traits>::add_ptable_node(unsigned start,unsigned end){
    for (unsigned j = start;j < end;++j) {
        m_ptable->push_back(j);
        m_last_word_end = j + 1;
    }
}

struct PTable_attr {
    FILE* m_fp;
    int          m_fd;

    //mmap attr
    unsigned*    mp_addr;    //mmap addr
    unsigned*    mp_end;     //end of mmap addr
    unsigned*    mp_pos;     //current pos

    //file attr
    off_t        m_pos;      //current pos in file
    unsigned     m_offset;   //current ngram  offset in ngram file
    off_t        m_end;      //end offset
    unsigned     m_size;
};

/**
 * merge several temp ptable file into one
 * and save it to m_filename_base + ".ptable"
 * Merging temporary ptable file using mmap() is faster than normal
 * file operation. But some system (such as Win32) has a memory limitation
 * of 2G. So if you want to process large file(1-2 Gb), do not use mmap().
 * However, we still need to mmap ngram file for fast random access.
 * That is to say, you can not process file larger than 2G on Win32 platform.
 * Use Linux instead.
 */
template <typename CharT,typename Traits>
void NGramStat<CharT, Traits>::merge_ptables() {
    string ptable_filename = m_filename_base + ".ptable";
    vector<PTable_attr > ptables(m_tempfiles.size());

    for (unsigned i = 0;i < ptables.size(); ++i) {
        ptables[i].m_fd     = -1;

        ptables[i].mp_addr  = 0;
        ptables[i].mp_end   = 0;
        ptables[i].mp_pos   = 0;

        ptables[i].m_pos    = 0;
        ptables[i].m_offset = 0;
        ptables[i].m_end    = 0;
        ptables[i].m_size   = 0;
    }

    unsigned files_to_merge = 0;

    vector<MmapFile*> fm_objs;

    for (unsigned i = 0;i < m_tempfiles.size(); ++i, ++files_to_merge) {
        if (m_is_use_mmap) {
            //open m_tempfiles using mmap()
            MmapFile* fm = new MmapFile(m_tempfiles[i].c_str());
            if (fm_objs[i]->open()) {
                ptables[i].mp_addr = (unsigned*)fm_objs[i]->addr();
                ptables[i].m_size = fm_objs[i]->size();
                ptables[i].mp_pos = ptables[i].mp_addr;
                ptables[i].mp_end = ptables[i].mp_addr + ptables[i].m_size /
                    sizeof(unsigned);
                fm_objs.push_back(fm);
            } else {
                cerr << "unable to mmap() file:" << m_tempfiles[i] << endl;
                delete fm;
                throw runtime_error("unable to mmap() temp ptable file");
            }
        } else {
            //ptables[i].m_fd = open(m_tempfiles[i].c_str(),O_RDONLY);
            ptables[i].m_fp = fopen(m_tempfiles[i].c_str(),"rb");
            if (ptables[i].m_fp == NULL) {
                perror("unable to open temporary ptable file");
                throw runtime_error("unable to open temporary ptable file");
            }
            ptables[i].m_fd = fileno(ptables[i].m_fp);

            if (ptables[i].m_fd == -1) {
                perror("unable to open temporary ptable file");
                throw runtime_error("unable to open temporary ptable file");
            }

            struct stat st;
            if (fstat(ptables[i].m_fd,&st) == -1) {
                perror("unable to stat temporary ptable file size");
                throw runtime_error("unable to stat temporary ptable file size");
            }

            ptables[i].m_size = st.st_size;
            ptables[i].m_end  = st.st_size;
        }
    }

    //mmap ngram table
#if !defined (HAVE_SYSTEM_MMAP)
#error the ptable merging code needs mmap(2) support, which is missing on this system
#endif
    CharT* ngram_table = 0;
    unsigned ngram_table_size = 0;
    MmapFile fm_ngram(string(m_filename_base + ".ngram").c_str());
    if (!fm_ngram.open()) {
        cerr << "unable to mmap:" << m_filename_base + ".ngram" << endl;
        throw runtime_error("unable to mmap() ngram file");
    }
    ngram_table = (CharT*)fm_ngram.addr();
    ngram_table_size = fm_ngram.size();

    ngram_table_size /= sizeof(CharT);

    ofstream ptable_file(ptable_filename.c_str(),ios::binary);
    if (!ptable_file) {
        cerr << "unable to write to ptable file:" << ptable_filename << endl;
        throw runtime_error("unable to open ptable file to write");
    }

    //now merging
    cerr << "Merging " << files_to_merge << " temporary ptables..." << endl;

    assert (files_to_merge == ptables.size());
    int rc;

    if (m_is_use_mmap) { //{{{
        PTable_attr* min;
        while (files_to_merge > 0) {
            min = 0;
            //set min to first ptable needed to merge
            for (unsigned i = 0;i < ptables.size(); ++i ) {
                if (ptables[i].mp_pos < ptables[i].mp_end) {
                    min = &ptables[i];
                    break;
                }
            }

            assert(min);

            for (unsigned i = 0;i < ptables.size(); ++i ) {
                PTable_attr& attr = ptables[i];
                if (attr.mp_pos < attr.mp_end && min != &ptables[i]) {
//                    rc = wcsncmp((const wchar_t*)&ngram_table[*attr.mp_pos],
//                            (const wchar_t*)&ngram_table[*(min->mp_pos)],255);
                    rc = Traits::compare((const CharT*)&ngram_table[*attr.mp_pos],
                            (const CharT*)&ngram_table[*(min->mp_pos)],255);
                    if (rc < 0 || (rc == 0 && *attr.mp_pos < *(min->mp_pos)))
                        min = &ptables[i];
                }
            }

            assert(min);

            ptable_file.write((char*)min->mp_pos,sizeof(unsigned));

            if (++(min->mp_pos) >= min->mp_end) {
//                mmap files will be closed on clean up
//                if (min->m_size > 0)
//                    munmap_file(min->mp_addr,min->m_size);
                --files_to_merge;
            }
        }//}}}
    } else { //do not use mmap() {{{
        PTable_attr* min;
        while (files_to_merge > 0) {
            min = 0;
            //set min to first ptable needed to merge
            for (unsigned i = 0;i < ptables.size(); ++i ) {
                if (ptables[i].m_pos < ptables[i].m_end) {
                    min = &ptables[i];
                    break;
                }
            }

            assert(min);

            if (fseek(min->m_fp,min->m_pos,SEEK_SET) == -1 ||
                    fread(&min->m_offset,sizeof(unsigned),1,min->m_fp) != 1 ) {
                perror("error reading temporary ptable file");
                exit(EXIT_FAILURE);
            }
            /*
            if (lseek(min->m_fd,min->m_pos,SEEK_SET) == -1 ) {
                perror("error reading temporary ptable file");
                exit(EXIT_FAILURE);
            }
            */
            //int size;
            //size = read(min->m_fd,&min->m_offset,sizeof(unsigned));
            /*
            size = fread(&min->m_offset,sizeof(unsigned),1,min->m_fp);
            if (size != 1)
                cerr << "read return:"<<size<< endl;
                */
            /*
            if (lseek(min->m_fd,min->m_pos,SEEK_SET) == -1 ||
                    read(min->m_fd,&min->m_offset,sizeof(unsigned)) != sizeof(unsigned)) {
                perror("error reading temporary ptable file");
                exit(EXIT_FAILURE);
            }
            */

            for (unsigned i = 0;i < ptables.size(); ++i ) {
                PTable_attr& attr = ptables[i];
                if (attr.m_pos < attr.m_end && min != &ptables[i]) {
                    if (fseek(attr.m_fp,attr.m_pos,SEEK_SET) == -1 ||
                            fread(&attr.m_offset,sizeof(unsigned),1,attr.m_fp) != 1) {
                        perror("error reading temporary ptable file");
                        exit(EXIT_FAILURE);
                    }
                    //size = read(attr.m_fd,&attr.m_offset,sizeof(unsigned));
                    /*
                    size = fread(&attr.m_offset,sizeof(unsigned),1,attr.m_fp);

                    if (size != 1)
                        cerr << "read2 return:"<<size<< endl;
                        */
                    /*
                       if (lseek(attr.m_fd,attr.m_pos,SEEK_SET) == -1 ||
                       read(attr.m_fd,&attr.m_offset,sizeof(unsigned)) != sizeof(unsigned)) {
                       perror("error reading temporary ptable file");
                       exit(EXIT_FAILURE);
                       }
                       */

//                    rc = wcsncmp((const wchar_t*)&ngram_table[attr.m_offset],
//                            (const wchar_t*)&ngram_table[min->m_offset],255);
                    rc = Traits::compare((const CharT*)&ngram_table[attr.m_offset],
                            (const CharT*)&ngram_table[min->m_offset],255);
                    if (rc < 0 || (rc == 0 && attr.m_offset < min->m_offset)) {
                        min = &ptables[i];
                    }
                }
            }

            assert(min);

            ptable_file.write((char*)&min->m_offset,sizeof(unsigned));

            min->m_pos += sizeof(unsigned);

            if (min->m_pos >= min->m_end) {
                if (min->m_size > 0)
                    if (fclose(min->m_fp) == EOF)
                    //if (close(min->m_fd) == -1)
                        perror("unable to close temporary ptable file");
                --files_to_merge;
            }
        }
    } //}}}

    ptable_file.close();

    write_ltable(ngram_table,ptable_filename);

    //clean up
    // munmap_file(ngram_table,ngram_table_size);

    for (size_t i = 0;i < fm_objs.size(); ++i)
        delete fm_objs[i];

    for (unsigned i = 0;i < m_tempfiles.size();++i)
        if (remove(m_tempfiles[i].c_str())) {
            string s = "unable to remove file:";
            s += m_tempfiles[i];
            perror(s.c_str());
        }
}

//return next temp ptable filename
template <typename CharT,typename Traits>
string NGramStat<CharT, Traits>::next_temp_ptable_filename() const {
    return next_temp_filename("NGramStat");
}
/**
 * calculate common words(max 255) from the beginning of s1 and s2
 */
template <typename CharT,typename Traits>
unsigned char NGramStat<CharT, Traits>::calc_common_words(const CharT* s1,const CharT* s2) const {
    assert(*s1 && *s2);

    unsigned char count = 0;
        //simply counting every char including blanks and punctuations
        while (count < 255 && *s1 != s_terminal && *s2 != s_terminal &&
                *s1++ == *s2++)
            ++count;
    return count;
}

/**
 * parse a string in given encoding
 */
//template <typename CharT,typename Traits>
//void NGramStat<CharT, Traits>::parse(const string& str, const string& encoding/*"UTF-8"*/){
//    IConvert iconv(encoding);
//    ustring ws;
//    if (iconv.convert(str,ws) == false)
//        throw runtime_error("fail to convert input string into UNICODE");
//
//    parse_begin();
//    parse_buf(ws);
//    parse_end();
//}

/**
 * parse several files in given encoding
 */
//template <typename CharT,typename Traits>
//void NGramStat<CharT, Traits>::parse(const vector<string>& files, const string& encoding/*"UTF-8"*/){
//    IConvert iconv(encoding);
//    string s;
//    ustring ws;
//
//    parse_begin();
//
//    for (unsigned i = 0;i < files.size(); ++i) {
//        ifstream f(files[i].c_str());
//        if (!f)
//            throw runtime_error("unable to open file for parsing");
//
//        cerr << "parsing file: " << files[i] << endl;
//        while (getline(f,s)) {
//            if (s.empty())
//                continue;
//
//            if (iconv.convert(s,ws) == false) {
//                cerr << '\"' << s << "\" can not be converted into UNICODE" << endl;
//                continue;
//            }
//
//            parse_buf(ws);
//        }
//    }
//
//    parse_end();
//}

/**
 * parse a string(array) of CharT
 */
//template <typename CharT,typename Traits>
//void NGramStat<CharT, Traits>::parse(const string_type& str) {
//    parse_begin();
//    parse_buf(str);
//    parse_end();
//}
//
/**
 * fetch a N gram from the beginning of s
 * if no N-gram found at s,set ngram to L""
 */
template <typename CharT,typename Traits>
void NGramStat<CharT, Traits>::fetch_ngram(unsigned N,CharT* s,string_type& ngram) const{
    ngram.clear();

    //Simply counting every char including punctuations and blanks
    while (N > 0 && *s) {
        ngram += *s++;
        --N;
    }

    if (N != 0)
        ngram.clear();
}

/**
 * this function behaves essentially like the above function
 * except it fetches N to M-gram(N<=M) from the beginning of s
 * and store them in a vector of ustring
 * if N-gram is not found in s,store a L""
 */
template <typename CharT,typename Traits>
void NGramStat<CharT, Traits>::fetch_ngrams(unsigned N,unsigned M,CharT* s,vector<NGram>& ngrams) const{
    assert(N <= M);
    assert(ngrams.size() >= M + 1);

    string_type  ngram;
    unsigned n;

    for (n = N;n <= M; ++n)
        ngrams[n].m_text.clear();

    //Simply counting every char including punctuations and blanks
    for (n = 1; n <= M && *s;++n) {
        ngram += *s++;
        if (n >= N) {
            ngrams[n].m_text = ngram;
            ngrams[n].m_count = 1;
        }
    }
}

/**
 * fetch a entry of ptable from m_ptable, mmaped ptable or ptable file
 * depending on access method used
 */
template <typename CharT,typename Traits>
unsigned NGramStat<CharT, Traits>::ptable_entry(unsigned* ptable,FILE* fp,unsigned pos) const {
    if (ptable) {
        assert(m_is_use_mmap);
        return ptable[pos];
    } else if (fp) {
        unsigned entry;
        if (fseek(fp,pos * sizeof(unsigned),SEEK_SET) == -1 ||
                fread(&entry,sizeof(unsigned),1,fp) != 1) {
            perror("error reading ptable file");
            throw runtime_error("error when reading ptable file");
        }
        /*
           if (lseek(g_ptable_fd,pos * sizeof(unsigned),SEEK_SET) == -1 ||
           read(g_ptable_fd,&entry,sizeof(unsigned)) != sizeof(unsigned)) {
           perror("error reading ptable file");
           exit(EXIT_FAILURE);
           }
           */
        return entry;
    } else {
        assert(m_ptable);
        return (*m_ptable)[pos];
    }
}

/**
 * fetch a entry of ltable from m_ltable, mmaped ltable or ltable file
 * depending on access method used
 */
template <typename CharT,typename Traits>
unsigned char NGramStat<CharT, Traits>::ltable_entry(unsigned char* ltable,FILE* fp,unsigned pos) const {
    if (ltable) {
        assert(m_is_use_mmap);
        return ltable[pos];
    } else if (fp) {
        unsigned char entry;
        /*
           if (lseek(g_ltable_fd,pos * sizeof(unsigned char),SEEK_SET) == -1 ||
           read(g_ltable_fd,&entry,sizeof(unsigned char)) != sizeof(unsigned char)) {
           perror("error reading ltable file");
           exit(EXIT_FAILURE);
           }
           */
        if (fseek(fp,pos * sizeof(unsigned char),SEEK_SET) == -1 ||
                fread(&entry,sizeof(unsigned char),1,fp) != 1) {
            perror("error reading ltable file");
            throw runtime_error("error when reading ltable file");
        }

        return entry;
    } else {
        assert(m_ltable);
        return (*m_ltable)[pos];
    }
}

/**
 * Extract N-M ngram with frequency >= freq from NGramStat object.
 *
 * Extract N-gram to M-gram from the inner text buffer.
 *
 * @param N smallest N-gram to extract
 * @param M largest M-gram to extract (M >= N)
 * @param freq only N-grams whose frequency >= freq are extracted
 * @param output a helper function object for extracting N-grams
 *        the first argument is the ngram the second argument is the count of
 *        the ngram
 * TODO: more document on OutputFunc
 */
template <typename CharT,typename Traits>
void NGramStat<CharT, Traits>::extract_ngram(unsigned N, unsigned M, unsigned freq,
        OutputFunc& output) {
    CharT*         ngramtable      = m_buffer;
    unsigned       ngramtable_size = m_buffer_offset;
    unsigned*      ptable          = 0;
    unsigned       ptable_size     = m_ptable->size();
    unsigned char* ltable          = 0;
    unsigned       ltable_size     = m_ltable ? m_ltable->size():0;
    FILE*          ptable_fp       = 0;
    FILE*          ltable_fp       = 0;
    //int            ptable_fd;
    //int            ltable_fd;

    string ngram_filename  = m_filename_base + ".ngram";
    string ptable_filename = m_filename_base + ".ptable";
    string ltable_filename = m_filename_base + ".ltable";

    vector<MmapFile*> fm_objs;

    //init: mmap ngram table,ptable and ltable if necessary {{{
    if (!m_filename_base.empty()) {
        //free memory(if any)
        clear();

        MmapFile* fm = new MmapFile(ngram_filename.c_str());
        if (!fm->open()) {
            cerr << "could not mmap file:" << ngram_filename << endl;
            throw runtime_error("unable to mmap ngram table file when extracting NGram");
        }
        fm_objs.push_back(fm);
        ngramtable = (CharT*)fm->addr();
        ngramtable_size = fm->size();
        assert(ngramtable);
        ngramtable_size /= sizeof(CharT);

        if (m_is_use_mmap) {
            MmapFile* fm = new MmapFile(ptable_filename.c_str());
            if (!fm->open()) {
                cerr << "could not mmap file:" << ptable_filename << endl;
                throw runtime_error("unable to mmap ptable file when extracting NGram");
            }
            fm_objs.push_back(fm);
            ptable = (unsigned*)fm->addr();
            ptable_size = fm->size();
            assert(ptable);
            ptable_size /= sizeof(unsigned);

            MmapFile* fm2 = new MmapFile(ltable_filename.c_str());
            if (!fm2->open()) {
                cerr << "could not mmap file:" << ltable_filename << endl;
                throw runtime_error("unable to mmap ltable file when extracting NGram");
            }
            fm_objs.push_back(fm2);
            ltable = (unsigned char*)fm2->addr();
            ltable_size = fm2->size();
            assert(ltable);
            ltable_size /= sizeof(unsigned char);
        } else {
            ptable_fp = fopen(ptable_filename.c_str(),"rb");
            if (ptable_fp == NULL) {
                perror("unable to open ptable file");
                throw runtime_error("unable to open ptable file when extracting NGram");
            }
            struct stat st;
            if (stat(ptable_filename.c_str(),&st) == -1) {
                perror("unable to stat ptable file size");
                throw runtime_error("unable to stat ptable file size when extracting NGram");
            }
            ptable_size = st.st_size / sizeof(unsigned);

            ltable_fp = fopen(ltable_filename.c_str(),"rb");
            if (ltable_fp == NULL) {
                perror("unable to open ltable file");
                throw runtime_error("unable to open ltable file when extracting NGram");
            }

            if (stat(ltable_filename.c_str(),&st) == -1) {
                perror("unable to stat ltable file size");
                throw runtime_error("unable to stat ltable file size when extracting NGram");
            }

            ltable_size = st.st_size / sizeof(unsigned char);
            /*  old implementation{{{
                ptable_fd = open(ptable_filename.c_str(),O_RDONLY);
                if (ptable_fd == -1) {
                perror("unable to open ptable file");
                exit(EXIT_FAILURE);
                }

                struct stat st;
                if (fstat(ptable_fd,&st) == -1) {
                perror("unable to stat ptable file size");
                exit(EXIT_FAILURE);
                }

                ptable_size = st.st_size;

                ltable_fd = open(g_ltable_filename.c_str(),O_RDONLY);
                if (ltable_fd == -1) {
                perror("unable to open ltable file");
                exit(EXIT_FAILURE);
                }

                if (fstat(ltable_fd,&st) == -1) {
                perror("unable to stat ltable file size");
                exit(EXIT_FAILURE);
                }

                ltable_size = st.st_size;
                }}}*/
        }

        assert(ptable_size == ltable_size);
    }//}}}

    if (false) { //dump ngram table and ltable {{{
        /*
        assert(m_is_use_mmap);

        unsigned char* l = g_ltable;
        unsigned* p = g_ptable;
        unsigned* pend = g_ptable + g_ptable_size/sizeof(unsigned);
        wchar_t ws[26];
        string s;
        ws[25] = L'\0';

        cerr << "Ngramtable_size:" << ngramtable_size/sizeof(wchar_t) << endl;
        cerr << "PTable_size:" << g_ptable_size/sizeof(unsigned) << endl;
        cerr << "LTable_size:" << g_ltable_size/sizeof(unsigned char) << endl;

        while (p < pend) {
            assert(*p < ngramtable_size/sizeof(wchar_t));
            wcsncpy(ws,&ngramtable[*p],25);
            if(g_iConvert->convert(ws,s)) {
                cout << int(*l) << "\t==>" << s << endl;
            }
            else {
                cerr << "error occurs when calling iconv()" << endl;
            }
            ++p;
            ++l;
        }
        */
        //}}}

    } else if (N == M) { //extract N-gram with a fixed N extraction algorithm {{{
        //which is a little faster than N-M extract algorithm below
        string_type  ngram;
        unsigned i;
        unsigned count = 1;
        unsigned size  = ltable_size;
        progress_display* progress = 0;
        if (true)
            progress = new progress_display(size,cerr);

        //set first ngram
        fetch_ngram(N,&ngramtable[ptable_entry(ptable,ptable_fp,0)],ngram);
        for (i = 1;i < size; ++i) {
            if (progress)
                ++(*progress);

            if (ltable_entry(ltable,ltable_fp,i) >= N) {
                ++count;
            } else {
                //this node is a n-gram which n < N
                //so we print out current ngram (n >=N )and its count
                if (count >= freq && !ngram.empty())
                    output(ngram,count);

                //fetch new ngram
                fetch_ngram(N,&ngramtable[ptable_entry(ptable,ptable_fp,i)],ngram);
                count = 1;
            }
        }//for

        //output last entry
        if (count >= freq && !ngram.empty())
            output(ngram,count);

        if (progress) {
            delete progress;
            cerr << endl;
        }
        //}}}

    } else { //extract N-gram in range[N,M] {{{
        vector<NGram> ngrams(M + 1); //so we can access the Nth-gram with ngrams[N]
        unsigned i;
        unsigned j;
        unsigned l;  //store ltable[l]:the co-occurence count of the two adjancent ngrams
        unsigned size = ltable_size;
        progress_display* progress = 0;
        if (true)
            progress = new progress_display(size,cerr);

        //get first N-Mngrams
        fetch_ngrams(N,M,&ngramtable[ptable_entry(ptable,ptable_fp,0)],ngrams);
        for (i = 1;i < size; ++i) {
            if (progress)
                ++(*progress);

            l = ltable_entry(ltable,ltable_fp,i);
            if (l < N) {
                //this ltable node is a n-gram which n < N
                //so we print out current ngrams (n in [N,M]) and their counts
                for (j = N;j <= M;++j)
                    if (ngrams[j].m_count >= freq && !ngrams[j].m_text.empty())
                        output(ngrams[j].m_text,ngrams[j].m_count);

                //and fetch new ngrams
                fetch_ngrams(N,M,&ngramtable[ptable_entry(ptable,ptable_fp,i)],ngrams);
            } else if (l >= M){
                //increasing N-gram count in [N,M]
                for (j = N;j <= M;++j)
                    ++ngrams[j].m_count;
            } else {
                // N <= l < M
                //increasing N-gram count in [N,l]
                for (j = N;j <= l;++j)
                    ++ngrams[j].m_count;

                //need to output ngrams[l + 1,M]
                for (j = l + 1;j <= M;++j)
                    if (ngrams[j].m_count >= freq && !ngrams[j].m_text.empty())
                        output(ngrams[j].m_text,ngrams[j].m_count);
                fetch_ngrams(l + 1,M,&ngramtable[ptable_entry(ptable,ptable_fp,i)],ngrams);
            }
        }//for

        //output last entry
        for (j = N;j <= M;++j)
            if (ngrams[j].m_count >= freq && !ngrams[j].m_text.empty())
                output(ngrams[j].m_text,ngrams[j].m_count);

        if (progress) {
            delete progress;
            cerr << endl;
        }
    } //}}}

    // clean up {{{
    for (size_t i = 0; i < fm_objs.size(); ++i)
        delete fm_objs[i];

    if (!m_filename_base.empty()) {
        if (!m_is_use_mmap) {
            assert(ptable_fp && ltable_fp);
            fclose(ptable_fp);
            fclose(ltable_fp);
        }
    }
    //}}}
}

//helper output function object
//struct DefaultOutputHelper{
//    DefaultOutputHelper(ostream& os,const string& encoding)
//    :os(&os),iconv(encoding){}
//
//    void operator()(const ustring& ws,unsigned count) const {
//        static string s;
//        if (iconv.convert(ws,s))
//            (*os) << s << '\t' << count << endl;
//    }
//    private:
//    ostream* os;
//    mutable IConvert iconv;
//};
//
///**
// * this function behaves essentially like above function(just a wrapper)
// * except it output the result N-Grams to a ostream using given
// * encoding
// */
//template <typename CharT,typename Traits>
//void NGramStat<CharT, Traits>::extract_ngram(unsigned N, unsigned M, unsigned freq,
//        std::ostream& os/*std::cout*/,const string& encoding/*"UTF-8"*/) {
//    DefaultOutputHelper out(os,encoding);
//    OutputFunc f = boost::ref(out);
//    extract_ngram(N,M,freq,f);
//}

