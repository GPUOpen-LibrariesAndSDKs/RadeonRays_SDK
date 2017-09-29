function fileExists(name)
   local f=io.open(name,"r")
   if f~=nil then io.close(f) return true else return false end
end

newoption {
    trigger     = "embed_kernels",
    description = "Embed CL kernels into binary module"
}

newoption {
    trigger     = "allow_cpu_devices",
    description = "Allows CPU Devices"
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
    trigger = "no_tests",
    description = "Don't add any unit tests and remove any test functionality from the library"
}

newoption {
    trigger = "static_library",
    description = "Create static libraries rather than dynamic"
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
    trigger     = "tutorials",
    description = "Add tutorials projects"
}

newoption {
    trigger     = "safe_math",
    description = "use safe math"
}

if not _OPTIONS["use_opencl"] and not _OPTIONS["use_vulkan"] and not _OPTIONS["use_embree"] then
    _OPTIONS["use_opencl"] = 1
end

if _OPTIONS["shared_calc"] and _OPTIONS["use_vulkan"] then
    print ">>Error: shared_calc option is not yet supported for Vulkan backend"
    return -1
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



solution "RadeonRays"

configurations { "Debug", "Release" }
platforms {"x64"}
language "C++"
flags { "NoMinimalRebuild", "EnableSSE", "EnableSSE2" }

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

if _OPTIONS["safe_math"] then
	defines { "USE_SAFE_MATH" }
end

if fileExists("./RadeonRays/RadeonRays.lua") then
	dofile("./RadeonRays/RadeonRays.lua")
end

if fileExists("./Calc/Calc.lua") then
	dofile("./Calc/Calc.lua")
end

if _OPTIONS["use_opencl"] then
	if fileExists("./CLW/CLW.lua") then
		dofile("./CLW/CLW.lua")
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