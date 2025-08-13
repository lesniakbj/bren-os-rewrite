#!/bin/bash
set -e # Exit on error

export PREFIX="/mnt/data/projects/os-dev/.toolchain/cross"
export TARGET_I686=i686-elf
export TARGET_X86_64=x86_64-elf
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

# Build i686-elf toolchain
echo "--- Building i686-elf Binutils ---"
mkdir -p build-binutils-i686
cd build-binutils-i686
../binutils-$BINUTILS_VERSION/configure --target=$TARGET_I686 --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make -j$(nproc)
make -j$(nproc) install
cd ..

echo "--- Building i686-elf GCC ---"
# The $PREFIX/bin dir must be in the PATH for this to work
which -- $TARGET_I686-as || (echo "$TARGET_I686-as is not in the PATH" && exit 1)
mkdir -p build-gcc-i686
cd build-gcc-i686
../gcc-$GCC_VERSION/configure --target=$TARGET_I686 --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers
make -j$(nproc) all-gcc
make -j$(nproc) all-target-libgcc
make -j$(nproc) install-gcc
make -j$(nproc) install-target-libgcc
cd ..

echo "--- Building i686-elf GDB ---"
mkdir -p build-gdb-i686
cd build-gdb-i686
../gdb-$GDB_VERSION/configure --target=$TARGET_I686 --prefix="$PREFIX" --disable-werror
make -j$(nproc)
make -j$(nproc) install
cd ..

# Build x86_64-elf toolchain
echo "--- Building x86_64-elf Binutils ---"
mkdir -p build-binutils-x86_64
cd build-binutils-x86_64
../binutils-$BINUTILS_VERSION/configure --target=$TARGET_X86_64 --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make -j$(nproc)
make -j$(nproc) install
cd ..

echo "--- Building x86_64-elf GCC ---"
# The $PREFIX/bin dir must be in the PATH for this to work
which -- $TARGET_X86_64-as || (echo "$TARGET_X86_64-as is not in the PATH" && exit 1)
mkdir -p build-gcc-x86_64
cd build-gcc-x86_64
../gcc-$GCC_VERSION/configure --target=$TARGET_X86_64 --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers
make -j$(nproc) all-gcc
make -j$(nproc) all-target-libgcc
make -j$(nproc) install-gcc
make -j$(nproc) install-target-libgcc
cd ..

echo "--- Building x86_64-elf GDB ---"
mkdir -p build-gdb-x86_64
cd build-gdb-x86_64
../gdb-$GDB_VERSION/configure --target=$TARGET_X86_64 --prefix="$PREFIX" --disable-werror
make -j$(nproc)
make -j$(nproc) install
cd ..

echo "--- Cross-compiler builds complete! ---"
echo "Both i686-elf and x86_64-elf toolchains are now available."