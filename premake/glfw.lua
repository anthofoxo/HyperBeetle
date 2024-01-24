project "glfw"

location "%{wks.location}/vendor/%{prj.name}"
language "C"

-- Common
files
{
	"%{prj.location}/src/context.c",
	"%{prj.location}/src/egl_context.c",
	"%{prj.location}/src/egl_context.h",
	"%{prj.location}/src/init.c",
	"%{prj.location}/src/input.c",
	"%{prj.location}/src/internal.h",
	"%{prj.location}/src/monitor.c",
	"%{prj.location}/src/osmesa_context.c",
	"%{prj.location}/src/osmesa_context.h",
	"%{prj.location}/src/vulkan.c",
	"%{prj.location}/src/window.c"
}

-- Windows
filter "system:windows"
files
{
	"%{prj.location}/src/wgl_context.c",
	"%{prj.location}/src/wgl_context.h",
	"%{prj.location}/src/win32_init.c",
	"%{prj.location}/src/win32_joystick.c",
	"%{prj.location}/src/win32_joystick.h",
	"%{prj.location}/src/win32_monitor.c",
	"%{prj.location}/src/win32_platform.h",
	"%{prj.location}/src/win32_thread.c",
	"%{prj.location}/src/win32_time.c",
	"%{prj.location}/src/win32_window.c"
}
defines { "_GLFW_WIN32", "_CRT_SECURE_NO_WARNINGS" }

-- Linux
-- sudo apt install xorg-dev
-- sudo dnf install libXcursor-devel libXi-devel libXinerama-devel libXrandr-devel
-- pkg install xorgproto

filter "system:linux"
files
{
	"%{prj.location}/src/glx_context.c",
	"%{prj.location}/src/glx_context.h",
	"%{prj.location}/src/linux_joystick.c",
	"%{prj.location}/src/linux_joystick.h",
	"%{prj.location}/src/posix_time.c",
	"%{prj.location}/src/posix_time.h",
	"%{prj.location}/src/posix_thread.c",
	"%{prj.location}/src/posix_thread.h",
	"%{prj.location}/src/x11_init.c",
	"%{prj.location}/src/x11_monitor.c",
	"%{prj.location}/src/x11_platform.h",
	"%{prj.location}/src/x11_window.c",
	"%{prj.location}/src/xkb_unicode.c",
	"%{prj.location}/src/xkb_unicode.h"
}
defines "_GLFW_X11"

-- Linux
-- For linux wayland: sudo apt install libwayland-dev libxkbcommon-dev wayland-protocols extra-cmake-modules
-- Fedora+ derivitives: sudo dnf install wayland-devel libxkbcommon-devel wayland-protocols-devel extra-cmake-modules
-- FreeBSD: pkg install wayland libxkbcommon wayland-protocols kf5-extra-cmake-modules

-- Doesnt build currenty: wl_platform.h:57:10: fatal error: 'wayland-xdg-shell-client-protocol.h' file not found
-- Likely related to `extra-cmake-modules` as we dont use cmake at all, x11 doesnt have this requirement
--[[
filter "system:linux"
files
{
	"%{prj.location}/src/glx_context.c",
	"%{prj.location}/src/glx_context.h",
	"%{prj.location}/src/linux_joystick.c",
	"%{prj.location}/src/linux_joystick.h",
	"%{prj.location}/src/posix_time.c",
	"%{prj.location}/src/posix_time.h",
	"%{prj.location}/src/posix_thread.c",
	"%{prj.location}/src/posix_thread.h",
	"%{prj.location}/src/wl_init.c",
	"%{prj.location}/src/wl_monitor.c",
	"%{prj.location}/src/wl_platform.h",
	"%{prj.location}/src/wl_window.c",
	"%{prj.location}/src/xkb_unicode.c",
	"%{prj.location}/src/xkb_unicode.h"
}
defines "_GLFW_WAYLAND"]]--


-- Macos
filter "system:macosx"
files
{
	"%{prj.location}/src/cocoa_init.m",
	"%{prj.location}/src/cocoa_joystick.m",
	"%{prj.location}/src/cocoa_joystick.h",
	"%{prj.location}/src/cocoa_monitor.m",
	"%{prj.location}/src/cocoa_platform.h",
	"%{prj.location}/src/cocoa_time.c",
	"%{prj.location}/src/cocoa_window.m",
	"%{prj.location}/src/nsgl_context.m",
	"%{prj.location}/src/nsgl_context.h",
	"%{prj.location}/src/posix_thread.c",
	"%{prj.location}/src/posix_thread.h"
}
defines "_GLFW_COCOA"