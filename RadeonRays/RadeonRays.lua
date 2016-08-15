project "RadeonRays"
    kind "SharedLib"
    location "../RadeonRays"
    includedirs { "./include", "../Calc/inc" }
    links {"Calc"}
    defines {"EXPORT_API"}
    files { "../RadeonRays/**.h", "../RadeonRays/**.cpp"}
    
    if not os.is("macosx") then
        linkoptions {"-Wl,--no-undefined"}
    elseif os.is("macosx") then 
        filter { "kind:SharedLib", "system:macosx" }
            linkoptions { '-Wl,-install_name', '-Wl,@loader_path/%{cfg.linktarget.name}' }
    end

    excludes {"../RadeonRays/src/device/embree*"}
    if os.is("macosx") then
        buildoptions "-std=c++11 -stdlib=libc++"
    elseif os.is("linux") then
        buildoptions "-std=c++11"

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
                                os.getcwd() .. "../Resources/kernels/GLSL/ "  .. 
                                ".comp " ..
                                "vulkan " ..
                                 "> ./src/kernelcache/kernels_vk.h"                           
                                ) 
            print ">> RadeonRays: VK kernels embedded"
        end
        
        if _OPTIONS["use_opencl"] then
            os.execute( "python ../Tools/scripts/stringify.py " .. 
                                os.getcwd() .. "../Resources/kernels/CL/ "  .. 
                                ".cl " ..
                                "opencl " ..
                                 "> ./src/kernelcache/kernels_cl.h"                           
                                ) 
            print ">> RadeonRays: CL kernels embedded"
        end
    end

    if _OPTIONS["use_tbb"] then
        defines {"USE_TBB"}
        links {"tbb"}
        includedirs { "../3rdParty/tbb/include" }
    end

    if _OPTIONS["use_opencl"] then
        includedirs { "../CLW" }
        links {"CLW"}
        files {"../RadeonRays/**.cl" }
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
            links {"embree"}
        elseif os.is("windows") then
            links {"embree"}
        end
    end


    if _OPTIONS["use_vulkan"] then
        local vulkanSDKPath = os.getenv( "VK_SDK_PATH" );
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
            links {"Anvil"}
            links {"vulkan"}
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
    

