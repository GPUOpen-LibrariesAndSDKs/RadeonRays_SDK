# Summary
FireRays is ray intersection acceleration library provided by AMD which makes the most of AMD hardware and allows for efficient ray queries independently of the generation of underlying AMD hardware.

# Description
The source tree consist of the following subdirectories:

 - FireRays: library binaries

- App: Standalone application featuring FireRays library

# Preliminaries
## System requirements
The library is cross-platform and the following compilers are supported:

- Visual Studio 2015

- Xcode 4 and later

- GCC 4.8 and later

- Python (for --embed_kernels option only)

AMD OpenCL APP SDK 2.0+ is also required for the standalone app build.  

## Set up OpenCL
- Set environmental variable.  GPU_MAX_ALLOC_PERCENT = 100. This is necessary to allocate a large buffers.

## Build                                                                                       

### Windows
- Create Visual Studio 2015 Solution

`./premake/win/premake5.exe vs2015`

### OSX
- Install Homebrew

`/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"`

- Install OpenImageIO

`brew install homebrew/science/openimageio`

- Create Xcode project

`./premake/osx/premake5 xcode4`

- Alternatively use gmake version

`./premake/osx/premake5 gmake`

`make config=release_x64`

### Linux
on Ubuntu:
install complimentary libraries:

`sudo apt-get install g++`

install OpenImageIO:

`sudo apt-get install libopenimageio-dev`

Create Makefile:

`./premake/linux64/premake5 gmake`

`make config=release_x64`

### Options

## Run

## Run standalone app
 - `export LD_LIBRARY_PATH=<FireRays_SDK path>/FireRays/lib/x64/:${LD_LIBRARY_PATH}`
 - `cd App`
 - `../Bin/Release/x64/App64`

Possible command line args:

- `-p path` path to mesh/material files
- `-f file` mesh file to render
- `-w` set window width
- `-h` set window height
- `-ao r` run in ambient occlusion mode and set AO radius
- `-nb num` run in GI mode and calculate num bounces of light
- `-cpx x -cpy y -cpz z` set camera position
- `-tpx x -tpy y -tpz z` set camera target


# Hardware  support

The library has been tested on the following hardware and OSes:

## Linux
 - Ubuntu Linux 14.04
 - AMD FirePro driver 15.201: W9100, W8100, W9000, W7000, W7100, S9300x2, W5100
 - AMD Radeon driver 15.302: R9 Nano, R9 Fury X, R9 290
 - NVIDIA driver 352.79: GeForce GTX970, Titan X

## Windows
 - Windows 7/8.1/10
 - AMD FirePro driver 15.201: W9100, W8100, W9000, W7000, W7100, S9300x2, W5100
 - AMD Radeon driver 16.4: R9 Nano, R9 Fury X, R9 290, Pro Duo
 - NVIDIA driver 364.72: GeForce GTX970, Titan X

## OSX
 - OSX El Capitan 10.11.4
 - Mac Pro (Late 2013) AMD FirePro D500 x2
 - Macbook Pro Retina 13" (Early 2013) Intel HD 4300
 - Macbook 12" (Early 2015) Intel HD 5300

---
# Known Issues

## Windows

## OSX

## Linux

If <CL/cl.h> is missing try to specify OpenCL SDK location.
AMD:
`export $AMDAPPSDKROOT=<SDK_PATH>`
NVIDIA:
`export $CUDA_PATH=<SDK_PATH>`
