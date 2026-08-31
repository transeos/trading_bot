# How to Setup #

> This project was originally written on Linux. For building on **Windows**
> (MSVC / Visual Studio + vcpkg), skip to [Windows Setup](#windows-setup-msvc--visual-studio--vcpkg)
> at the bottom of this file.

# Linux #

## prerequisites
sudo apt-get install autoconf
sudo apt-get install libtool
sudo apt-get install libcurl4-gnutls-dev
sudo apt-get install curl
sudo apt-get install cmake
sudo apt-get install clang

export APP_DIR=/home/<user>/trading_bot

## TCMalloc
sudo apt-get install google-perftools
export LD_PRELOAD="/usr/lib/libtcmalloc.so.4" (need to be set before compilation)
export HEAPCHECK=normal (to check whether TCMalloc is working)

## cassandra setup
echo "deb http://www.apache.org/dist/cassandra/debian 311x main" | sudo tee -a /etc/apt/sources.list.d/cassandra.sources.list
curl https://www.apache.org/dist/cassandra/KEYS | sudo apt-key add -
sudo apt-get update
sudo apt-get install cassandra
sudo service cassandra start

sudo add-apt-repository ppa:acooks/libwebsockets6
sudo apt-get update
sudo apt-get install libuv1.dev
sudo apt-get install libssl-dev (might need to enable precise-updates repository in ubuntu)

## download git repository
git clone --recursive https://github.com/transeos/trading_bot
## or update submodules (inside 'trading_bot' repository)
git submodule update --init --recursive

## build ta-lib
cd $APP_DIR/3rdparty/ta-lib-rt/ta-lib
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=. ../
make install PREFIX=$(pwd)

## build cassandra
cd $APP_DIR/3rdparty/cpp-driver
mkdir build
cd build
cmake ../ -DCMAKE_INSTALL_PREFIX=$(pwd)
make
make install PREFIX=$(pwd)

## build cryptopp
cd $APP_DIR/3rdparty/cryptopp
mkdir build
make libcryptopp.a libcryptopp.so cryptest.exe
make install PREFIX=$(pwd)/build


## build CryptoTrader
cd $APP_DIR
mkdir build
cd build
cmake ..
make

## build CryptoTrader with strict compile (might require changes in 3rd party libraries)
cd $APP_DIR
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../CMake/toolchain.cmake ../
make

## build CryptoTrader with Ninja
sudo apt install ninja-build
cd $APP_DIR
mkdir build
cd build
cmake -GNinja -DCMAKE_TOOLCHAIN_FILE=../CMake/toolchain.cmake ../ 
ninja

## Now you'll have "../trading_bot/build/cryptotrader" executable file.


# Windows Setup (MSVC / Visual Studio + vcpkg) #

These steps produce `build\cryptotrader.exe` (and `build\test_cryptotrader.exe`)
with the native MSVC toolchain. All shell snippets are **PowerShell** and assume
you start from the repository root (`c:\git\trading_bot`).

## prerequisites

- **Visual Studio 2022 or newer** with the **"Desktop development with C++"**
  workload (provides the MSVC `cl.exe` compiler, the Windows SDK) and the
  **"C++ CMake Tools for Windows"** component (provides CMake + Ninja).
- **Git**.
- **vcpkg** (installed below) for the C/C++ library dependencies.

Open the **"Developer PowerShell for VS"** from the Start menu so `cl` is on the
PATH. If `cmake` / `ninja` are not found, either add the bundled copies to your
PATH, e.g.

    $vs = "C:\Program Files\Microsoft Visual Studio\18\Community"   # adjust edition/year
    $env:PATH = "$vs\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;" +
                "$vs\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;$env:PATH"

or install CMake (>= 3.15) standalone from https://cmake.org/download/.

> Everything below builds in **Release / x64**. Keep the dependencies and the app
> on the same configuration so the C runtime (`/MD`) matches.

## get the source

    git clone --recursive https://github.com/transeos/trading_bot
    cd trading_bot
    git submodule update --init --recursive

## websocketpp headers

The repository references `3rdparty/websocketpp` but only ships broken symlinks
for it, so the header tree has to be fetched once (asio 1.10.8 pairs with
websocketpp 0.8.2):

    git clone --depth 1 --branch 0.8.2 https://github.com/zaphoyd/websocketpp.git $env:TEMP\wspp
    Remove-Item -Recurse -Force .\3rdparty\websocketpp\websocketpp -ErrorAction SilentlyContinue
    Copy-Item -Recurse $env:TEMP\wspp\websocketpp .\3rdparty\websocketpp\websocketpp

## vcpkg dependencies (curl, openssl, crypto++, libuv, zlib)

    git clone https://github.com/microsoft/vcpkg C:\vcpkg
    C:\vcpkg\bootstrap-vcpkg.bat
    C:\vcpkg\vcpkg.exe install curl openssl cryptopp libuv zlib --triplet x64-windows

(OpenSSL is built from source and takes the longest.)

## build ta-lib (static)

    cmake -S 3rdparty/ta-lib-rt/ta-lib -B 3rdparty/ta-lib-rt/ta-lib/build -G Ninja `
      -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 `
      -DCMAKE_INSTALL_PREFIX=3rdparty/ta-lib-rt/ta-lib/build/install `
      -DTA_LIB_ENABLE_TESTS=OFF -DTA_LIB_ENABLE_GEN_CODE=OFF
    cmake --build 3rdparty/ta-lib-rt/ta-lib/build --config Release --target install

## build the Cassandra cpp-driver

The driver finds libuv/OpenSSL/zlib from vcpkg. `LIBUV_ROOT_DIR` is required
because the driver's `FindLibuv` only searches that hint.

    cmake -S 3rdparty/cpp-driver -B 3rdparty/cpp-driver/build -G Ninja `
      -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 `
      -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows `
      -DLIBUV_ROOT_DIR=C:/vcpkg/installed/x64-windows `
      -DCMAKE_INSTALL_PREFIX=3rdparty/cpp-driver/build/install `
      -DCASS_BUILD_SHARED=ON -DCASS_BUILD_STATIC=OFF -DCASS_USE_ZLIB=ON `
      -DCASS_BUILD_EXAMPLES=OFF -DCASS_BUILD_TESTS=OFF -DCASS_INSTALL_PKG_CONFIG=OFF
    cmake --build 3rdparty/cpp-driver/build --config Release --target install

> **Note (CMake >= 4.0):** the cpp-driver (2018) forces a few very old CMake
> policies to `OLD`, which CMake 4.x rejects. `cmake/modules/CppDriver.cmake` in
> this tree already guards those `cmake_policy(SET CMP0042/CMP0048/CMP0054 OLD)`
> calls with `AND CMAKE_VERSION VERSION_LESS "4.0"`. If you build from a fresh
> submodule checkout and hit a `cmake_policy` error, apply the same guard.

## configure and build the application

    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release `
      -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows
    cmake --build build --config Release

vcpkg copies `libcurl.dll`, `libssl-*.dll`, `libcrypto-*.dll` and `z.dll` next to
the executables automatically. Copy the two remaining runtime DLLs (the Cassandra
driver and its libuv dependency):

    Copy-Item 3rdparty/cpp-driver/build/install/bin/cassandra.dll build/
    Copy-Item C:/vcpkg/installed/x64-windows/bin/uv.dll build/

## run

    $env:TRADER_HOME = (Get-Location).Path
    .\build\cryptotrader.exe

You should see the "Welcome to cryptotrader" banner and initialization output.
Live data/trading and the test suite need a running **Cassandra** server reachable
at `127.0.0.1` (set `g_cassandra_ip` / config otherwise); without one the app logs
"not able to connect to cassandra database", which confirms the driver is working.

## what was changed for Windows

- `lib/include/utils/PlatformCompat.h` — new cross-platform shim (force-included
  on Windows via `/FI`) providing `getcwd`, `timegm`, `clock_gettime`, `ssize_t`,
  and neutralising the Win32 `DELETE` macro.
- `bin/main.cpp` — Windows `_spawnv` supervisor in place of `fork()`/`wait()`,
  plus ANSI-colour console enablement.
- `lib/src/TraderBot.cpp` — signal handling guarded (`SIGTSTP`/`SIGCONT` are
  POSIX-only) and `rm -rf` replaced with `std::filesystem`.
- `lib/src/utils/TraderUtils.cpp` — `mkdir -p` and `tm_gmtoff` replaced with
  portable equivalents.
- Assorted portability fixes (`\033` escapes, `std::gcd`, explicit returns,
  missing `<deque>`/`<numeric>` includes).
- `CMakeLists.txt` / `lib/CMakeLists.txt` — MSVC flags, vcpkg `find_package`,
  static `TraderBot` on Windows.
