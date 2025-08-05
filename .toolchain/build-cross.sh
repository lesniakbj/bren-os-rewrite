#!/bin/bash
set -e # Exit on error

export PREFIX="/mnt/data/projects/os-dev/.toolchain/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

GCC_VERSION=13.3.0
BINUTILS_VERSION=2.42
GDB_VERSION=14.2

cd /mnt/data/projects/os-dev/.toolchain/cross-src

echo "--- Downloading Sources ---"
wget -q -c https://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VERSION.tar.gz
wget -q -c https://ftp.gnu.org/gnu/gcc/gcc-$GCC_VERSION/gcc-$GCC_VERSION.tar.gz
wget -q -c https://ftp.gnu.org/gnu/gdb/gdb-$GDB_VERSION.tar.gz

echo "--- Extracting Sources ---"
tar -xf binutils-$BINUTILS_VERSION.tar.gz
tar -xf gcc-$GCC_VERSION.tar.gz
tar -xf gdb-$GDB_VERSION.tar.gz

echo "--- Building Binutils ---"
mkdir -p build-binutils
cd build-binutils
../binutils-$BINUTILS_VERSION/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make -j$(nproc)
make -j$(nproc) install
cd ..

echo "--- Building GCC ---"
# The $PREFIX/bin dir must be in the PATH for this to work
which -- $TARGET-as || (echo "$TARGET-as is not in the PATH" && exit 1)
mkdir -p build-gcc
cd build-gcc
../gcc-$GCC_VERSION/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers
make -j$(nproc) all-gcc
make -j$(nproc) all-target-libgcc
make -j$(nproc) install-gcc
make -j$(nproc) install-target-libgcc
cd ..

echo "--- Building GDB ---"
mkdir -p build-gdb
cd build-gdb
../gdb-$GDB_VERSION/configure --target=$TARGET --prefix="$PREFIX" --disable-werror
make -j$(nproc)
make -j$(nproc) install
cd ..

echo "--- Cross-compiler build complete! ---"