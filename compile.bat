@echo off
echo "Compiling..."
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
msbuild funLoader.sln /p:Configuration=Release /p:Platform=x64
