# Basket Library

The Basket Library, or libbasket, is a distributed data structure
library. It consists of several templated data structures built on top
of MPI, including a hashmap, map, multimap, priority queue, and
message queue. There's also a global clock and a sequencer.

## Compilation

The Basket Library compiles with cmake, so the general procedure is

```bash
cd basket
mkdir build
cmake ..
make
sudo make install
```
If you want to install somewhere besides `/usr/local`, then use

```bash
cd basket
mkdir build
cmake -DCMAKE_INSTALL_PREFIX:PATH=/wherever ..
make
make install
```

### Dependencies
- mpi
- boost
- rpclib
- glibc (for librt and posix threads)

## Usage

Since libbasket uses MPI, data structures have to be declared on the
server and clients. Each data structure is declared with a name, a
boolean to indicate whether it is on the server or not, the MPI rank
of the server, and the number of servers it utilizes (generally
one). In the test/ directory, you will find examples for how to use
each data structure, and also for the clock and sequencer. Data
structures typically assume that we are running the server and clients
on the same node.

### GlobalClock

GlobalClock makes the assumption that a node is running a server and
multiple clients (this is the assumption that all structures
make). Technically one node could run multiple sets of servers and
clients, but for our purposes we will call the set of a server with
clients a node. GlobalClock uses RPC calls to access time on other
nodes via the GetTimeServer method. GetTime gets the local node
time. The test is ClockTest, and it runs as an MPI program (most tests
do). ClockTest can have as many ranks as you like, so long as it has
more ranks than the number of servers. The number of servers can be
passed as an argument, but defaults to one. ClockTest will go through
each MPI rank (where ranks 0 to (n-1) are servers and clients are
assigned with modular arithmetic) and output the time at that rank and
the time at each server accessed from that rank with
GetTimeServer(). It uses various MPI barriers to ensure that output
doesn't conflict with itself.

### Testing on Ares cluster

## Configure

```bash
$ mkdir $HOME/basket_build
$ cd $HOME/basket_build
$ $HOME/software/install/bin/cmake \
-DCMAKE_BUILD_TYPE=Debug \
-DCMAKE_C_COMPILER=/opt/ohpc/pub/compiler/gcc/7.3.0/bin/gcc \
-DCMAKE_CXX_COMPILER=/opt/ohpc/pub/compiler/gcc/7.3.0/bin/g++ \
"-DCMAKE_CXX_FLAGS=-I${HOME}/software/install/include -L${HOME}/software/install/lib"\
-G "CodeBlocks - Unix Makefiles" $SRC_DIR
```

where $SRC_DIR = source directory (e.g. /tmp/tmp.bYNfITLGMr)

## Compile

```bash
$ $HOME/software/install/bin/cmake --build ./ --target all -- -j 8
```

## Run

```bash
$ cd $HOME/basket_build/test
$ ctest -V
```
## Patching Mercury 1.0.1 to work with RoCE

For Basket to work with Mercury 1.0.1 with RoCE, it needs a patched version of Mercury.
Assuming that Mercury 1.0.1 has been extracted to `./mercury-1.0.1` run our patch `mercury-1.0.1-RoCE.patch` in the `RoCE_Patch` folder as such 

```bash
$ patch  -p0 <  mercury-1.0.1-RoCE.patch
```

