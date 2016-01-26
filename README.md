# Summary
FireRays is ray intersection acceleration library provided by AMD which makes the most of AMD hardware and allows for efficient ray queries independently of the generation of underlying AMD hardware.

# Description
The source tree consist of the following subdirectories:

 - FireRays: library binaries

- App: Standalone application featuring FireRays library

# Preliminaries
## System requirements
The library is cross-platform and the following compilers are supported:

- Visual Studio 2012 and later

- Xcode 4 and later

- GCC 4.8 and later

- Python (for --embed_kernels option only)

AMD OpenCL APP SDK 2.0+ is also required for the standalone app build.  

## Set up OpenCL
- Set environmental variable.  GPU_MAX_ALLOC_PERCENT = 100. This is necessary to allocate a large buffers. 

## Build                                                                                       

### Windows
- Create Visual Studio 2013 Solution

`./premake/win/premake5.exe vs2013`

### OSX
- Create Xcode project

`./premake/osx/premake4 xcode4` 

### Linux
on Ubuntu:
install complimentary libraries:

`sudo apt-get install g++`

- Create Makefile

`./premake/linux64/premake4 gmake`

`make config=release64`

### Options
Specify --embed_kernels option to premake to get rid of kernel files dependency in standalone ap. This option forces premake to embed kernel sources into app binary module.
This option requires python to be installed.

## Run

## Run standalone app

`./Bin/Release/x64/App64`

Possible command line args:

- `-p path` path to mesh/material files
- `-f file` mesh file to render
- `-w` set window width
- `-h` set window height
- `-ao r` run in ambient occlusion mode and set AO radius
- `-nb num` run in GI mode and calculate num bounces of light
- `-cpx x -cpy y -cpz z` set camera position
- `-tpx x -tpy y -tpz z` set camera target

---
# Known Issues

## Windows 

## OSX 

## Linux
    