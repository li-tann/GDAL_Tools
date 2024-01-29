# 获取当前脚本所在的路径
$scriptPath = Split-Path -Parent $MyInvocation.MyCommand.Definition

# 将脚本路径添加到PATH环境变量
$env:PATH += ";$scriptPath"

echo "add '$scriptPath' to PATH"

# 现在你可以运行脚本所在目录下的程序了
# 例如，如果你有一个名为example.exe的程序，你可以这样运行它：
# .\example.exe