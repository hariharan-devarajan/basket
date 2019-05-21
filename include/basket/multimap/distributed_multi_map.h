// Copyright 2019 Hariharan Devarajan
//
// Created by HariharanDevarajan on 2/1/2019.
//

#ifndef SRC_BASKET_MULTIMAP_DISTRIBUTED_MULTI_MAP_H_
#define SRC_BASKET_MULTIMAP_DISTRIBUTED_MULTI_MAP_H_

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
#include <boost/interprocess/containers/map.hpp>
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
#include <vector>

/**
 * This is a Distributed MultiMap Class. It uses shared memory + RPC + MPI to
 * achieve the data structure.
 *
 * @tparam MappedType, the value of the MultiMap
 */
template<typename KeyType, typename MappedType, typename Compare =
         std::less<KeyType>>
class DistributedMultiMap {
 private:
  std::hash<KeyType> keyHash;
  /** Class Typedefs for ease of use **/
  typedef std::pair<const KeyType, MappedType> ValueType;
  typedef boost::interprocess::allocator<
    ValueType, boost::interprocess::managed_shared_memory::segment_manager>
  ShmemAllocator;
  typedef boost::interprocess::multimap<KeyType, MappedType, Compare,
                                        ShmemAllocator> MyMap;
  /** Class attributes**/
  int comm_size, my_rank, num_servers;
  uint16_t  my_server;
  std::shared_ptr<RPC> rpc;
  really_long memory_allocated;
  bool is_server;
  boost::interprocess::managed_shared_memory segment;
  std::string name, func_prefix;
  MyMap *mymap;
  boost::interprocess::interprocess_mutex* mutex;

 public:
  /* Constructor to deallocate the shared memory*/
  ~DistributedMultiMap();

  DistributedMultiMap();
  explicit DistributedMultiMap(std::string name_, bool is_server_,
                               uint16_t my_server_, int num_servers_);
  bool Put(KeyType key, MappedType data);
  std::pair<bool, MappedType> Get(KeyType key);

  std::pair<bool, MappedType> Erase(KeyType key);
  std::vector<std::pair<KeyType, MappedType>> Contains(KeyType key);

  std::vector<std::pair<KeyType, MappedType>> GetAllData();

  std::vector<std::pair<KeyType, MappedType>> ContainsInServer(KeyType key);
  std::vector<std::pair<KeyType, MappedType>> GetAllDataInServer();
};


#endif  // SRC_BASKET_MULTIMAP_DISTRIBUTED_MULTI_MAP_H_
