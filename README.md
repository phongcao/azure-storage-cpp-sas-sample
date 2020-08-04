## Build Instructions

### Prerequisites
- [Visual Studio including MFC and ATL optional sub-components](https://visualstudio.microsoft.com/) (Windows build only)

### Download vcpkg

```
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
```

### Configure vcpkg

`Linux`

```bash
~/$ ./bootstrap-vcpkg.sh
```

`Windows`

```powershell
PS> .\bootstrap-vcpkg.bat
```

### Install azure-storage-cpp package

`Linux`

```bash
~/$ ./vcpkg install azure-storage-cpp
```

`Windows`

```powershell
PS> .\vcpkg install azure-storage-cpp:x64-windows
```

### Building

`Linux`

```bash
~/$ mkdir build && cd build
~/$ cmake -DCMAKE_TOOLCHAIN_FILE=[vcpkg directory]/scripts/buildsystems/vcpkg.cmake ../
~/$ make -j $(nproc)
```

`Windows`

```PowerShell
PS> mkdir build_win
PS> cd build_win
PS> cmake -DCMAKE_TOOLCHAIN_FILE=[vcpkg directory]\scripts\buildsystems\vcpkg.cmake ..\
PS> cmake --build . --config "Release"
```

## Running

`Linux`

```bash
~/$ export STORAGE_NAME="Storage account name"
~/$ export STORAGE_CONNECTION_STRING="Storage connection string"
~/$ ./bin/storage_sas_sample
```

`Windows`

```PowerShell
PS> $env:STORAGE_NAME="Storage account name"
PS> $env:STORAGE_CONNECTION_STRING="Storage connection string"
PS> bin\Release\storage_sas_sample.exe
```
