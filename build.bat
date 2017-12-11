@echo off

for %%X in (scons.bat, lua.exe) do (IF EXIST %%~$PATH:X echo Found %%X at %%~$PATH:X)

SET NO_SSE=1
scons -Q debug=1 force32=1 no_sql=1 no_notify=1 tools=default
@rem lua package.lua
