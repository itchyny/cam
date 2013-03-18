#!/bin/sh
# touch INSTALL NEWS README COPYING AUTHORS ChangeLog
# autoheader\
#   && aclocal\
#   && automake --add-missing --copy\
#   && autoconf\
#   && ./configure\
#   && make\
#   && make dist
automake --add-missing --copy\
  && autoconf\
  && ./configure\
  && make\
  && make dist


