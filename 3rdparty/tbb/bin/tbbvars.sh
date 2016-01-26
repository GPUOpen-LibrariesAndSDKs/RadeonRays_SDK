#!/bin/sh
#
# Copyright 2005-2015 Intel Corporation.  All Rights Reserved.
#
# This file is part of Threading Building Blocks. Threading Building Blocks is free software;
# you can redistribute it and/or modify it under the terms of the GNU General Public License
# version 2  as  published  by  the  Free Software Foundation.  Threading Building Blocks is
# distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See  the GNU General Public License for more details.   You should have received a copy of
# the  GNU General Public License along with Threading Building Blocks; if not, write to the
# Free Software Foundation, Inc.,  51 Franklin St,  Fifth Floor,  Boston,  MA 02110-1301 USA
#
# As a special exception,  you may use this file  as part of a free software library without
# restriction.  Specifically,  if other files instantiate templates  or use macros or inline
# functions from this file, or you compile this file and link it with other files to produce
# an executable,  this file does not by itself cause the resulting executable to be covered
# by the GNU General Public License. This exception does not however invalidate any other
# reasons why the executable file might be covered by the GNU General Public License.


export TBBROOT="SUBSTITUTE_INSTALL_DIR_HERE" 
if [ "$1" == "libc++" ]; then
    library_directory="/libc++"
else
    library_directory=""
fi
if [ -z "${LIBRARY_PATH}" ]; then 
    LIBRARY_PATH="${TBBROOT}/lib${library_directory}"; export LIBRARY_PATH 
else
    LIBRARY_PATH="${TBBROOT}/lib${library_directory}:$LIBRARY_PATH"; export LIBRARY_PATH 
fi
if [ -z "${DYLD_LIBRARY_PATH}" ]; then 
    DYLD_LIBRARY_PATH="${TBBROOT}/lib${library_directory}"; export DYLD_LIBRARY_PATH 
else
    DYLD_LIBRARY_PATH="${TBBROOT}/lib${library_directory}:$DYLD_LIBRARY_PATH"; export DYLD_LIBRARY_PATH 
fi
if [ -z "${CPATH}" ]; then 
    CPATH="${TBBROOT}/include"; export CPATH 
else 
    CPATH="${TBBROOT}/include:$CPATH"; export CPATH 
fi 
