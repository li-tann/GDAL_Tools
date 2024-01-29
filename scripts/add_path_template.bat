@echo off
setlocal

:: 获取当前脚本所在的目录
set "SCRIPT_DIR=%~dp0"

:: 将脚本目录添加到PATH环境变量
set "PATH=%PATH%;%SCRIPT_DIR%"

:: 现在你可以运行脚本所在目录下的程序了
:: 例如，如果你有一个名为example.exe的程序，你可以这样运行它：
:: example.exe

:: 脚本结束时，恢复原始的PATH环境变量
endlocal