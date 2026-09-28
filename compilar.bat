@echo off
set RAY=third_party\raylib-src\raylib-5.5_win64_mingw-w64
gcc -std=c11 -O2 -Wall -Wextra -finput-charset=UTF-8 -fexec-charset=UTF-8 -I %RAY%\include -o cubo.exe c\main.c c\estado.c c\fronteira.c c\busca.c c\tela.c -L %RAY%\lib -lraylib -lopengl32 -lgdi32 -lwinmm
if errorlevel 1 exit /b 1
echo.
echo Compilado em C: cubo.exe
echo Rode:  .\cubo.exe
echo Texto: .\cubo.exe --console
echo Teste: .\cubo.exe --teste
