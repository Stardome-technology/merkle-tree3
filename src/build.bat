@echo off
REM Build script for Merkle Tree C implementation
REM Requires GCC or compatible C compiler

echo Building Merkle Tree C implementation...

REM Try GCC first
where gcc >nul 2>&1
if %ERRORLEVEL% == 0 (
    echo Using GCC...
    gcc -Wall -Wextra -std=c99 -pedantic -O2 -g -o test_merkle_tree merkle_tree.c merkle_tree_secure.c test_merkle_tree.c
    if %ERRORLEVEL% == 0 (
        echo Build successful!
        echo Running tests...
        test_merkle_tree.exe
    ) else (
        echo Build failed!
    )
    goto :end
)

REM Try Microsoft Visual C++ compiler
where cl >nul 2>&1
if %ERRORLEVEL% == 0 (
    echo Using Microsoft Visual C++...
    cl /W4 /O2 /Fe:test_merkle_tree.exe merkle_tree.c merkle_tree_secure.c test_merkle_tree.c
    if %ERRORLEVEL% == 0 (
        echo Build successful!
        echo Running tests...
        test_merkle_tree.exe
    ) else (
        echo Build failed!
    )
    goto :end
)

REM Try Clang
where clang >nul 2>&1
if %ERRORLEVEL% == 0 (
    echo Using Clang...
    clang -Wall -Wextra -std=c99 -pedantic -O2 -g -o test_merkle_tree merkle_tree.c merkle_tree_secure.c test_merkle_tree.c
    if %ERRORLEVEL% == 0 (
        echo Build successful!
        echo Running tests...
        test_merkle_tree.exe
    ) else (
        echo Build failed!
    )
    goto :end
)

echo No C compiler found! Please install:
echo - GCC (MinGW-w64 or similar)
echo - Microsoft Visual Studio with C++ tools
echo - Clang/LLVM
echo.
echo Or compile manually using your preferred compiler with:
echo   your_compiler -o test_merkle_tree merkle_tree.c test_merkle_tree.c

:end
pause