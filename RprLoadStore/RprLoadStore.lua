project "RprLoadStore"
    kind "SharedLib"
    location "../RprLoadStore"
    links {"RadeonRays", "CLW", "Calc", "Rpr",}
    files { "../RprLoadStore/**.h", "../RprLoadStore/**.cpp", "../RprLoadStore/**.hpp" , "../RprLoadStore/**.cl" ,"../RprLoadStore/**.def"}
    configuration "x32"
--        libdirs { os.getenv("AMDAPPSDKROOT").."lib/x86"} 
    configuration "x64"
--         libdirs { os.getenv("AMDAPPSDKROOT").."lib/x86_64"} 
    configuration {} -- back to all configurations.
    if( os.is("windows") ) then
         linkoptions { '/DEF:"RprLoadStore.def"' }
    end

    if( os.is("linux") ) then
        buildoptions { '-std=c++0x' }      
    end

    if os.is("macosx") then
        buildoptions "-std=c++11 -stdlib=libc++"
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
       
