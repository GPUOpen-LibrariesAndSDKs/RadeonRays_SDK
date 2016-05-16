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

    if os.is("windows") then
        os.execute("mkdir Bin & cd Bin & mkdir Release & cd Release & mkdir x64 & cd .. & mkdir Debug & cd Debug & mkdir x64 & cd ../..")

        os.execute("cp ./3rdParty/freeglut/bin/x64/freeglut.dll ./Bin/Release/x64/")
        os.execute("cp ./3rdParty/freeglut/bin/x64/freeglut.dll ./Bin/Debug/x64/")
    
        os.execute("cp ./3rdParty/glew/bin/x64/glew32.dll ./Bin/Release/x64/")
        os.execute("cp ./3rdParty/glew/bin/x64/glew32.dll ./Bin/Debug/x64/")
    
        os.execute("cp ./3rdParty/oiio/bin/x64/OpenImageIO.dll ./Bin/Release/x64/")
        os.execute("cp ./3rdParty/oiio/bin/x64/OpenImageIOD.dll ./Bin/Debug/x64/")

        os.execute("cp ./FireRays/bin/x64/FireRays64.dll ./Bin/Release/x64/")
        os.execute("cp ./FireRays/bin/x64/FireRays64.dll ./Bin/Debug/x64/")
    end
    

    if os.is("macosx") then
        os.execute("mkdir Bin ; cd Bin ; mkdir Release ; cd Release ; mkdir x64 ; cd .. ; mkdir Debug ; cd Debug ; mkdir x64 ; cd ../..")
        os.execute("cp ./FireRays/lib/x64/libFireRays64.dylib ./Bin/Release/x64/")
        os.execute("cp ./FireRays/lib/x64/libFireRays64.dylib ./Bin/Debug/x64/")
    end
