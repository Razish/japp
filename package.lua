#!/usr/bin/env lua

require 'lfs' -- lua filesystem

--[[
	JA++ binaries package script

	requirements:
	- lua 5.4
	- luafilesystem
	- 7z or zip
--]]

local function get_platform_details()
    if package.config:sub(1, 1) == '\\' then
        -- Windows
        local env_OS = os.getenv('OS')
        local osTranslation = {Windows_NT = 'Windows'};
        local os_name = osTranslation[env_OS] and osTranslation[env_OS] or env_OS

        local env_ARCH = os.getenv('PROCESSOR_ARCHITECTURE')
        local archTranslation = {AMD64 = 'x86_64'};
        local arch_name = archTranslation[env_ARCH] and archTranslation[env_ARCH] or env_ARCH

        return os_name, arch_name
    else
        -- hopefullu a POSIX-y unix
        local os_name = io.popen('uname -s', 'r'):read('*l')
        local arch_name = io.popen('uname -m', 'r'):read('*l')
        return os_name, arch_name
    end
end

local host_platform, arch = get_platform_details()
local target_arch = os.getenv('TARGET_ARCH') -- this can override what arch we're packaging
arch = target_arch and target_arch or arch

local suffix = host_platform .. '_' .. arch
local libExt = ({
    Linux = '.so', --
    Windows = '.dll', --
    Darwin = '.dylib',
})[host_platform]
local nixy = true and package.config:find('/') or false
local extension = nixy and '.zip' or '.pk3'
local redirect_stdout = nixy and '>/dev/null' or '>nul'
local redirect_all = nixy and '>/dev/null 2>&1' or '>nul 2>&1'

local function test_cmd(cmd) return os.execute((nixy and 'command -v ' or 'where.exe ') .. cmd .. redirect_all) end

local paks = {
    ['cl'] = {
        ['bins'] = {'cgame' .. arch .. libExt, 'ui' .. arch .. libExt},
        -- pdb for windows?
    },

    ['sv'] = {
        ['bins'] = {'jampgame' .. arch .. libExt},
        -- pdb for windows?
    },
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
            local cmd = nil
            if test_cmd('zip') then
                cmd = 'zip ' .. outname .. ' ' .. filelist .. redirect_stdout
            elseif test_cmd('7z') then
                cmd = '7z a -tzip -y ' .. outname .. ' ' .. filelist .. ' ' .. redirect_stdout
            else
                error('can\'t find zip or 7z on PATH')
            end
            if os.execute(cmd) == nil then error() end
        end
    end
end
