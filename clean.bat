@echo off

for %%X in (scons.bat, lua.exe) do (IF EXIST %%~$PATH:X echo Found %%X at %%~$PATH:X)

scons -Qc debug=0 force32=0 no_sql=1 tools=default
