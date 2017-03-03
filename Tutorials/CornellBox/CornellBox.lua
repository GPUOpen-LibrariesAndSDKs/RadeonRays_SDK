project "TutorialCornellBox"
    kind "ConsoleApp"
    location "../CornellBox"
    links {"RadeonRays", "CLW", "Calc"}
    files { "../CornellBox/**.h", "../CornellBox/**.cpp", "../Tools/**.h", "../Tools/**.cpp", "../CornellBox/**.fsh" , "../CornellBox/**.vsh"}
    includedirs{ "../../RadeonRays/include", "../../CLW", "../Tools", "." }

    if os.is("macosx") then
        sysincludedirs {"/usr/local/include"}
        libdirs {"/usr/local/lib"}
        linkoptions{ "-framework OpenGL", "-framework GLUT" }
        buildoptions "-std=c++11 -stdlib=libc++"
        links {"OpenImageIO"}
    end

    if os.is("windows") then
        includedirs { "../../3rdparty/glew/include", "../../3rdparty/freeglut/include", "../../3rdparty/oiio/include"  }
        links {"RadeonRays",}
        links {"freeglut", "glew"}
        libdirs {   "../../3rdparty/glew/lib/%{cfg.platform}", 
                    "../../3rdparty/freeglut/lib/%{cfg.platform}", 
                    "../../3rdparty/embree/lib/%{cfg.platform}", 
                    "../../3rdparty/oiio/lib/%{cfg.platform}"}
        configuration {"Debug"}
            links {"OpenImageIOD"}
        configuration {"Release"}
            links {"OpenImageIO"}
        configuration {}
    end

    if os.is("linux") then
        buildoptions "-std=c++11"
        links {"OpenImageIO", "pthread",}
        links{"glut", "GLEW", "GL",}
        os.execute("rm -rf obj");
    end

    configuration {"x32", "Debug"}
        targetdir "../../Bin/Debug/x86"
    configuration {"x64", "Debug"}
        targetdir "../../Bin/Debug/x64"
    configuration {"x32", "Release"}
        targetdir "../../Bin/Release/x86"
    configuration {"x64", "Release"}
        targetdir "../../Bin/Release/x64"
    configuration {}

    if os.is("windows") then
        postbuildcommands  { 
          'copy "..\\..\\3rdparty\\glew\\bin\\%{cfg.platform}\\glew32.dll" "%{cfg.buildtarget.directory}"', 
          'copy "..\\..\\3rdparty\\freeglut\\bin\\%{cfg.platform}\\freeglut.dll" "%{cfg.buildtarget.directory}"', 
          'copy "..\\..\\3rdparty\\embree\\bin\\%{cfg.platform}\\embree.dll" "%{cfg.buildtarget.directory}"',
          'copy "..\\..\\3rdparty\\embree\\bin\\%{cfg.platform}\\tbb.dll" "%{cfg.buildtarget.directory}"',
          'copy "..\\..\\3rdparty\\oiio\\bin\\%{cfg.platform}\\OpenImageIO.dll" "%{cfg.buildtarget.directory}"',
          'copy "..\\..\\3rdparty\\oiio\\bin\\%{cfg.platform}\\OpenImageIOD.dll" "%{cfg.buildtarget.directory}"' 
        }
    end