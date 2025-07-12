@echo off
echo "Compiling..."
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
msbuild funLoader.sln /p:Configuration=Release /p:Platform=x64
