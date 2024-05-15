project "hyperbeetle"

debugdir "../working"

kind "ConsoleApp"

filter { "configurations:dist", "system:windows" }
kind "WindowedApp"
defines "HB_ENTRY_WINMAIN"
filter {}

defines "TRACY_ENABLE" -- Move to AppDefines
defines "YAML_CPP_STATIC_DEFINE" -- Move to AppDefines

files
{
    "%{prj.location}/**.cpp",
    "%{prj.location}/**.hpp",
    "%{prj.location}/**.c",
    "%{prj.location}/**.h"
}

filter "system:windows"
files "%{prj.location}/hyperbeetle.rc"
filter {}

includedirs
{
    "%{prj.location}",
    "%{prj.location}/source",
    "%{prj.location}/vendor",

    "%{wks.location}/vendor/glm",

    "%{wks.location}/vendor/glfw/include",
    "%{wks.location}/vendor/glad/include",

    "%{wks.location}/vendor/nanovg/src",

    "%{wks.location}/vendor/spdlog/include",

    "%{wks.location}/vendor/entt/single_include",

    "%{wks.location}/vendor/lua/src",
    "%{wks.location}/vendor/yaml/include",

    "%{wks.location}/vendor/tracy/public",
}

links
{
    "glfw", "glad",
    "nanovg",
    "yaml",
    --"lua", 
    --"tracy"
}

filter "system:linux"
links { "pthread", "dl", "X11", "m" }

filter "system:windows"
links "opengl32"