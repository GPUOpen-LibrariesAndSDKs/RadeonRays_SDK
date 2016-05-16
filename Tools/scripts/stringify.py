#!/usr/bin/env python
import sys
import os
import re

def printfile(filename, dir):
    fh = open(dir + "/" + filename)
    for line in fh.readlines():
        a = line.strip('\r\n')
        inl = re.search("#include\s*<.*/(.+)>", a)
        if inl:
            printfile (inl.group(1), dir)
        else:
            print '"' + a.replace("\\","\\\\").replace("\"", "\\\"") + ' \\n"\\'

def stringify(filename, dir, varname):
    print 'static const char cl_'+varname+'[]= \\'
    printfile(filename, dir)
    print ';'

argvs = sys.argv

if len(argvs) == 2:
    dir = argvs[1]

files = os.listdir(dir)

for file in files:
    if file.find('.cl') == -1:
        continue
    stringify(file, dir, file.replace(".cl", ""))
    
