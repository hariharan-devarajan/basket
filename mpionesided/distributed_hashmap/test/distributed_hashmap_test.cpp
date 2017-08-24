//
// Created by hdevarajan on 8/22/17.
//

#include "../include/distributed_hashmap.h"
#include "util/city.h"
#include <unistd.h>
#include <execinfo.h>
#include <csignal>

uint64 SEED = 10000;
const int MAX_SIZE = 256;

struct MyKey : public Key<MyKey> {
  char keyname[MAX_SIZE];

  MyKey() : keyname() {

  }

  MyKey(const MyKey &n) {
    strcpy(keyname, n.keyname);
  }

  bool equals(MyKey k) override {
    if (keyname == nullptr) {
      return false;
    } else if (strcmp(keyname, k.keyname) == 0) {
      return true;
    } else return false;
  }

  std::string to_string() override {
    if (keyname == nullptr) {
      return "null";
    }
    std::string k = keyname;
    return "{keyname:" + k + "}";
  }

  size_t size() override {
    return sizeof(MyKey);
  }

  size_t hash_code() override {
    size_t hash = CityHash64WithSeed(keyname, strlen(keyname), SEED);
    return hash;
  }

  std::vector<MPI_Datatype> get_types() override {
    return {MPI_CHAR};
  }

  std::vector<MPI_Aint> get_displacements() override {
    return {0};
  }

  std::vector<int> get_block_lengths() override {
    return {MAX_SIZE};
  }
};

struct MyValue : public Value<MyValue> {
  int i;

  MyValue() {}

  MyValue(const MyValue &n) : i(n.i) {
  }

  size_t size() override {
    return sizeof(MyValue);
  }

  std::vector<MPI_Datatype> get_types() override {
    return {MPI_INT};
  }

  std::vector<MPI_Aint> get_displacements() override {
    return {0};
  }

  std::vector<int> get_block_lengths() override {
    return {sizeof(int)};
  }

  std::string to_string() override {
    return "{i:" + std::to_string(i) + "}";
  }
};

void test() {
  int rank, comm_size;
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  DistributedHashMap<MyKey, MyValue> map = DistributedHashMap<MyKey, MyValue>();
  MPI_Barrier(MPI_COMM_WORLD);
  for (int i = 0; i < 100; i++) {
    MyKey k1;
    strcpy(k1.keyname, std::to_string(i).c_str());
    MyValue v1;
    v1.i = i;
    map.put(k1, v1);
  }
  MPI_Barrier(MPI_COMM_WORLD);
  if (rank == 0) {
    for (int i = 0; i < 100; i++) {
      MyKey k1;
      strcpy(k1.keyname, std::to_string(i).c_str());
      MyValue v1;
      v1 = map.get(k1);
      printf("key %s value %d\n", k1.keyname, v1.i);
    }
  }
  map.kill();

}

void handler(int sig) {
  void *array[10];
  size_t size;
  // get void*'s for all entries on the stack
  size = backtrace(array, 300);
  int rank, comm_size;
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d rank %d:\n", sig, rank);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(0);
}

int main(int argc, char *argv[]) {
#ifdef DEBUG
  signal(SIGSEGV, handler);
  signal(SIGABRT, handler);
#endif
  MPI_Init(&argc, &argv);
  test();
  //MPI_Barrier(MPI_COMM_WORLD);
  MPI_Finalize();
  return 0;
}

