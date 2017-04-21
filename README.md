# Summary
Radeon Rays is ray intersection acceleration library provided by AMD which makes the most of the hardware and allows for efficient ray queries. Three backends support a range of use cases.

# Description
Radeon Rays has three backends, 
- OpenCL
- Vulkan
- Embree

OpenCL uses GPUs and CPUs that support at least OpenCL 1.2
Vulkan supports GPUs with Vulkan 1.0 or greater
Embree uses Intels Optimized CPU ray casting software for x86 and x64 devices

The source tree consist of the following subdirectories:

- Radeon Rays: library binaries

- App: Standalone sample/application featuring Radeon Rays OpenCL to implement a path tracer.

# Preliminaries
## System requirements
The library is cross-platform and the following compilers are supported:

- Visual Studio 2015

- Xcode 4 and later

- GCC 4.8 and later

- Python (for --embed_kernels option only)

- [Anvil](https://github.com/GPUOpen-LibrariesAndSDKs/Anvil) for Vulkan backend only

- [Embree](https://github.com/embree/embree) for Embree backend only

- AMD OpenCL APP SDK 2.0+ is also required for the standalone app build.  

## Set up OpenCL
Set environmental variable.  GPU_MAX_ALLOC_PERCENT = 100. This is necessary to allocate a large buffers.

## Set up Vulkan
Anvil is set as a submodule and will be downloaded by using `git submodule update --init --recursive` from the command line.
Some gui clients (github app for example) may do this automatically for you

## Multiple Backends
You can either choose a particular backend (OpenCL, Vulkan or Embree) or compile any combination of them and pick at run-time. By default OpenCL only will be compiled in (see Options below to enable other backends).
At runtime OpenCL devices will appear first, then Vulkan devices (if enabled) with the Embree device last (if enabled).

If the default behaviour is not what you want, an API call `IntersectionApi::SetPlatform( backend )` takes a backend argument bitfield allows you to specify exactly which backends device will be enumurated.

## Build                                                                                       

### Windows
- Create Visual Studio 2015 Solution

`./Tools/premake/win/premake5.exe vs2015`

### OSX
- Install Homebrew

`/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"`

- Install OpenImageIO

`brew install homebrew/science/openimageio`

- Create Xcode project

`./Tools/premake/osx/premake5 xcode4`

- Alternatively use gmake version

`./Tools/premake/osx/premake5 gmake`

`make config=release_x64`

### Linux
on Ubuntu:
install complimentary libraries:

`sudo apt-get install g++`

install build dependencies:

`sudo apt-get install libopenimageio-dev libglew-dev freeglut3-dev libglfw3-dev`

Also make sure you have the `opencl-dev` headers installed. Then create the Makefile:

`./Tools/premake/linux64/premake5 gmake`

`make config=release_x64`

### Options
Available premake options:
 - `--package` will package the library for a binary release in `dist` folder.  
 example of usage :
 `./Tools/premake/win/premake5.exe --package`

- `--use_embree` will enable the embree backend. Embree device will be the last one in IntersectionApi device list.
 example of usage :
 `./Tools/premake/win/premake5.exe --use_embree vs2015`

- `--use_vulkan` will enable the vulkan backend.

- `--use_opencl` will enable the OpenCL backend. If no other --use_ option is provided, this is the default

- `--shared_calc` will build Calc (Compute Abstraction Layer) as a shared object. This means RadeonRays library does not directly depend on OpenCL and can be used on the systems where OpenCL is not available (with Embree backend). 

## Run

## Run standalone app
 - `export LD_LIBRARY_PATH=<Radeon Rays_SDK path>/Radeon Rays/lib/x64/:${LD_LIBRARY_PATH}`
 - `cd App`
 - `../Bin/Release/x64/App64`

Possible command line args:

- `-p path` path to mesh/material files
- `-f file` mesh file to render
- `-w` set window width
- `-h` set window height
- `-nb num` run in GI mode and calculate num bounces of light
- `-ns num` limit the number of samples per pixel
- `-cs speed` set camera movement speed
- `-cpx x -cpy y -cpz z` set camera position
- `-tpx x -tpy y -tpz z` set camera target
- `-fd distance` set camera focus distance in meters
- `-fl length` set lens focal length in meters (default 35mm)
- `-a aperture` set lens aperture value in meters (values > 0 switches camera model from pinhole to physical)
- `-interop [0|1]` disable | enable OpenGL interop (enabled by default, might be broken on some Linux systems)
- `-config [gpu|cpu|mgpu|mcpu|all]` set device configuration to run on: single gpu (default) | single cpu | all available gpus | all available cpus | all devices

The app only supports loading of pure triangle .obj meshes. The list of supported texture formats:

- png
- bmp
- jpg
- gif
- exr
- tex
- dds (limited support)
- tga


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
## Vulkan

 - Currently the public Anvil has a [bug](https://github.com/GPUOpen-LibrariesAndSDKs/Anvil/issues/3) that causes a crash on exit. Its already been fixed in an internal branch which will which will move to public when its cooked and ready. For now just comment out the free of the layout manager in Anvil::BasePipelineManager::Pipeline::release_vulkan_objects()

## Windows

## OSX

## Linux

 - If <CL/cl.h> is missing try to specify OpenCL SDK location.
 - If your are experiencing problems creating your CL context with a default config chances are CL-GL interop is broken on your system, try running the sample app with -interop 0 command line option (expect performance drop). 
 
AMD:
`export $AMDAPPSDKROOT=<SDK_PATH>`
NVIDIA:
`export $CUDA_PATH=<SDK_PATH>`
