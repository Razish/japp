@echo off

for %%X in (scons.bat, lua.exe) do (IF EXIST %%~$PATH:X echo Found %%X at %%~$PATH:X)

scons -Q debug=0 force32=0 no_sql=1
@rem lua package.lua
