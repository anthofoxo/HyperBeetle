-- Uses ISO 639-2
local mt = {}
mt.__index = function(table, key)
    local subtable = setmetatable({}, mt)
    rawset(table, key, subtable)
    return subtable
end

local lang = setmetatable({}, mt)

lang.title = "강남스타일" -- Change this before release plz :3
lang.menu.play = "놀다"
lang.menu.option = "옵션"
lang.menu.editor = "편집자"
lang.menu.quit = "출구"

return lang