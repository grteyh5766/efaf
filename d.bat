@echo off
title Kernel Driver (Disable USB debugging)
mode con: cols=55 lines=20
COLOR 0D

:: Ativa variáveis com atraso
setlocal enabledelayedexpansion

:: Exibe a chuva de letras verdes por alguns segundos
cls
timeout /t 1 >nul
cls

for /L %%n in (1,1,30) do (
    for /L %%i in (1,1,50) do (
        set /a "rand=!random! %% 2"
        <nul set /p="!rand! "
    )
    echo.
    timeout /nobreak /t 0 >nul
)
timeout /t 1 >nul
cls

:: Início do script principal (saída de todos os comandos suprimida)
cd C:\platform-tools >nul 2>&1
setlocal enabledelayedexpansion

set "ADB_PATH=%USERPROFILE%\Desktop\Release\platform-tools\adb.exe"

cd /d "%USERPROFILE%\Desktop\Release\platform-tools"


adb shell settings put global adb_enabled 0


echo.
echo [+] Depuração desativada com sucesso!
echo.
echo CASO encontre alguma falha contate o seu vendedor.
pause > nul

:: Aguarda 15 segundos antes de se excluir
timeout /t 15 >nul

:: Autoexclusão do script
del /f /q "%~f0"
exit
