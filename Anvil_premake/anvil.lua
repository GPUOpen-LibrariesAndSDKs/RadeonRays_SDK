project "Anvil"
location "../Anvil"
kind "StaticLib"

if os.is("macosx") then
    buildoptions "-std=c++11 -stdlib=libc++ -fpermissive"
    glslang_os_files = "../Anvil/deps/glslang/glslang/OSDependent/Unix/*.*"
elseif os.is("linux") then
    buildoptions "-std=c++11 -fPIC -fpermissive"
    glslang_os_files = "../Anvil/deps/glslang/glslang/OSDependent/Unix/*.*"
elseif os.is("windows") then
    includedirs { "../Anvil/deps/glslang/OGLCompilersDLL" }
    glslang_os_files = "../Anvil/deps/glslang/glslang/OSDependent/Windows/*.*"
end


files { "./config.h",
        "../Anvil/include/misc/*.*",
        "../Anvil/include/wrappers/*.*",
        "../Anvil/src/misc/*.*",
        "../Anvil/src/wrappers/*.*",
        "../Anvil/deps/glslang/hlsl/*.*",
        "../Anvil/deps/glslang/glslang/MachineIndependent/preprocessor/*.*",
        "../Anvil/deps/glslang/glslang/MachineIndependent/*.*",
        "../Anvil/deps/glslang/glslang/GenericCodeGen/*.*",
        "../Anvil/deps/glslang/SPIRV/*.*",
        glslang_os_files,
}
if os.is("linux") then
    excludes {"../Anvil/include/misc/window_win3264.h",
            "../Anvil/src/misc/window_win3264.cpp", }
    files { "../Anvil/deps/glslang/OGLCompilersDLL/*.*" }
elseif os.is("windows") then
    files { "../Anvil/deps/glslang/OGLCompilersDLL/*.*" }

   excludes {
        "../Anvil/include/misc/window_xcb.h", 
        "../Anvil/include/misc/xcb_loader_for_anvil.h",
        "../Anvil/src/misc/window_xcb.cpp", 
        "../Anvil/src/misc/xcb_loader_for_anvil.cpp"
    }
end

defines {"ANVIL_LINK_WITH_GLSLANG" }

configuration {"x32", "Debug"}
targetdir "../Bin/Debug/x86"
configuration {"x64", "Debug"}
targetdir "../Bin/Debug/x64"
configuration {"x32", "Release"}
targetdir "../Bin/Release/x86"
configuration {"x64", "Release"}
targetdir "../Bin/Release/x64"
configuration {}