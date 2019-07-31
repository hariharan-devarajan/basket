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
#include <basket/common/data_structures.h>
#include <basket/common/debug.h>
#include <basket/common/macros.h>
#include <basket/common/singleton.h>
#include <basket/common/typedefs.h>
#include <mpi.h>
#include <rpc/server.h>
#include <rpc/client.h>

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <thallium.hpp>
#include <thallium/serialization/serialize.hpp>
#include <thallium/serialization/buffer_input_archive.hpp>
#include <thallium/serialization/buffer_output_archive.hpp>
#include <thallium/serialization/stl/array.hpp>
#include <thallium/serialization/stl/complex.hpp>
#include <thallium/serialization/stl/deque.hpp>
#include <thallium/serialization/stl/forward_list.hpp>
#include <thallium/serialization/stl/list.hpp>
#include <thallium/serialization/stl/map.hpp>
#include <thallium/serialization/stl/multimap.hpp>
#include <thallium/serialization/stl/multiset.hpp>
#include <thallium/serialization/stl/pair.hpp>
#include <thallium/serialization/stl/set.hpp>
#include <thallium/serialization/stl/string.hpp>
#include <thallium/serialization/stl/tuple.hpp>
#include <thallium/serialization/stl/unordered_map.hpp>
#include <thallium/serialization/stl/unordered_multimap.hpp>
#include <thallium/serialization/stl/unordered_multiset.hpp>
#include <thallium/serialization/stl/unordered_set.hpp>
#include <thallium/serialization/stl/vector.hpp>

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
namespace tl = thallium;

/* typedefs */
typedef bip::allocator<CharStruct, bip::managed_shared_memory::segment_manager>
ShmemAllocator;
typedef bip::vector<CharStruct, ShmemAllocator> MyVector;

class RPC {
  private:
    bool isInitialized, is_server;
    int my_rank, comm_size, num_servers;
    uint16_t server_port, my_server;
    std::string processor_name;
    std::string name;
#ifdef BASKET_ENABLE_RPCLIB
    std::shared_ptr<rpc::server> rpclib_server;
#endif
#if defined(BASKET_ENABLE_THALLIUM_TCP) || defined(BASKET_ENABLE_THALLIUM_ROCE)
    std::shared_ptr<tl::engine> thallium_engine;
    std::string engine_init_str;
    /*std::promise<void> thallium_exit_signal;

    void runThalliumServer(std::future<void> futureObj){

        while(futureObj.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout){}
        thallium_engine->wait_for_finalize();
    }*/

#endif
    MyVector* server_list;
    really_long memory_allocated;
    boost::interprocess::managed_shared_memory segment;

  public:
  ~RPC();

  RPC(std::string name_, bool is_server_, uint16_t my_server_,
      int num_servers_, std::string processor_name_ = "");

  template <typename F>
  void bind(std::string str, F func);

  void run(size_t workers = RPC_THREADS);

#ifdef BASKET_ENABLE_THALLIUM_ROCE
    template<typename MappedType>
    MappedType prep_rdma_server(tl::endpoint endpoint, tl::bulk &bulk_handle);

    template<typename MappedType>
    tl::bulk prep_rdma_client(MappedType &data);
#endif
  /**
   * Response should be RPCLIB_MSGPACK::object_handle for rpclib and
   * tl::packed_response for thallium/mercury
   */
  template <typename Response, typename... Args>
    Response call(uint16_t server_index,
		  std::string const &func_name,
		  Args... args);
};

#include "rpc_lib.cpp"

#endif  // INCLUDE_BASKET_COMMUNICATION_RPC_LIB_H_
