OutputDir = "%{cfg.system}-%{cfg.architecture}/%{cfg.buildcfg}"
AppLinks = {}
AppIncludes = {}
AppDefines = {}

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

    filter "configurations:release"
        runtime "Release"
        optimize "Speed"

    filter "configurations:dist"
        runtime "Release"
        optimize "Speed"
        vectorextensions "AVX2"
        flags { "LinkTimeOptimization", "NoBufferSecurityCheck" }

    filter "system:windows"
        systemversion "latest"
        defines { "WIN32_LEAN_AND_MEAN", "NOMINMAX" }
        buildoptions { "/EHsc", "/Zc:preprocessor", "/Zc:__cplusplus" }

    filter {}

group "dependencies"
    include "premake/glfw.lua"
    include "premake/glad.lua"
    include "premake/lua.lua"
    include "premake/imgui.lua"
    include "premake/tracy.lua"
    include "premake/imguizmo.lua"
    include "premake/boost_regex.lua"
    include "premake/assimp.lua"
    include "premake/nanovg.lua"
group ""

include "hyperbeetle/premake5.lua"