// Copyright 2019 Hariharan Devarajan
//
// Created by hariharan on 2/19/19.
//

#ifndef INCLUDE_BASKET_SEQUENCER_GLOBAL_SEQUENCE_H_
#define INCLUDE_BASKET_SEQUENCER_GLOBAL_SEQUENCE_H_

#include <basket/communication/rpc_lib.h>
#include <basket/common/singleton.h>
#include <stdint-gcc.h>
#include <mpi.h>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <utility>
#include <memory>
#include <string>

namespace bip = boost::interprocess;

class GlobalSequence{
 private:
  uint64_t* value;
  bool is_server;
  bip::interprocess_mutex* mutex;
  int my_rank, comm_size, num_servers;
  uint16_t my_server;
  really_long memory_allocated;
  bip::managed_shared_memory segment;
  std::string name, func_prefix;
  std::shared_ptr<RPC> rpc;

 public:
  ~GlobalSequence();
  GlobalSequence(std::string name_, bool is_server_, uint16_t my_server_,
                 int num_servers_);

  uint64_t GetNextSequence();
  uint64_t GetNextSequenceServer(uint16_t server);
};

#endif  // INCLUDE_BASKET_SEQUENCER_GLOBAL_SEQUENCE_H_
