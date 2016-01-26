function fileExists(name)
   local f=io.open(name,"r")
   if f~=nil then io.close(f) return true else return false end
end

newoption {
	trigger     = "embed_kernels",
	description = "Embed CL kernels into binary module"
}


solution "FireRays"
	configurations { "Debug", "Release" }    		
	language "C++"
	flags { "NoMinimalRebuild", "EnableSSE", "EnableSSE2" }
    -- find and add path to Opencl headers
	dofile ("./OpenCLSearch.lua" )
    -- define common includes
    includedirs { ".","./3rdParty/include" }

    -- perform OS specific initializations
    local targetName;
    if os.is("macosx") then
        targetName = "osx"
		platforms {"x64"}
	else
		platforms {"x64"}
	end
	
    if os.is("windows") then
        targetName = "win"
		defines{ "WIN32" }
        buildoptions { "/MP"  } --multiprocessor build
        defines {"_CRT_SECURE_NO_WARNINGS"}
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
	configuration {"x64", "Release"}
		targetsuffix "64"	
    
    configuration {} -- back to all configurations
	
	if fileExists("./CLW/CLW.lua") then
		dofile("./CLW/CLW.lua")
	end
	
	if fileExists("./App/App.lua") then
		dofile("./App/App.lua")
	end
