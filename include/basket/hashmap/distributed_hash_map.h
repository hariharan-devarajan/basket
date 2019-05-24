/*
 * Copyright (C) 2019  Hariharan Devarajan, Keith Bateman
 *
 * This file is part of Basket
 * 
 * Basket is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDE_BASKET_HASHMAP_DISTRIBUTED_HASH_MAP_H_
#define INCLUDE_BASKET_HASHMAP_DISTRIBUTED_HASH_MAP_H_

/**
 * Include Headers
 */

#include <basket/communication/rpc_lib.h>
#include <basket/common/singleton.h>
#include <basket/common/typedefs.h>
/** MPI Headers**/
#include <mpi.h>
/** RPC Lib Headers**/
#include <rpc/server.h>
#include <rpc/client.h>
#include <rpc/rpc_error.h>
/** Boost Headers **/
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/unordered_map.hpp>
#include <boost/functional/hash.hpp>
#include <boost/algorithm/string.hpp>
/** Standard C++ Headers**/
#include <iostream>
#include <functional>
#include <utility>
#include <stdexcept>
#include <memory>
#include <string>
#include <vector>

/** Namespaces Uses **/

/** Global Typedefs **/

/**
 * This is a Distributed HashMap Class. It uses shared memory + RPC + MPI to
 * achieve the data structure.
 *
 * @tparam MappedType, the value of the HashMap
 */
template<typename KeyType, typename MappedType>
class DistributedHashMap {
 private:
  std::hash<KeyType> keyHash;
  /** Class Typedefs for ease of use **/
  typedef std::pair<const KeyType, MappedType> ValueType;
  typedef boost::interprocess::allocator<ValueType, boost::interprocess::
                                         managed_shared_memory::segment_manager>
  ShmemAllocator;
  typedef boost::unordered_map<KeyType, MappedType, std::hash<KeyType>,
                               std::equal_to<KeyType>,
                               ShmemAllocator> MyHashMap;
  /** Class attributes**/
  int comm_size, my_rank, num_servers;
  uint16_t  my_server;
  std::shared_ptr<RPC> rpc;
  really_long memory_allocated;
  bool is_server;
  boost::interprocess::managed_shared_memory segment;
  std::string name, func_prefix;
  MyHashMap *myHashMap;
  boost::interprocess::interprocess_mutex* mutex;

 public:
  ~DistributedHashMap();

  explicit DistributedHashMap(std::string name_, bool is_server_,
                              uint16_t my_server_, int num_servers_);
  bool Put(KeyType key, MappedType data);
  std::pair<bool, MappedType> Get(KeyType key);
  std::pair<bool, MappedType> Erase(KeyType key);
  std::vector<std::pair<KeyType, MappedType>> GetAllData();
  std::vector<std::pair<KeyType, MappedType>> GetAllDataInServer();
};

#include "distributed_hash_map.cpp"

#endif  // INCLUDE_BASKET_HASHMAP_DISTRIBUTED_HASH_MAP_H_
