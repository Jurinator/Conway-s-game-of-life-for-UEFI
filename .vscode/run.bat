@echo off
setlocal

cd /d D:\workspaces\asm\efi

REM ---- VS environment ----
call "D:\bin\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

REM ---- Build folder ----
if not exist build mkdir build

REM ---- Compile C++ ----
cl.exe ^
 /c hello.c ^
 /Fo:build\hello.obj ^
 /GS- /GR- /EHs- /GR- ^
 /Oi /Os /nologo /Zl

if errorlevel 1 goto :fail

REM ---- Link as EFI ----
link.exe ^
 /nologo ^
 /SUBSYSTEM:EFI_APPLICATION ^
 /ENTRY:efi_main ^
 /NODEFAULTLIB ^
 /MACHINE:X64 ^
 build\hello.obj ^
 /OUT:build\BOOTX64.EFI

if errorlevel 1 goto :fail

echo Built EFI.

REM ---- FIX 1: Copy BOTH OVMF files from your ovmf folder to build folder ----
copy /Y "ovmf\OVMF_CODE.fd" "build\ovmf-code-x86_64.fd"
copy /Y "ovmf\OVMF_VARS.fd" "build\ovmf-vars-x86_64.fd"

REM ---- FIX 2: Check if E: drive is actually there ----
if exist E:\ (
    md E:\EFI\BOOT 2>nul
    copy /Y build\BOOTX64.EFI E:\EFI\BOOT\BOOTX64.EFI
) else (
    echo [SKIP] Drive E: not found. Skipping copy to USB.
)

echo Running in QEMU...

REM Note: Run VS Code/Terminal as Admin to use PhysicalDrive
"D:\bin\qemu\qemu-system-x86_64.exe" ^
 -drive if=pflash,format=raw,readonly=on,file=build\ovmf-code-x86_64.fd ^
 -drive if=pflash,format=raw,file=build\ovmf-vars-x86_64.fd ^
 -serial stdio ^
 -vga std ^
 -display sdl,window-close=on ^
 -drive file=\\.\PhysicalDrive2,format=raw,if=ide,index=0 ^
 -boot menu=on

goto :eof

:fail
echo Build failed.

PAUSE
exit /b 1
