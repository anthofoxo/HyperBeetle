project "tracy"

location "%{wks.location}/vendor/%{prj.name}"

defines {"TRACY_ENABLE"}

filter "system:linux"
defines "TRACY_DELAYED_INIT"
filter {}

location "%{wks.location}/vendor/tracy"
files "%{prj.location}/public/TracyClient.cpp"
-- BSD: libexecinfo
-- TRACY_NO_BROADCAST, Disables network broadcasting