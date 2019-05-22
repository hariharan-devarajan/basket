// Copyright 2019 Hariharan Devarajan
//
// Created by HariharanDevarajan on 2/1/2019.
//

#ifndef INCLUDE_BASKET_PRIORITY_QUEUE_DISTRIBUTED_PRIORITY_QUEUE_H_
#define INCLUDE_BASKET_PRIORITY_QUEUE_DISTRIBUTED_PRIORITY_QUEUE_H_

/**
 * Include Headers
 */
#include <basket/communication/rpc_lib.h>
#include <basket/common/singleton.h>
#include <basket/common/debug.h>
#include <basket/common/typedefs.h>
/** MPI Headers**/
#include <mpi.h>
/** RPC Lib Headers**/
#include <rpc/server.h>
#include <rpc/client.h>
/** Boost Headers **/
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/algorithm/string.hpp>
/** Standard C++ Headers**/
#include <iostream>
#include <functional>
#include <utility>
#include <queue>
#include <string>
#include <memory>
#include <vector>

/** Namespaces Uses **/
namespace bip = boost::interprocess;

/** Global Typedefs **/

/**
 * This is a Distributed PriorityQueue Class. It uses shared memory + RPC + MPI
 * to achieve the data structure.
 *
 * @tparam MappedType, the value of the PriorityQueue
 */
template<typename MappedType, typename Compare = std::less<MappedType>>
class DistributedPriorityQueue {
 private:
  /** Class Typedefs for ease of use **/
  typedef bip::allocator<MappedType,
                         bip::managed_shared_memory::segment_manager>
  ShmemAllocator;
  typedef std::priority_queue<MappedType,
                              std::vector<MappedType, ShmemAllocator>, Compare>
  Queue;

  /** Class attributes**/
  int comm_size, my_rank, num_servers;
  uint16_t  my_server;
  std::shared_ptr<RPC> rpc;
  really_long memory_allocated;
  bool is_server;
  boost::interprocess::managed_shared_memory segment;
  std::string name, func_prefix;
  Queue *queue;
  boost::interprocess::interprocess_mutex* mutex;

 public:
  ~DistributedPriorityQueue();

  explicit DistributedPriorityQueue(std::string name_, bool is_server_,
                                    uint16_t my_server_, int num_servers_);
  bool Push(MappedType data, uint16_t key_int);
  std::pair<bool, MappedType> Pop(uint16_t key_int);
  std::pair<bool, MappedType> Top(uint16_t key_int);
  size_t Size(uint16_t key_int);
};

#include "distributed_priority_queue.cpp"

#endif  // INCLUDE_BASKET_PRIORITY_QUEUE_DISTRIBUTED_PRIORITY_QUEUE_H_
