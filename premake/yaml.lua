project "yaml"

location "%{wks.location}/vendor/%{prj.name}"

includedirs "%{prj.location}/include"

files {
    "%{prj.location}/src/**.cpp",
    "%{prj.location}/src/**.h",
}

defines "YAML_CPP_STATIC_DEFINE"