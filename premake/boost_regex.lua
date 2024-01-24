project "regex"

location "%{wks.location}/vendor/%{prj.name}"
language "C++"

files
{
	"%{prj.location}/src/*.cpp",
	"%{prj.location}/src/*.hpp"
}

includedirs "%{prj.location}/include"

filter "system:windows"
defines "_CRT_SECURE_NO_WARNINGS"