OutputDir = "%{cfg.system}-%{cfg.architecture}/%{cfg.buildcfg}"

workspace "hyperbeetle"
    architecture "x86_64"
    configurations { "debug", "release", "dist" }
    startproject "hyperbeetle"

    -- Project defaults
    flags "MultiProcessorCompile"
    language "C++"
    cppdialect "C++20"
    cdialect "C11"
    staticruntime "On"
    stringpooling "On"
    symbols "On"
    -- Interferes with debugging with tracy
    editandcontinue "Off"

    kind "StaticLib"
    targetdir("%{wks.location}/binaries/" .. OutputDir .. "/%{prj.name}")
    objdir("%{wks.location}/binaries/intermediates/" .. OutputDir .. "/%{prj.name}")

    filter "configurations:debug"
        runtime "Debug"
        optimize "Debug"
        defines "HE_DEBUG"

    filter "configurations:release"
        runtime "Release"
        optimize "Speed"
        defines "HE_RELEASE"

    filter "configurations:dist"
        runtime "Release"
        optimize "Speed"
        vectorextensions "AVX2"
        flags { "LinkTimeOptimization", "NoBufferSecurityCheck" }
        defines "HE_DIST"

    filter "system:windows"
        systemversion "latest"
        defines { "WIN32_LEAN_AND_MEAN", "NOMINMAX" }
        buildoptions { "/EHsc", "/Zc:preprocessor", "/Zc:__cplusplus" }    

    filter {}

group "dependencies"
for _, matchedfile in ipairs(os.matchfiles("premake/*.lua")) do
    include(matchedfile)
end
group ""

include "hyperbeetle/premake5.lua"