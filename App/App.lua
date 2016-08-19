project "App"
    kind "ConsoleApp"
    location "../App"
    links {"RadeonRays", "CLW"}
    files { "../App/**.h", "../App/**.cpp", "../App/**.cl", "../App/**.fsh", "../App/**.vsh" }
    includedirs{ "../RadeonRays/include", "../CLW", "." } 

    if os.is("macosx") then
        includedirs {"../3rdParty/oiio16/include"}
        libdirs {"../3rdParty/oiio16/lib/x64", "/usr/local/lib"}
        linkoptions{ "-framework OpenGL", "-framework GLUT" }
        buildoptions "-std=c++11 -stdlib=libc++"
        links {"OpenImageIO"}
    end

    if os.is("windows") then
        includedirs { "../3rdParty/glew/include", "../3rdParty/freeglut/include", "../3rdParty/oiio/include"  }
        links {"RadeonRays", "freeglut", "glew"}

        configuration {"x32"}
            libdirs { "../3rdParty/glew/lib/x86", "../3rdParty/freeglut/lib/x86", "../3rdParty/embree/lib/x86", "../3rdParty/oiio/lib/x86" }
        configuration {"x64"}
            libdirs { "../3rdParty/glew/lib/x64", "../3rdParty/freeglut/lib/x64", "../3rdParty/embree/lib/x64", "../3rdParty/oiio/lib/x64"}

        configuration {}

        configuration {"Debug"}
          links {"OpenImageIOD"}
        configuration {"Release"}
          links {"OpenImageIO"}
    end

    if os.is("linux") then
        buildoptions "-std=c++11"
        links {"OpenImageIO", "glut", "GLEW", "GL", "pthread"}
        os.execute("rm -rf obj");
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
            links { "vulkan"}
        elseif os.is("windows") then
            links {"Anvil"}
            links{"vulkan-1"}
        end
    end
    -- if _OPTIONS["embed_kernels"] then
    --      configuration {}
    --      defines {"FR_EMBED_KERNELS"}
    --      os.execute("python ../Tools/scripts/stringify.py ./CL/ > ./CL/cache/kernels.h")
--      print ">> App: CL kernels embedded"
--    end

    configuration {"x32", "Debug"}
        targetdir "../Bin/Debug/x86"
    configuration {"x64", "Debug"}
        targetdir "../Bin/Debug/x64"
    configuration {"x32", "Release"}
        targetdir "../Bin/Release/x86"
    configuration {"x64", "Release"}
        targetdir "../Bin/Release/x64"
    configuration {}
