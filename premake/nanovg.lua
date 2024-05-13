project "nanovg"

location "%{wks.location}/vendor/%{prj.name}"
language "C"

files
{
	"%{prj.location}/src/*.c",
	"%{prj.location}/src/*.h"
}

filter "system:windows"
defines "_CRT_SECURE_NO_WARNINGS"