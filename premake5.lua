function fileExists(name)
   local f=io.open(name,"r")
   if f~=nil then io.close(f) return true else return false end
end

newoption {
    trigger     = "embed_kernels",
    description = "Embed CL kernels into binary module"
}

newoption {
    trigger = "use_tbb",
    description = "Use Intel(R) TBB library for multithreading"
}


newoption {
    trigger = "use_opencl",
    description = "Use OpenCL for GPU hit testing"
}

newoption {
    trigger = "use_embree",
    description = "Use Intel(R) Embree for CPU hit testing"
}


newoption {
    trigger = "use_vulkan",
    description = "Use vulkan for GPU hit testing"
}

newoption {
    trigger = "package",
    description = "Package the library for a binary release"
}

newoption {
    trigger = "library_only",
    description = "Don't define a solution just add library to calling parent premake solution"
}

newoption {
    trigger = "no_tests",
    description = "Don't add any unit tests and remove any test functionality from the library"
}

newoption {
    trigger = "static_library",
    description = "Create static libraries rather than dynamic"
}

newoption {
    trigger = "benchmark",
    description = "Benchmark with command line interface instead of App."
}

newoption {
    trigger = "shared_calc",
    description = "Link Calc(compute abstraction layer) dynamically"
}

newoption {
    trigger     = "enable_raymask",
    description = "Enable ray masking in intersection kernels"
}

newoption {
    trigger     = "rpr",
    description = "Enable RadeonProRender API lib"
}

newoption {
    trigger     = "tutorials",
    description = "Add tutorials projects"
}

if not _OPTIONS["use_opencl"] and not _OPTIONS["use_vulkan"] and not _OPTIONS["use_embree"] then
    _OPTIONS["use_opencl"] = 1
end

if _OPTIONS["shared_calc"] and _OPTIONS["use_vulkan"] then
    print ">>Error: shared_calc option is not yet supported for Vulkan backend"
    return -1
end

if _OPTIONS["library_only"] then
    _OPTIONS["no_tests"] = 1
    print ">> Disabling test for library only build";
end

if _OPTIONS["shared_calc"] then
   print ">> Building Calc as a shared library"
end

if _OPTIONS["use_embree"] then
   print ">> Embree backend enabled"
end


function build(config)
    if os.is("windows") then
        buildcmd="devenv RadeonRays.sln /build \"" .. config .. "|x64\""
    else
        config=config .. "_x64"
        buildcmd="make config=" .. config
    end

    return os.execute(buildcmd)
end


if _OPTIONS["package"] then
    print ">> RadeonRays: Packaging mode"
    os.execute("rm -rf dist")
    os.execute("mkdir dist")
    os.execute("echo $(pwd)")
    os.execute("cd dist && mkdir RadeonRays && cd RadeonRays && mkdir include && mkdir lib && cd lib && mkdir x64 && cd .. && mkdir bin && cd bin && mkdir x64 && cd .. && cd include && mkdir math && cd ../.. && mkdir 3rdParty")
    os.execute("cp -r ./RadeonRays/include ./dist/RadeonRays/")
    os.execute("cp ./Bin/Release/x64/Radeon*.lib ./dist/RadeonRays/lib/x64")
    os.execute("cp ./Bin/Release/x64/Radeon*.dll ./dist/RadeonRays/bin/x64")
    os.execute("cp ./Bin/Release/x64/libRadeon*.so ./dist/RadeonRays/lib/x64")
    os.execute("cp ./Bin/Release/x64/Radeon*.dylib ./dist/RadeonRays/bin/x64")
    os.execute("rm -rf ./App/obj")
    os.execute("rm -rf ./CLW/obj")
    os.execute("cp -r ./App ./dist")
    os.execute("cp -r ./CLW ./dist")
    os.execute("rm -rf ./dist/App/*.vcxproj && rm -rf ./dist/App/*.vcxproj.*")
    os.execute("rm -rf ./dist/App/Makefile && rm -rf ./dist/CLW/Makefile")
    os.execute("rm -rf ./dist/CLW/*.vcxproj && rm -rf ./dist/CLW/*.vcxproj.*")
    os.execute("rm -rf ./dist/CLW/*.lua && rm -rf ./dist/App/*.lua")
    os.execute("cp ./Tools/deploy/premake4.lua ./dist")
    os.execute("cp ./Tools/deploy/OpenCLSearch.lua ./dist")
    os.execute("cp ./Tools/deploy/App.lua ./dist/App")
    os.execute("cp ./Tools/deploy/CLW.lua ./dist/CLW")
    os.execute("cp -r ./Tools/premake ./dist")

    os.execute("cp -r ./3rdParty/freeglut ./dist/3rdParty/")
    os.execute("cp -r ./3rdParty/glew ./dist/3rdParty/")
    os.execute("cp -r ./3rdParty/oiio ./dist/3rdParty/")
    os.execute("cp -r ./3rdParty/oiio16 ./dist/3rdParty/")
    os.execute("cp -r ./Tools/deploy/LICENSE.txt ./dist")
    os.execute("cp -r ./Tools/deploy/README.md ./dist")
