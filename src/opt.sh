#!/bin/sh
gengetopt -i extractngram.ggo -u -F extractngram_cmdline
gengetopt -i text2ngram.ggo -u -F text2ngram_cmdline
gengetopt -i strreduction.ggo -u -F strreduction_cmdline

