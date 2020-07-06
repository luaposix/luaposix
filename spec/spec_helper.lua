local unpack = table.unpack or unpack


do
   local std = require 'specl.std'
   local spawn = require 'specl.shell'.spawn
   local objdir = spawn('./build-aux/luke --value=objdir').output

   package.path = std.package.normalize (
      './lib/?.lua',
      './lib/?/init.lua',
      package.path
   )
   package.cpath = std.package.normalize(
      './' .. objdir:match("^objdir='(.*)'") .. '/?.so',
      package.cpath
   )
end


local bit = require 'posix._base'
band, bnot, bor = bit.band, bit.bnot, bit.bor

badargs = require 'specl.badargs'
hell = require 'specl.shell'
posix = require 'posix'

local gsub = string.gsub


-- Allow user override of LUA binary used by hell.spawn, falling
-- back to environment PATH search for 'lua' if nothing else works.
local LUA = os.getenv 'LUA' or 'lua'


-- Easily check for std.object.type compatibility.
function prototype(o)
   return(getmetatable(o) or {})._type or io.type(o) or type(o)
end


local function mkscript(code)
   local f = os.tmpname()
   local h = io.open(f, 'w')
   h:write(code)
   h:close()
   return f
end


--- Run some Lua code with the given arguments and input.
-- @string code valid Lua code
-- @tparam[opt={}] string|table arg single argument, or table of
--    arguments for the script invocation
-- @string[opt] stdin standard input contents for the script process
-- @treturn specl.shell.Process|nil status of resulting process if
--    execution was successful, otherwise nil
function luaproc(code, arg, stdin)
   local f = mkscript(code)
   if type(arg) ~= 'table' then
      arg = {arg}
   end
   local cmd = {LUA, f, unpack(arg)}
   -- inject env and stdin keys separately to avoid truncating `...` in
   -- cmd constructor
   cmd.stdin = stdin
   cmd.env = {
      LUA = LUA,
      LUA_CPATH = package.cpath,
      LUA_PATH = package.path,
      LUA_INIT = '',
      LUA_INIT_5_2 = '',
      LUA_INIT_5_3 = '',
      PATH = os.getenv 'PATH'
   }
   local proc = hell.spawn(cmd)
   os.remove(f)
   return proc
end


-- Use a consistent template for all temporary files.
TMPDIR = posix.getenv('TMPDIR') or '/tmp'
template = TMPDIR .. '/luaposix-test-XXXXXX'

-- Allow comparison against the error message of a function call result.
function Emsg(_, msg) return msg or '' end

-- Collect stdout from a shell command, and strip surrounding whitespace.
function cmd_output(cmd)
   return hell.spawn(cmd).output:gsub('^%s+', ''):gsub('%s+$', '')
end


local st = require 'posix.sys.stat'
local stat, S_ISDIR = st.lstat, st.S_ISDIR

-- Recursively remove a temporary directory.
function rmtmp(dir)
   for f in posix.files(dir) do
      if f ~= '.' and f ~= '..' then
         local path = dir .. '/' .. f
         if S_ISDIR(stat(path).st_mode) ~= 0 then
            rmtmp(path)
         else
            os.remove(path)
         end
      end
   end
   os.remove(dir)
end


-- Create an empty file at PATH.
function touch(path) io.open(path, 'w+'):close() end


-- Format a bad argument type error.
local function typeerrors(fname, i, want, field, got)
   return {
      badargs.format('?', i, want, field, got),    -- LuaJIT
      badargs.format(fname, i, want, field, got), -- PUC-Rio
   }
end


function init(M, fname)
   return M[fname], function(...) return typeerrors(fname, ...) end
end


pack = table.pack or function(...)
   return {n=select('#', ...), ...}
end


local function tabulate_output(code)
   local proc = luaproc(code)
   if proc.status ~= 0 then
      return error(proc.errout)
   end
   local r = {}
   gsub(proc.output, '(%S*)[%s]*', function(x)
      if x ~= '' then
         r[x] = true
      end
   end)
   return r
end


--- Show changes to tables wrought by a require statement.
-- There are a few modes to this function, controlled by what named
-- arguments are given.   Lists new keys in T1 after 'require 'import'`:
--
--       show_apis {added_to=T1, by=import}
--
-- List keys returned from `require 'import'`, which have the same
-- value in T1:
--
--       show_apis {from=T1, used_by=import}
--
-- List keys from `require 'import'`, which are also in T1 but with
-- a different value:
--
--       show_apis {from=T1, enhanced_by=import}
--
-- List keys from T2, which are also in T1 but with a different value:
--
--       show_apis {from=T1, enhanced_in=T2}
--
-- @tparam tabble argt one of the combinations above
-- @treturn table a list of keys according to criteria above
function show_apis(argt)
   local added_to, from, not_in, enhanced_in, enhanced_after, by =
      argt.added_to, argt.from, argt.not_in, argt.enhanced_in,
      argt.enhanced_after, argt.by

   if added_to and by then
      return tabulate_output([[
         local before, after = {}, {}
         for k in pairs(]] .. added_to .. [[) do
            before[k] = true
         end

         local M = require ']] .. by .. [['
         for k in pairs(]] .. added_to .. [[) do
            after[k] = true
         end

         for k in pairs(after) do
            if not before[k] then
               print(k)
            end
         end
      ]])

   elseif from and not_in then
      return tabulate_output([[
         local _ENV = require 'std.normalize' {
            from = ']] .. from .. [[',
            M = require ']] .. not_in .. [[',
         }

         for k in pairs(M) do
            -- M[1] is typically the module namespace name, don't match
            -- that!
            if k ~= 1 and from[k] ~= M[k] then
               print(k)
            end
         end
      ]])

   elseif from and enhanced_in then
      return tabulate_output([[
         local _ENV = require 'std.normalize' {
            from = ']] .. from .. [[',
            M = require ']] .. enhanced_in .. [[',
         }

         for k, v in pairs(M) do
            if from[k] ~= M[k] and from[k] ~= nil then
               print(k)
            end
         end
      ]])

   elseif from and enhanced_after then
      return tabulate_output([[
         local _ENV = require 'std.normalize' {
            from = ']] .. from .. [[',
         }
         local before , after = {}, {}
         for k, v in pairs(from) do
            before[k] = v
         end
         ]] .. enhanced_after .. [[
         for k, v in pairs(from) do
            after[k] = v
         end

         for k, v in pairs(before) do
            if after[k] ~= nil and after[k] ~= v then
               print(k)
            end
         end
      ]])
   end

   assert(false, 'missing argument to show_apis')
end


do
   --[[ ================ ]]--
   --[[ Custom matchers. ]]--
   --[[ ================ ]]--

   local Matcher = require 'specl.matchers'.Matcher
   local matchers = require 'specl.matchers'.matchers

   -- Avoid timestamp race-conditions.
   matchers.be_within_n_of = Matcher {
      function(self, actual, expected)
         local delta = expected.delta or 1
         local value = expected.value or expected[1]
         return(value <= actual + delta) and(value >= actual - delta)
      end,

      actual = 'within_of',

      format_expect = function(self, expect)
         local delta = tostring(expect.delta or 1)
         local value = tostring(expect.value or expect[1])
         return ' number within ' .. delta .. ' of ' .. value .. ', '
      end,
   }

end
