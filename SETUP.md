# How to Setup #

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
