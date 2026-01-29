@echo on
setlocal EnableDelayedExpansion

rem ===== このbatの場所 =====
set Root=%~dp0

rem ===== プロジェクト相対パス =====
set SRC=%ROOT%..\Source\
set DST=%ROOT%..\Compiled\

rem ===== 正規化(内部では絶対パスが必要になるのでPC単位でのパスを生成) =====
if not exist "%SRC%" mkdir "%SRC%"
pushd "%SRC%"
set SRC=%CD%\
popd

if not exist "%DST%" mkdir "%DST%"

pushd "%DST%"
set DST=%CD%\
popd


rem ===== FXCの場所（絶対パスなので編集予定）=====
set FXC=C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\fxc.exe
set FAILED=0


rem ===== VS =====
for /R "%SRC%" %%f in (*VS.hlsl) do (
	set DIR=%%~dpf
    set REL=!DIR:%SRC%=!
    set OUTDIR=%DST%!REL!

	echo -----
	echo SRC =%SRC%
    echo DIR =!DIR!
    echo REL =!REL!
    echo OUTDIR =!OUTDIR!

	if not exist "!OUTDIR!" mkdir "!OUTDIR!"
	
	call "%FXC%" "%%f" /E vs /T vs_5_0 /Fo "!OUTDIR!%%~nf.cso" /Zi /Od || set FAILED=1
)

rem ===== PS =====
for /R "%SRC%" %%f in (*PS.hlsl) do (
	
	set DIR=%%~dpf
    set REL=!DIR:%SRC%=!
    set OUTDIR=%DST%!REL!

	echo -----
	echo SRC =%SRC%
    echo DIR =!DIR!
    echo REL =!REL!
    echo OUTDIR =!OUTDIR!

	if not exist "!OUTDIR!" mkdir "!OUTDIR!"

	call "%FXC%" "%%f" /E ps /T ps_5_0 /Fo "!OUTDIR!%%~nf.cso" /Zi /Od || set FAILED=1
)

if %FAILED% neq 0 (
	echo Shader Compile Failed
	exit /b 1
)
exit /b 0