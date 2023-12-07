@REM https://blog.csdn.net/sdewendong/article/details/93736366

set target=''
setlocal enabledelayedexpansion
for /f %%i in (points.txt) do (
    set target=%%i
    @REM echo !target!
    _determine_topological_relationship.exe !target! china.shp >> 2.txt
)


