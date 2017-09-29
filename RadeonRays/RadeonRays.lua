project "RadeonRays"

    if( _OPTIONS["static_library"]) then
        kind "StaticLib"
    else
        kind "SharedLib"
    end

    location "../RadeonRays"
    includedirs { "./include", "../Calc/inc" }

    if _OPTIONS["shared_calc"] then
        defines {"CALC_IMPORT_API"};
        if os.is("windows") then
            characterset ("MBCS")
        else
            links {"dl"}
        end
    else
    defines {"CALC_STATIC_LIBRARY"}
        links {"Calc"}
    if _OPTIONS["use_opencl"] then
           links {"CLW"}
    end
    end

    if _OPTIONS["enable_raymask"] then
        defines {"RR_RAYMASK"}
    end

    defines {"EXPORT_API"}

    files { "../RadeonRays/**.h", "../RadeonRays/**.cpp","../RadeonRays/src/kernels/CL/**.cl", "../RadeonRays/src/kernels/GLSL/**.comp"}

    excludes {"../RadeonRays/src/device/embree*"}
    if os.is("macosx") then
        buildoptions "-std=c++11 -stdlib=libc++"
        filter { "kind:SharedLib", "system:macosx" }
        linkoptions { '-Wl,-install_name', '-Wl,@loader_path/%{cfg.linktarget.name}' }

    elseif os.is("linux") then
        buildoptions "-std=c++11 -fPIC"
        linkoptions {"-Wl,--no-undefined"}

        --get API version from header.
        local handle = io.popen("grep -r RADEONRAYS_API_VERSION include/radeon_rays.h | cut -d \" \" -f 3")
        local lib_version = (handle:read("*a")):gsub("\n", "")
        handle:close()

        --specify soname for linker
        configuration {"x64", "Debug"}
            linkoptions {"-Wl,-soname,libRadeonRays64D.so." .. lib_version}
        configuration {"x32", "Debug"}
            linkoptions {"-Wl,-soname,libRadeonRaysD.so." .. lib_version}
        configuration {"x64", "Release"}
            linkoptions {"-Wl,-soname,libRadeonRays64.so." .. lib_version}
        configuration {"x32", "Release"}
            linkoptions {"-Wl,-soname,libRadeonRays.so." .. lib_version}
        configuration{}


        --replacing lib by soft link
        postbuildcommands {"mv $(TARGET) $(TARGET)." .. lib_version}
        postbuildcommands {"ln -s `basename $(TARGET)." .. lib_version .. "` $(TARGET)"}
    end

    configuration {}

    if _OPTIONS["embed_kernels"] then
        defines {"RR_EMBED_KERNELS=1"}

        if _OPTIONS["use_vulkan"] then
            os.execute( "python ../Tools/scripts/stringify.py " ..
                                os.getcwd() .. "../RadeonRays/src/kernels/GLSL/ "  ..
                                ".comp " ..
                                "vulkan " ..
                                 "> ./src/kernelcache/kernels_vk.h"
                                )
            print ">> RadeonRays: VK kernels embedded"
        end

        if _OPTIONS["use_opencl"] then
            os.execute( "python ../Tools/scripts/stringify.py " ..
                                os.getcwd() .. "/../RadeonRays/src/kernels/CL/ "  ..
                                ".cl " ..
                                "opencl " ..
                                 "> ./src/kernelcache/kernels_cl.h"
                                )
            print ">> RadeonRays: CL kernels embedded"
        end
    end

    if _OPTIONS["use_embree"] then
        files {"../RadeonRays/src/device/embree*"}
        defines {"USE_EMBREE=1"}
        includedirs {"../3rdParty/embree/include"}

        configuration {"x32"}
            libdirs { "../3rdParty/embree/lib/x86"}
        configuration {"x64"}
            libdirs { "../3rdParty/embree/lib/x64"}
        configuration {}

        if os.is("macosx") then
            links {"embree.2"}
        elseif os.is("linux") then
            buildoptions {"-msse3"}
            links {"embree"}
        elseif os.is("windows") then
            links {"embree"}
        end
    end

    if _OPTIONS["enable_raymask"] then
        configuration {}
        defines {"RR_RAY_MASK"}
    end

    if _OPTIONS["safe_math"] then
        configuration {}
        defines { "USE_SAFE_MATH" }
    end

    if _OPTIONS["use_vulkan"] then
        local vulkanSDKPath = os.getenv( "VK_SDK_PATH" );
        if vulkanSDKPath == nil then
            vulkanSDKPath = os.getenv( "VULKAN_SDK" );
        end
        if vulkanSDKPath ~= nil then
            configuration {"x32"}
            libdirs { vulkanSDKPath .. "/Bin32" }
            configuration {"x64"}
            libdirs { vulkanSDKPath .. "/Bin" }
            configuration {}
        end
        if os.is("macosx") then
            --no Vulkan on macOs need to error out TODO
        elseif os.is("linux") then
            libdirs { vulkanSDKPath .. "/lib" }
            links { "Anvil",
                    "vulkan",
                    "pthread"}
        elseif os.is("windows") then
            links {"Anvil"}
            links {"vulkan-1"}
        end
    end

    configuration {"x32", "Debug"}
        targetdir "../Bin/Debug/x86"
    configuration {"x64", "Debug"}
        targetdir "../Bin/Debug/x64"
    configuration {"x32", "Release"}
        targetdir "../Bin/Release/x86"
    configuration {"x64", "Release"}
        targetdir "../Bin/Release/x64"
    configuration {}
