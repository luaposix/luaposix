--[[
 POSIX library for Lua 5.1, 5.2 & 5.3.
 (c) Gary V. Vaughan <gary@vaughan.pe>, 2014-2015
]]
--[[--
 Private argument checking helpers.

 Undocumented internal helpers for argcheck wrapping.

 @module posix._checkarg
]]


local function argerror (name, i, extramsg, level)
  level = level or 1
  local s = string.format ("bad argument #%d to '%s'", i, name)
  if extramsg ~= nil then
    s = s .. " (" .. extramsg .. ")"
  end
  error (s, level + 1)
end

local function toomanyargerror (name, expected, got, level)
  level = level or 1
  local fmt = "no more than %d argument%s expected, got %d"
  argerror (name, expected + 1,
            fmt:format (expected, expected == 1 and "" or "s", got), level + 1)
end

local function argtypeerror (name, i, expect, actual, level)
  level = level or 1
  local fmt = "%s expected, got %s"
  argerror (name, i,
            fmt:format (expect, type (actual):gsub ("nil", "no value")), level + 1)
end

local function badoption (name, i, what, option, level)
  level = level or 1
  local fmt = "invalid %s option '%s'"
  argerror (name, i, fmt:format (what, option), level + 1)
end

local function checkint (name, i, actual, level)
  level = level or 1
  if type (actual) ~= "number" then
    argtypeerror (name, i, "int", actual, level + 1)
  end
  return actual
end

local function checkstring (name, i, actual, level)
  level = level or 1
  if type (actual) ~= "string" then
    argtypeerror (name, i, "string", actual, level + 1)
  end
  return actual
end

local function checkselection (fname, argi, fields, level)
  level = level or 1
  local field1, type1 = fields[1], type (fields[1])
  if type1 == "table" and #fields > 1 then
    toomanyargerror (fname, argi, #fields + argi - 1, level + 1)
  elseif field1 ~= nil and type1 ~= "table" and type1 ~= "string" then
    argtypeerror (fname, argi, "table, string or nil", field1, level + 1)
  end
  for i = 2, #fields do
    checkstring (fname, i + argi - 1, fields[i], level + 1)
  end
end

local function checktable (name, i, actual, level)
  level = level or 1
  if type (actual) ~= "table" then
    argtypeerror (name, i, "table", actual, level + 1)
  end
  return actual
end

local function optint (name, i, actual, def, level)
  level = level or 1
  if actual ~= nil and type (actual) ~= "number" then
    argtypeerror (name, i, "int or nil", actual, level + 1)
  end
  return actual or def
end

local function optstring (name, i, actual, def, level)
  level = level or 1
  if actual ~= nil and type (actual) ~= "string" then
    argtypeerror (name, i, "string or nil", actual, level + 1)
  end
  return actual or def
end

local function opttable (name, i, actual, def, level)
  level = level or 1
  if actual ~= nil and type (actual) ~= "table" then
    argtypeerror (name, i, "table or nil", actual, level + 1)
  end
  return actual or def
end


return {
  argerror        = argerror,
  argtypeerror    = argtypeerror,
  badoption       = badoption,
  checkint        = checkint,
  checkselection  = checkselection,
  checkstring     = checkstring,
  checktable      = checktable,
  optint          = optint,
  optstring       = optstring,
  opttable        = opttable,
  toomanyargerror = toomanyargerror,
}
