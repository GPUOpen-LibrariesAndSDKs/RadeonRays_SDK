project "CLW"
    kind "StaticLib"
    location "../CLW"  
    includedirs { "." }
    files { "../CLW/**.h", "../CLW/**.cpp", "../CLW/**.cl" }

    if os.is("macosx") then
        buildoptions "-std=c++11 -stdlib=libc++"
    else if os.is("linux") then
        buildoptions "-std=c++11 -fPIC"
        os.execute("rm -rf obj");
        end
    end

    if _OPTIONS["embed_kernels"] then
    configuration {}
    defines {"FR_EMBED_KERNELS"}
    os.execute("python ../scripts/stringify.py ./CL/ > ./CL/cache/kernels.h")
    print ">> CLW: CL kernels embedded"
    end
    
    configuration {"x64", "Debug"}
        targetdir "../Bin/Debug/x64"
    configuration {"x64", "Release"}
        targetdir "../Bin/Release/x64"
    configuration {}
