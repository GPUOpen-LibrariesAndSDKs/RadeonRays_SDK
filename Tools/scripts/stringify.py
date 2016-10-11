#!/usr/bin/env python
import sys
import os
import re

visited = set()

def printfile(filename, dir):
    visited.add(filename)
    fh = open(dir + "/" + filename)
    for line in fh.readlines():
        a = line.strip('\r\n')
        inl = re.search("#include\s*<.*/(.+)>", a)
        if inl:
            if inl.group(1) not in visited:
                printfile( inl.group(1), dir)
        else:
            print( '"' + a.replace("\\","\\\\").replace("\"", "\\\"") + ' \\n"\\' )

def stringify(filename, dir, varname, typest):
    visited.clear()
    print( 'static const char g_'+varname+"_"+typest+'[]= \\' )
    printfile(filename, dir)
    print( ';' )

argvs = sys.argv

if len(argvs) == 4:
    dir = argvs[1]
    ext = argvs[2]
    typest = argvs[3]

files = os.listdir(dir)

print("/* This is an auto-generated file. Do not edit manually*/\n")

for file in files:
    if file.find(ext) == -1:
        continue

    stringify(file, dir, file.replace(ext, ""),typest)
    
