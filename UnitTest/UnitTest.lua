project "UnitTest"
    location "../UnitTest"
    kind "ConsoleApp"
	includedirs { "../RadeonRays/include", "../Gtest/include", "../CLW", "../Calc/inc", "." }
    links {"Gtest", "RadeonRays", "CLW", "Calc"}
    files { "**.cpp", "**.h" }
    
    if os.is("macosx") then
        buildoptions "-std=c++11 -stdlib=libc++"
    else if os.is("linux") then
        buildoptions "-std=c++11"
        os.execute("rm -rf obj");
        end
    end

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