else
    if not _OPTIONS["library_only"] then
        solution "RadeonRays"

        configurations { "Debug", "Release" }
        language "C++"
        flags { "NoMinimalRebuild", "EnableSSE", "EnableSSE2" }
    end

    if( _OPTIONS["static_library"]) then
        defines{ "RR_STATIC_LIBRARY=1" }
	print ">> Building Radeon Rays as a static library";
    end

    if _OPTIONS["use_opencl"] then
        print ">> OpenCL backend enabled"
        -- find and add path to Opencl headers
        dofile ("./OpenCLSearch.lua" )
    end
    -- define common includes
    includedirs { ".","./3rdParty/include" }

    if not _OPTIONS["no_tests"]  then
        defines{"PRORAY_UNITTEST=1"}
    end

    -- perform OS specific initializations
    local targetName;
    if os.is("macosx") then
        targetName = "osx"
        platforms {"x64"}
    else
        platforms {"x32", "x64"}
    end

    if os.is("windows") then
        targetName = "win"
        defines{ "WIN32" }
        buildoptions { "/MP"  } --multiprocessor build
        defines {"_CRT_SECURE_NO_WARNINGS"}
    elseif os.is("linux") then
        buildoptions {"-fvisibility=hidden"}
    end

    if _OPTIONS["use_opencl"] then
        defines{"USE_OPENCL=1"}
    end
    if _OPTIONS["use_vulkan"] then
	print ">> Vulkan backend enabled"
        defines{"USE_VULKAN=1"}
        vulkanPath = ""
        vulkanSDKPath = os.getenv( "VK_SDK_PATH" );
        if vulkanSDKPath == nil then
            vulkanSDKPath = os.getenv( "VULKAN_SDK" );
        end

        if vulkanSDKPath ~= nil then
            if os.is("linux") then
                vulkanPath = vulkanSDKPath .. "/include"
            else
                vulkanPath = vulkanSDKPath .. "/Include"
            end
        end

        includedirs {   "./Anvil",
            "./Anvil/deps",
            "./Anvil/include",
            "./Anvil_premake",
            vulkanPath }
    end

    --make configuration specific definitions
    configuration "Debug"
        defines { "_DEBUG" }
        flags { "Symbols" }
    configuration "Release"
        defines { "NDEBUG" }
        flags { "Optimize" }

    configuration {"x64", "Debug"}
        targetsuffix "64D"
    configuration {"x32", "Debug"}
        targetsuffix "D"
    configuration {"x64", "Release"}
        targetsuffix "64"

    configuration {} -- back to all configurations

    if  _OPTIONS["use_vulkan"] then
        if fileExists("./Anvil_premake/anvil.lua") then
            dofile("./Anvil_premake/anvil.lua")
        end
    end

    if fileExists("./RadeonRays/RadeonRays.lua") then
        dofile("./RadeonRays/RadeonRays.lua")
    end

    if fileExists("./Calc/Calc.lua") then
        dofile("./Calc/Calc.lua")
    end
    
    if _OPTIONS["rpr"] then
        if fileExists("./Rpr/Rpr.lua") then
            dofile("./Rpr/Rpr.lua")
        end
        if fileExists("./Test/Test.lua") then
            dofile("./Test/Test.lua")
        end
    end

    if _OPTIONS["use_opencl"] then
        if fileExists("./CLW/CLW.lua") then
            dofile("./CLW/CLW.lua")
        end

        if not _OPTIONS["library_only"] then
            if fileExists("./App/App.lua") then
                dofile("./App/App.lua")
            end
        end
    end

    if not _OPTIONS["no_tests"] then
        if fileExists("./Gtest/gtest.lua") then
            dofile("./Gtest/gtest.lua")
        end
        if fileExists("./UnitTest/UnitTest.lua") then
            dofile("./UnitTest/UnitTest.lua")
        end
    end

    if _OPTIONS["tutorials"] then
        if fileExists("./Tutorials/Tutorials.lua") then
            dofile("./Tutorials/Tutorials.lua")
        end
    end

end
