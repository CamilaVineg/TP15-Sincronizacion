@echo off
setlocal enabledelayedexpansion

echo ========================================================
echo  Compilador para TP 15 - Ejercicio C (Metodo de Cristian)
echo ========================================================

if not exist bin mkdir bin

:: Verificar si g++ esta disponible
where g++ >nul 2>nul
if %ERRORLEVEL% equ 0 (
    echo [INFO] Detectado g++. Compilando con g++...
    g++ -std=c++17 -Wall -Wextra src\ejercicio_c_cristian\main.cpp -o bin\cristian.exe -lws2_32
    if %ERRORLEVEL% equ 0 (
        echo [OK] Compilacion exitosa: bin\cristian.exe
        exit /b 0
    ) else (
        echo [ERROR] Fallo la compilacion con g++.
        exit /b 1
    )
)

:: Si no hay g++, buscar MSVC Build Tools
set VCVARS="C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if exist %VCVARS% (
    echo [INFO] Detectado MSVC 2019. Cargando entorno de compilacion...
    call %VCVARS% >nul
    cl /nologo /EHsc /std:c++17 /W3 src\ejercicio_c_cristian\main.cpp ws2_32.lib /Fe:bin\cristian.exe /Fo:bin\
    if %ERRORLEVEL% equ 0 (
        echo [OK] Compilacion exitosa: bin\cristian.exe
        exit /b 0
    ) else (
        echo [ERROR] Fallo la compilacion con cl.exe.
        exit /b 1
    )
)

echo [ERROR] No se encontro ni g++ ni Visual Studio Build Tools en el sistema.
exit /b 1
