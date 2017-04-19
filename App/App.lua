project "App"
    kind "ConsoleApp"
    location "../App"
    links {"RadeonRays", "CLW", "Calc"}
    files { "../App/**.inl", "../App/**.h", "../App/**.cpp", "../App/**.cl", "../App/**.fsh", "../App/**.vsh" }

    includedirs{ "../RadeonRays/include", "../CLW", "."}

    if os.is("macosx") then
        sysincludedirs {"/usr/local/include"}
        includedirs{"../3rdparty/glfw/include"}
        libdirs {"/usr/local/lib", "../3rdparty/glfw/lib/x64"}
        linkoptions{ "-framework OpenGL -framework CoreFoundation -framework CoreGraphics -framework IOKit -framework AppKit -framework QuartzCore" }
        buildoptions "-std=c++11 -stdlib=libc++"
        links {"OpenImageIO", "glfw3"}
    end

    if os.is("windows") then
        includedirs { "../3rdparty/glew/include", "../3rdparty/freeglut/include",
        "../3rdparty/oiio/include", "../3rdparty/glfw/include"}
        links {"RadeonRays", "glfw3"}
        if not _OPTIONS["benchmark"] then
            links {"glew", "OpenGL32", "glfw3"}
        end
            libdirs {   "../3rdparty/glew/lib/%{cfg.platform}",
                        "../3rdparty/freeglut/lib/%{cfg.platform}",
                        "../3rdparty/embree/lib/%{cfg.platform}",
                        "../3rdparty/oiio/lib/%{cfg.platform}",
			"../3rdparty/glfw/lib/%{cfg.platform}" }

        configuration {"Debug"}
            links {"OpenImageIOD"}
        configuration {"Release"}
            links {"OpenImageIO"}
        configuration {}
    end

    if os.is("linux") then
        buildoptions "-std=c++11"
        links {"OpenImageIO", "pthread"}
        if not _OPTIONS["benchmark"] then
            links{"GLEW", "GL", "glfw"}
        end
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

    if _OPTIONS["benchmark"] then
        defines{"APP_BENCHMARK"}
        removefiles{"../App/main.cpp",
                    "../App/shader_manager.cpp",}
    else
        removefiles {"../App/main_benchmark.cpp"}
    end
    -- if _OPTIONS["embed_kernels"] then
    --      configuration {}
    --      defines {"FR_EMBED_KERNELS"}
    --      os.execute("python ../Tools/scripts/stringify.py ./CL/ > ./CL/cache/kernels.h")
--      print ">> App: CL kernels embedded"
--    end

    if _OPTIONS["denoiser"] then
        defines{"ENABLE_DENOISER"}
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

    if os.is("windows") then
        postbuildcommands  {
          'copy "..\\3rdparty\\glew\\bin\\%{cfg.platform}\\glew32.dll" "%{cfg.buildtarget.directory}"',
          'copy "..\\3rdparty\\glfw\\bin\\%{cfg.platform}\\glfw3.dll" "%{cfg.buildtarget.directory}"',
          'copy "..\\3rdparty\\embree\\bin\\%{cfg.platform}\\embree.dll" "%{cfg.buildtarget.directory}"',
          'copy "..\\3rdparty\\embree\\bin\\%{cfg.platform}\\tbb.dll" "%{cfg.buildtarget.directory}"',
          'copy "..\\3rdparty\\oiio\\bin\\%{cfg.platform}\\OpenImageIO.dll" "%{cfg.buildtarget.directory}"',
          'copy "..\\3rdparty\\oiio\\bin\\%{cfg.platform}\\OpenImageIOD.dll" "%{cfg.buildtarget.directory}"'
        }
    end
