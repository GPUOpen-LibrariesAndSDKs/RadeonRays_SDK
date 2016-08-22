project "Calc"

    if _OPTIONS["static_calc"] then
        kind "StaticLib"
        defines {"CALC_STATIC_LIBRARY"}
    else
        kind "SharedLib"
        defines {"CALC_EXPORT_API"}

        if os.is("linux") then
            linkoptions {"-Wl,--no-undefined"}
        elseif os.is("macosx") then
            filter { "kind:SharedLib", "system:macosx" }
            linkoptions { '-Wl,-install_name', '-Wl,@loader_path/%{cfg.linktarget.name}' }
        end

    end

    location "../Calc"
    includedirs { ".", "./inc", "../CLW" }
    files { "../Calc/**.h", "../Calc/**.cpp"}
    links {"CLW"}

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
