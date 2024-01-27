-- Uses ISO 639-2
local mt = {}
mt.__index = function(table, key)
    local subtable = setmetatable({}, mt)
    rawset(table, key, subtable)
    return subtable
end

local lang = setmetatable({}, mt)

lang.title = "HyperBeetle"
lang.menu.play = "Play"
lang.menu.option = "Options"
lang.menu.editor = "Editor"
lang.menu.quit = "Quit"

return lang