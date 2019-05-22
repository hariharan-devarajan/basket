// Copyright 2019 Hariharan Devarajan
//
// Created by HariharanDevarajan on 2/1/2019.
//

#ifndef INCLUDE_BASKET_QUEUE_DISTRIBUTED_MESSAGE_QUEUE_H_
#define INCLUDE_BASKET_QUEUE_DISTRIBUTED_MESSAGE_QUEUE_H_

/**
 * Include Headers
 */
#include <basket/communication/rpc_lib.h>
#include <basket/common/singleton.h>
#include <basket/common/debug.h>
/** MPI Headers**/
#include <mpi.h>
/** RPC Lib Headers**/
#include <rpc/server.h>
#include <rpc/client.h>
/** Boost Headers **/
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/deque.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/algorithm/string.hpp>
/** Standard C++ Headers**/
#include <iostream>
#include <functional>
#include <utility>
#include <memory>
#include <string>

/** Namespaces Uses **/
namespace bip = boost::interprocess;

/**
 * This is a Distributed MessageQueue Class. It uses shared memory +
 * RPC + MPI to achieve the data structure.
 *
 * @tparam MappedType, the value of the MessageQueue
 */
template<typename MappedType>
class DistributedMessageQueue {
 private:
  /** Class Typedefs for ease of use **/
  typedef bip::allocator<MappedType,
                         bip::managed_shared_memory::segment_manager>
  ShmemAllocator;
  typedef boost::interprocess::deque<MappedType, ShmemAllocator> Queue;

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
  ~DistributedMessageQueue();

  explicit DistributedMessageQueue(std::string name_,
                                   bool is_server_,
                                   uint16_t my_server_,
                                   int num_servers_);
  bool Push(MappedType data, uint16_t key_int);
  std::pair<bool, MappedType> Pop(uint16_t key_int);
  bool WaitForElement(uint16_t key_int);
  size_t Size(uint16_t key_int);
};

#include "distributed_message_queue.cpp"

#endif  // INCLUDE_BASKET_QUEUE_DISTRIBUTED_MESSAGE_QUEUE_H_
