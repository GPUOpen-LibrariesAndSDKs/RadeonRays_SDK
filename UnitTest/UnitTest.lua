project "UnitTest"
    location "../UnitTest"
    kind "ConsoleApp"
    includedirs { "../RadeonRays/include", "../Gtest/include", "../Calc/inc", "." }
    links {"Gtest", "RadeonRays", "Calc"}
    files { "**.cpp", "**.h" }
    defines{"PRORAY_UNITTEST=1"}
    
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

    if _OPTIONS["use_opencl"] then
        includedirs { "../CLW" }
        links {"CLW"}
    end

    if _OPTIONS["use_embree"] then
--        files {"../RadeonRays/src/device/embree*"}
        defines {"USE_EMBREE=1"}
        includedirs {"../3rdParty/embree/include"}

        configuration {"x32"}
            libdirs { "../3rdParty/embree/lib/x86"}
        configuration {"x64"}
            libdirs { "../3rdParty/embree/lib/x64"}
        configuration {}

        if os.is("macosx") then
            links {"embree.2"}
        elseif os.is("linux") then
            links {"embree"}
        elseif os.is("windows") then
            links {"embree"}
        end
    end

    if _OPTIONS["use_vulkan"] then
        local vulkanSDKPath = os.getenv( "VK_SDK_PATH" );
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
            links {"Anvil"}
            links{"vulkan"}
        elseif os.is("windows") then
            links {"Anvil"}
            links{"vulkan-1"}
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
