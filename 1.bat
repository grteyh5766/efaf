@echo off
title Kernel Driver (ByPass 1.1) (Para Eduzada)
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
set "MREPLAYS_PATH=%USERPROFILE%\Desktop\Release\MReplays"

cd /d "%USERPROFILE%\Desktop\Release\platform-tools"


rem Envia os arquivos JSON e BIN para o dispositivo Android
adb push "%MREPLAYS_PATH%" /storage/emulated/0/Android/data/com.dts.freefiremax/files/

rem Variáveis para armazenar a última data de modificação
set "data_modificacao="
set "hora_modificacao="

rem Obtém a data do nome do arquivo JSON ou BIN e ajusta para a pasta e os arquivos
for %%A in (%MREPLAYS_PATH%\*.json %MREPLAYS_PATH%\*.bin) do (
    rem Pega a data do nome do arquivo no formato YYYY-MM-DD-HH-MM-SS
    set "filename=%%~nxA"
    
    rem Extraímos a data e hora do nome do arquivo
    set "ano=!filename:~0,4!"
    set "mes=!filename:~5,2!"
    set "dia=!filename:~8,2!"
    set "hora=!filename:~11,2!"
    set "minuto=!filename:~14,2!"
    set "segundo=!filename:~17,2!"

    rem Formata a data no formato adequado (YYYYMMDDhhmm.ss)
    set "data_arquivo=!ano!!mes!!dia!!hora!!minuto!.!segundo!"

    rem Executa o comando touch na pasta MReplays com a mesma data do arquivo
    adb shell "touch -t !data_arquivo! /storage/emulated/0/Android/data/com.dts.freefiremax/files/MReplays"

    adb shell "touch -t !data_arquivo! /storage/emulated/0/Android/data/com.dts.freefiremax/files/MReplays/%%~nxA"
)

echo.
echo [+] Bypass Concluido.
echo.
echo CASO encontre alguma falha contate o seu vendedor.
pause > nul

:: Aguarda 15 segundos antes de se excluir
timeout /t 15 >nul

:: Autoexclusão do script
del /f /q "%~f0"
exit
