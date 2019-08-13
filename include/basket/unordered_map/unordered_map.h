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

#ifndef INCLUDE_BASKET_UNORDERED_MAP_UNORDERED_MAP_H_
#define INCLUDE_BASKET_UNORDERED_MAP_UNORDERED_MAP_H_

/**
 * Include Headers
 */

/** Standard C++ Headers**/
#include <iostream>
#include <functional>
#include <utility>
#include <stdexcept>
#include <memory>
#include <string>
#include <vector>
#include <tuple>

#include <basket/communication/rpc_lib.h>
#include <basket/common/singleton.h>
#include <basket/common/typedefs.h>


/** MPI Headers**/
#include <mpi.h>
/** RPC Lib Headers**/
#ifdef BASKET_ENABLE_RPCLIB
#include <rpc/server.h>
#include <rpc/client.h>
#include <rpc/rpc_error.h>
#endif
/** Thallium Headers **/
#if defined(BASKET_ENABLE_THALLIUM_TCP) || defined(BASKET_ENABLE_THALLIUM_ROCE)
#include <thallium.hpp>
#endif
/** Boost Headers **/
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/functional/hash.hpp>
#include <boost/algorithm/string.hpp>

/** Namespaces Uses **/

/** Global Typedefs **/

namespace basket {
/**
 * This is a Distributed HashMap Class. It uses shared memory + RPC + MPI to
 * achieve the data structure.
 *
 * @tparam MappedType, the value of the HashMap
 */
template<typename KeyType, typename MappedType>
class unordered_map {
  private:
    std::hash<KeyType> keyHash;
    /** Class Typedefs for ease of use **/
    typedef std::pair<const KeyType, MappedType> ValueType;
    typedef boost::interprocess::allocator<ValueType, boost::interprocess::
                                           managed_shared_memory::segment_manager>
    ShmemAllocator;
    typedef boost::unordered::unordered_map<KeyType, MappedType, std::hash<KeyType>,
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
    bool server_on_node;
    std::unordered_map<std::string, std::string> binding_map;

  public:
    ~unordered_map();

    explicit unordered_map(std::string name_, bool is_server_,
                           uint16_t my_server_, int num_servers_,
                           bool server_on_node_,
                           std::string processor_name_ = "");

    template <typename F>
    void Bind(std::string rpc_name, F fun);

    void BindClient(std::string rpc_name);

    bool LocalPut(KeyType &key, MappedType &data);
    std::pair<bool, MappedType> LocalGet(KeyType &key);
    std::pair<bool, MappedType> LocalErase(KeyType &key);
    std::vector<std::pair<KeyType, MappedType>> LocalGetAllDataInServer();

#if defined(BASKET_ENABLE_THALLIUM_TCP) || defined(BASKET_ENABLE_THALLIUM_ROCE)
    THALLIUM_DEFINE(LocalPut, (key,data) ,KeyType &key, MappedType &data)

    // void ThalliumLocalPut(const tl::request &thallium_req, tl::bulk &bulk_handle, KeyType key) {
    //     MappedType data = rpc->prep_rdma_server<MappedType>(thallium_req.get_endpoint(), bulk_handle);
    //     thallium_req.respond(LocalPut(key, data));
    // }

    // void ThalliumLocalGet(const tl::request &thallium_req, KeyType key) {
    //     auto retpair = LocalGet(key);
    //     if (!retpair.first) {
    //         printf("error\n");
    //     }
    //     MappedType data = retpair.second;
    //     tl::bulk bulk_handle = rpc->prep_rdma_client<MappedType>(data);
    //     thallium_req.respond(bulk_handle);
    // }

    THALLIUM_DEFINE(LocalGet, (key), KeyType &key)
    THALLIUM_DEFINE(LocalErase, (key), KeyType &key)
    THALLIUM_DEFINE1(LocalGetAllDataInServer)
#endif

    bool Put(KeyType &key, MappedType &data);
    std::pair<bool, MappedType> Get(KeyType &key);
    std::pair<bool, MappedType> Erase(KeyType &key);
    std::vector<std::pair<KeyType, MappedType>> GetAllData();
    std::vector<std::pair<KeyType, MappedType>> GetAllDataInServer();

    template<typename... CB_Tuple_Args>
    bool LocalPutWithCallback(KeyType &key, MappedType &data,
                              std::string cb_name,
                              std::tuple<CB_Tuple_Args...> cb_args);
    // std::pair<bool, MappedType> LocalGet(KeyType &key);
    // std::pair<bool, MappedType> LocalErase(KeyType &key);
    // std::vector<std::pair<KeyType, MappedType>> LocalGetAllDataInServer();

    template<typename... CB_Args>
    bool PutWithCallback(KeyType &key, MappedType &data,
                         std::string cb_name,
                         CB_Args... cb_args);

    template<typename... CB_Tuple_Args, size_t... Is>
    void callWithCallbackSequence(std::string cb_name,
                                  std::tuple<CB_Tuple_Args...> cb_args,
                                  std::index_sequence<Is...> sequence);
};

#include "unordered_map.cpp"

}  // namespace basket

#endif  // INCLUDE_BASKET_UNORDERED_MAP_UNORDERED_MAP_H_
