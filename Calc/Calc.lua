project "Calc"
    kind "StaticLib"
    location "../Calc"  
    includedirs { ".", "./inc", "../CLW" }
    files { "../Calc/**.h", "../Calc/**.cpp"}

    if os.is("macosx") then
        buildoptions "-std=c++11 -stdlib=libc++"
    else if os.is("linux") then
        buildoptions "-std=c++11 -fPIC"
        os.execute("rm -rf obj");
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
    

		
