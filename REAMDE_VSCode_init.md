# vscode + vcpkg 的配置

分两步，添加头文件和添加依赖库

## setting.json

依赖库由vcpkg.cmake查找（find_package）

如果该环境在其他工程中也会使用，可以将一下添加到default settings, 而不只是workspace settings

`ctrl+shift+p` + `setting` -> setting.json:

加入或修改字段，

```json
"cmake.configureSettings": {
    "CMAKE_TOOLCHAIN_FILE": "e:/vcpkg/scripts/buildsystems/vcpkg.cmake",
    "VCPKG_TARGET_TRIPLET": "x64-windows"
},
"cmake.cmakePath": "D:\\CMake\\bin\\cmake.exe",
"cmake.ctestPath": "D:\\CMake\\bin\\ctest.exe",
```

## c_cpp_properties.json

头文件需要自己引用

`ctrl+shift+p` + `configuration` -> c_cpp_properties.json:

在include字段中加入vcpkg中依赖库的头文件路径，

```json
"includePath": [
    "...",
    "E:/vcpkg/installed/x64-windows/include",
    "E:/vcpkg/installed/x64-windows/include/spdlog",
    "...",
],
```

如果CMakeLists.txt 中find_package仍报错，删除build后重新编译（主要是为了删除cache）