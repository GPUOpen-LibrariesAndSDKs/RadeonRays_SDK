project "CLW"
    kind "StaticLib"
    location "../CLW"
    includedirs { "." }
    files { "../CLW/**.h", "../CLW/**.cpp", "../CLW/CL/CLW.cl" }

    if os.is("macosx") then
        buildoptions "-std=c++11 -stdlib=libc++"
    else if os.is("linux") then
        buildoptions "-std=c++11 -fPIC"
        os.execute("rm -rf obj");
        end
    end

    configuration {}

    -- we rely on RadeonRays to do the actual embedding for us
    if _OPTIONS["embed_kernels"] then
        defines {"RR_EMBED_KERNELS=1"}
            os.execute( "python ../Tools/scripts/stringify.py " ..
                                os.getcwd() .. "/../CLW/CL/ "  .. 
                                ".cl " ..
                                "opencl " ..
                                 "> ./kernelcache/clwkernels_cl.h"
                                )
            print ">> CLW: CL kernels embedded"

        -- there is no CLW version for vulkan (yet at least)
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
