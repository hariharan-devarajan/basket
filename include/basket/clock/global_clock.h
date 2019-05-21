// Copyright 2019 Hariharan Devarajan
//
// Created by hariharan on 2/25/19.
//

#ifndef SRC_BASKET_CLOCK_GLOBAL_CLOCK_H_
#define SRC_BASKET_CLOCK_GLOBAL_CLOCK_H_

#include <stdint-gcc.h>
#include <mpi.h>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <basket/common/singleton.h>
#include <basket/common/data_structures.h>
#include <basket/common/typedefs.h>
#include <basket/communication/rpc_lib.h>
#include <basket/common/debug.h>
#include <utility>
#include <memory>
#include <string>

namespace bip = boost::interprocess;
class GlobalClock {
 private:
  typedef std::chrono::high_resolution_clock::time_point chrono_time;
  chrono_time *start;
  bool is_server;
  bip::interprocess_mutex* mutex;
  really_long memory_allocated;
  int my_rank, comm_size, num_servers;
  uint16_t my_server;
  bip::managed_shared_memory segment;
  std::string name, func_prefix;
  std::shared_ptr<RPC> rpc;

 public:
  ~GlobalClock();
  GlobalClock(std::string name_,
              bool is_server_,
              uint16_t my_server_,
              int num_servers_);

  HTime GetTime();
  HTime GetTimeServer(uint16_t server);
};
#endif  // SRC_BASKET_CLOCK_GLOBAL_CLOCK_H_
