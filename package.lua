require 'lfs' -- lua filesystem

--[[
JA++ binaries package script
author: Raz0r

requirements:
- lua 5.1
- lua-filesystem
- 7zip
--]]

if lfs == nil then error('lua-filesystem not available for this version of lua (' .. _VERSION .. ')') end

local linux = true and package.config:find('/') or false
local bits = (arg[1] == '64bit') and 64 or 32
local extension = linux and '.zip' or '.pk3'
local suffix = (linux and 'linux' or 'win') .. tostring(bits)
local libExt = linux and '.so' or '.dll'
local arch
if linux then
    arch = (bits == 32) and 'i386' or 'x86_64'
else
    arch = (bits == 32) and 'x86' or 'x64'
end

local paks = {
    ['cl'] = {
        ['bins'] = {'cgame' .. arch .. libExt, 'ui' .. arch .. libExt}
        -- pdb for windows?
    },

    ['sv'] = {
        ['bins'] = {'jampgame' .. arch .. libExt}
        -- pdb for windows?
    }
}

for prefix, pak in pairs(paks) do
    for pakname, files in pairs(pak) do
        local filelist = ''
        for _, file in pairs(files) do
            if lfs.touch(file) == nil then
                print('Missing file: ' .. file)
            else
                filelist = filelist .. ' ' .. file -- append file name
            end
        end
        filelist = string.sub(filelist, 2) -- remove the leading space

        local outname = prefix .. '_' .. pakname .. '_' .. suffix .. extension

        -- remove existing pak
        if lfs.touch(outname) ~= nil then
            print('removing existing pak "' .. outname .. '"')
            os.remove(outname)
        end

        if #filelist ~= 0 then
            print('creating "' .. outname .. '"')
            local redirect = (linux ~= false) and '>/dev/null 2>&1' or '>nul 2>&1'
            -- FIXME: consider `zip` on macOS, check availability with `command -v foo`
            os.execute('7z a -tzip -y ' .. outname .. ' ' .. filelist .. ' ' .. redirect)
        end
    end
end
