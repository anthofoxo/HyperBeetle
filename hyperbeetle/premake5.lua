project "hyperbeetle"

debugdir "../hyperbeetle_data"

kind "ConsoleApp" -- Override default
filter {"configurations:dist", "system:windows"}
    kind "WindowedApp" -- Win32 AND Dist use winmain
    defines "HE_ENTRY_WINMAIN"
filter{}

defines "TRACY_ENABLE" -- Move to AppDefines

files
{
    "%{prj.location}/**.cpp",
    "%{prj.location}/**.hpp",
    "%{prj.location}/**.c",
    "%{prj.location}/**.h"
}

includedirs
{
    "%{prj.location}",
    "%{prj.location}/source",
    "%{prj.location}/vendor",
    "%{wks.location}/sandbox_engine/source",
    "%{wks.location}/vendor/tracy/public",
    "%{wks.location}/vendor/glfw/include",
    "%{wks.location}/vendor/glad/include",
    "%{wks.location}/vendor/glm",
    "%{wks.location}/vendor/entt/src",
    "%{wks.location}/vendor/lua/src",
    "%{wks.location}/vendor/imgui",
    "%{wks.location}/vendor/imguizmo",
    "%{wks.location}/vendor/regex/include",
    "%{wks.location}/vendor/assimp_config",
    "%{wks.location}/vendor/assimp/include",
    "%{wks.location}/vendor/nanovg/src",
    "%{wks.location}/vendor/spdlog/include",
}

links
{
    "tracy",
    "glfw",
    "glad",
    "lua",
    "imgui",
    "imguizmo",
    "regex",
    "assimp",
    "nanovg"
}

filter "system:linux"
    links {
        "pthread",
        "dl",
        "X11",
        "m"
    }