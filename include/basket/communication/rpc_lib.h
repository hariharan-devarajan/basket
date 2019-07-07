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

#ifndef INCLUDE_BASKET_COMMUNICATION_RPC_LIB_H_
#define INCLUDE_BASKET_COMMUNICATION_RPC_LIB_H_


#include <basket/common/constants.h>
#include <basket/common/typedefs.h>
#include <basket/common/data_structures.h>
#include <basket/common/debug.h>
#include <rpc/server.h>
#include <mpi.h>
#include <rpc/client.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <cstdint>
#include <utility>
#include <memory>
#include <string>
#include <vector>

namespace bip = boost::interprocess;
/* typedefs */
typedef bip::allocator<CharStruct, bip::managed_shared_memory::segment_manager>
ShmemAllocator;
typedef bip::vector<CharStruct, ShmemAllocator> MyVector;

class RPC {
 private:
  bool isInitialized, is_server;
  int my_rank, comm_size, num_servers;
  uint16_t server_port, my_server;
  std::string name;
  std::shared_ptr<rpc::server> server;
  MyVector* server_list;
  really_long memory_allocated;
  boost::interprocess::managed_shared_memory segment;

 public:
  ~RPC();
  RPC(std::string name_, bool is_server_, uint16_t my_server_,
      int num_servers_);
  template <typename F> void bind(std::string str, F func);

  void run(size_t workers = RPC_THREADS);
  template <typename... Args>
  RPCLIB_MSGPACK::object_handle call(uint16_t server_index,
                                     std::string const &func_name,
                                     Args... args);
  template <typename... Args>
  std::future<RPCLIB_MSGPACK::object_handle> async_call(
      uint16_t server_index, std::string const &func_name,
      Args... args);
};

#include "rpc_lib.cpp"

#endif  // INCLUDE_BASKET_COMMUNICATION_RPC_LIB_H_
