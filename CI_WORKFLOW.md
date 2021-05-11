# RadeonRays

https://github.com/Radeon-Pro/RadeonRays

## Build

### Requirements

* Cmake 3.12+
* VS 2019
* Windows 10 + latest updates
* MacOS 10.15 (10.14 - build ok)
* Ubuntu latest LTS (18.04)

### Dependencies

* Ubuntu
    * fmt

    ```bash
    $ sudo apt install libfmt-dev
    ```

    * spdlog
    https://github.com/gabime/spdlog

    ```bash
    $ git clone https://github.com/gabime/spdlog.git
    $ cd spdlog && mkdir build && cd build
    $ cmake .. && make -j
    $ sudo make install
    ```

* macOS

    ```bash
    brew install fmt
    brew install spdlog
    ```

* Windows

    1. Install with `vcpkg` and use `CMAKE_PREFIX_PATH` for build
    2. Install form sources

    ```cmd
    git clone https://github.com/gabime/spdlog.git
    mkdir build
    cd build
    cmake ..
    cmake --build . --config Release --target INSTALL
    ```

    ```cmd
    git clone https://github.com/fmtlib/fmt.git
    mkdir build
    cd build
    cmake ..
    cmake --build . --config Release --target INSTALL
    ```
### Build options

```
EMBEDDED_KERNELS=ON
ENABLE_TESTING=ON
CMAKE_BUILD_TYPE=Release
```

* Windows

```cmake
ENABLE_DX12=ON
ENABLE_VULKAN=ON
```

* Lunix

```cmake
ENABLE_VULKAN=ON
CMAKE_CXX_FLAGS="-std=gnu++17"
```

* OSX

```cmake
CMAKE_CXX_FLAGS="-std=c++17"
CMAKE_MACOSX_RPATH=ON
```

## Tests

> :warning: Windows required beta driver with ray-traicing support

[resources](resources) dir is required for tests

### GPUs

* vega56/64
* navi (5700/5700XT)
* nvidia 1080/2070/2080
