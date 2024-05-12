workspace "hyperbeetle"
configurations { "debug", "release", "dist" }
architecture "amd64"
startproject "hyperbeetle"

flags "MultiProcessorCompile"
language "C++"
cppdialect "C++latest"
cdialect "C11"
staticruntime "On"
stringpooling "On"
editandcontinue "Off"
kind "StaticLib"
targetdir "%{wks.location}/bin/%{cfg.buildcfg}"
objdir "%{wks.location}/bin_obj/%{cfg.buildcfg}"

filter "configurations:debug"
runtime "Debug"
optimize "Debug"
symbols "On"
defines "HB_DEBUG"

filter "configurations:release"
runtime "Release"
optimize "Speed"
symbols "On"
defines "HB_RELEASE"

filter "configurations:dist"
runtime "Release"
optimize "Speed"
symbols "Off"
vectorextensions "AVX2"
flags { "LinkTimeOptimization", "NoBufferSecurityCheck" }
defines "HB_DIST"

filter "system:windows"
systemversion "latest"
defines { "WIN32_LEAN_AND_MEAN", "NOMINMAX", "_ITERATOR_DEBUG_LEVEL=0" }
buildoptions { "/EHsc", "/Zc:preprocessor", "/Zc:__cplusplus" }

-- The makefiles dont respect the C++latest dialect
filter { "system:linux", "language:C++" }
buildoptions "-std=c++23"
filter {}

group "dependencies"
for _, matchedfile in ipairs(os.matchfiles("premake/*.lua")) do
    include(matchedfile)
end
group ""

include "hyperbeetle/premake5.lua"