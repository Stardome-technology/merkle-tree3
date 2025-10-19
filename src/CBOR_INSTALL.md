# CBOR Installation Instructions

## Installing libcbor

### Ubuntu/Debian
```bash
sudo apt update
sudo apt install libcbor-dev
```

### CentOS/RHEL/Fedora
```bash
# Fedora
sudo dnf install libcbor-devel

# CentOS/RHEL with EPEL
sudo yum install epel-release
sudo yum install libcbor-devel
```

### macOS
```bash
# Using Homebrew
brew install libcbor

# Using MacPorts
sudo port install libcbor
```

### Windows (MSYS2/MinGW)
```bash
# Using MSYS2
pacman -S mingw-w64-x86_64-libcbor

# Using vcpkg
vcpkg install libcbor
```

### Building from Source
```bash
git clone https://github.com/PJK/libcbor.git
cd libcbor
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

## Building with CBOR Support

### Using CMake (Recommended)
```bash
mkdir build && cd build
cmake .. -DWITH_CBOR=ON
make
```

### Using Make
```bash
# With CBOR (default if libcbor found)
make

# Force disable CBOR
make WITH_CBOR=0
```

### Testing CBOR Support
```bash
# Run tests (includes CBOR tests if compiled with support)
make test

# Or run directly
./bin/test_merkle_tree
```

## Troubleshooting

### pkg-config Issues
If `pkg-config` can't find libcbor:
```bash
# Find the .pc file
find /usr -name "libcbor.pc" 2>/dev/null

# Add to PKG_CONFIG_PATH if needed
export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH"
```

### Manual Linking
If pkg-config is not available, you can link manually:
```bash
# Add to Makefile/CMakeLists.txt
LIBS += -lcbor
CFLAGS += -I/usr/local/include
```

### Windows Specific
For Windows development, ensure libcbor is in your PATH and library search paths:
```batch
# Add to environment
set PATH=%PATH%;C:\path\to\libcbor\bin
set LIB=%LIB%;C:\path\to\libcbor\lib
set INCLUDE=%INCLUDE%;C:\path\to\libcbor\include
```