#! /usr/bin/env python
# vi:ts=4:tw=78:shiftwidth=4:expandtab
# vim600:fdm=marker
#
# boostheaders.py  -  extract all boost related header files in a
# specific directory
# usage: boostheaders.py src [-o boost -b /usr/local/include/boost]
#
# Copyright (C) 2003 by Zhang Le <ejoy@users.sourceforge.net>
# Begin       : 11-Sep-2003
# Last Change : 12-Mar-2004.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public
# License along with this program.
#/

import sys
import popen2
import sets
import os
import stat
import re
import shutil
from optparse import OptionParser

cxx_ext = [".c", ".C", ".cxx", ".cpp", ".c\+\+", ".cc", ".CC", 
       ".h", ".H", ".hxx", ".hpp", ".hh",
       ".F", ".fpp", ".FPP"]

header_re = ''
for x in cxx_ext:
    header_re += '.*\%s$|' % x
header_re = re.compile(header_re[:-1])

boost_include_re = '^[ \t]*#[ \t]*\d+[ \t]*"(.*boost.*)"[ \t]*\d+.*$'
boost_include_re = re.compile(boost_include_re)

boost_inc_path = '/usr/local/include'
inc_path = None

platform_specific_headers = [ 
'config/platform/cygwin.hpp', 
'config/platform/linux.hpp', 
'detail/lwm_pthreads.hpp', 
]

#def get_boost_headers(boost_dir):
#    h = []
#    for x in os.walk(boost_dir):
#        for f in x[2]:
#            h.append(os.path.join(x[0], f))
#    if boost_dir[-1] != '/':
#        boost_dir += '/'
#    l = len(boost_dir)
#    h = [ x[l:] for x in h]
#    return h

def extract_boost_headers(f):
    "extract all boost headers referenced in f"
    h = []
    if inc_path:
        cmd = 'g++ -E -I%s -I%s %s ' % (inc_path, boost_inc_path, f)
    else:
        cmd = 'g++ -E -I%s %s ' % (boost_inc_path, f)
    print 'running %s' % cmd
    p = popen2.Popen3(cmd, True)
    stdout = p.fromchild.readlines()
    stderr = p.childerr.readlines()
    exit =  p.wait()
    if exit != 0:
        raise 'cpp failed with %s' % ''.join(stderr)
    for s in stdout:
        m = boost_include_re.match(s)
        if m:
            h.append(m.groups()[0])
    return h

def process_dir(dir):
    "return all boost headers in files located in dir (and subdir)"
    cxx_files = []
    for x in os.walk(dir):
        for f in x[2]:
            if header_re.match(f):
                cxx_files.append(os.path.join(x[0], f))
    h = []
    for f in cxx_files:
        h += extract_boost_headers(f)
    return h

def copy_boost_headers(headers, output):
    "copy all boost related headers to dir output"
    if not os.access(output, os.R_OK):
        os.makedirs(output)
    global boost_inc_path
    if boost_inc_path[-1] != '/':
        boost_inc_path += '/boost/'
    else:
        boost_inc_path += 'boost/'
    l = len(boost_inc_path)
    for h in sets.Set(headers):
        dirname = '%s/%s/' % (output, os.path.dirname(h[l:]))
        if not os.access(dirname, os.R_OK):
            os.makedirs(dirname)
        shutil.copy(h, dirname)
        os.chmod('%s/%s' % (dirname, os.path.basename(h)), 0666)
def main ():

    # parsing options{{{
    usage = "usage: %prog [options] dirs"
    parser = OptionParser(usage)
    parser.add_option("-b", "--boost", type="string", default="/usr/local/include", 
            help="local boost include directory, default=[/usr/local/include]")
    parser.add_option("-I", type="string", help="set additional include path for cpp")
    parser.add_option("-o", "--output", type="string", default="boost", 
            help="output directory, default=[boost]")
    (options, args) = parser.parse_args()
    #}}}

    if len(args) < 1:
        print >> sys.stderr, 'argument missing!'
        parser.print_usage()
        sys.exit(1)

    global boost_inc_path
    boost_inc_path = options.boost
    global inc_path
    inc_path = options.I

    boost_headers = []
    for dir in args:
        boost_headers += process_dir(dir)

    # add some additional headers required on other platforms
    boost_headers += [ '%s/boost/%s' % (boost_inc_path, x) for x in platform_specific_headers ]

    print 'Copying boost headers to %s' % options.output
    copy_boost_headers(boost_headers, options.output)

if __name__ == "__main__":
    main()

