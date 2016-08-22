project "Gtest"
    location "../Gtest"
    kind "StaticLib"
    includedirs { ".", "include" }
    defines { "GTEST_HAS_PTHREAD=0" }
    files { "src/gtest-all.cc", "src/gtest_main.cc" }
    buildoptions { "-g", "-Wall" }

    if _ACTION == "vs2012" then
    defines{ "GTEST_HAS_TR1_TUPLE=0" }
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
