-- tree view of the file system like the "tree" unix utility
-- John Belmonte <jvb@prairienet.org>

local posix = require 'posix'

local leaf_indent = '|   '
local tail_leaf_indent = '    '
local leaf_prefix = '|-- '
local tail_leaf_prefix = '`-- '
local link_prefix = ' -> '

local function printf(...)
  io.write(string.format(...))
end

local function do_directory(directory, level, prefix)
    local num_dirs = 0
    local num_files = 0
    local files = posix.dir(directory)
    local last_file_index = #files
    table.sort(files)
    for i, name in ipairs(files) do
        if name ~= '.' and name ~= '..' then
            local full_name = string.format('%s/%s', directory, name)
            local info = assert(posix.stat(full_name))
            local is_tail = (i==last_file_index)
            local prefix2 = is_tail and tail_leaf_prefix or leaf_prefix
            local link = ''
            if info.type == 'link' then
                linked_name = assert(posix.readlink(full_name))
                link = string.format('%s%s', link_prefix, linked_name)
            end
            printf('%s%s%s%s\n', prefix, prefix2, name, link)
            if info.type == 'directory' then
                local indent = is_tail and tail_leaf_indent or leaf_indent
                -- TODO: cache string concatination
                sub_dirs, sub_files = do_directory(full_name, level+1,
                    prefix .. indent)
                num_dirs = num_dirs + sub_dirs + 1
                num_files = num_files + sub_files
            else
                num_files = num_files + 1
            end
        end
    end
    return num_dirs, num_files
end

local function fore(directory)
    print(directory)
    num_dirs, num_files = do_directory(directory, 0, '')
    printf('\n%d directories, %d files\n', num_dirs, num_files)
end

directory = (arg and #arg > 0) and arg[1] or '.'
fore(directory)
