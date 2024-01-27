# Multilingual Support
HyperBeetle supports multiple languages. Currently the language may only be changed in the source code.
The current languages are defined in `pack/core/lang/x.lua` where `x` is the name of the language as specified by [ISO 639-2](https://www.loc.gov/standards/iso639-2/php/code_list.php).

## Lanuage template

```lua
-- Metatable, auto create key indexes if not exists
-- Allows us to do `lang.menu.play` without needing to first `lang.menu = {}`
local mt = {}
mt.__index = function(table, key)
    local subtable = setmetatable({}, mt)
    rawset(table, key, subtable)
    return subtable
end

local lang = setmetatable({}, mt)

-- Reference existing language files for up to date keys
lang.language_key = "value"

return lang
```

## Current language keys
* `title`
* `menu.play`
* `menu.option`
* `menu.editor`
* `menu.quit`

Note: Language keys may be omitted, the engine will implicitly give the key name as the default value.