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

#ifndef INCLUDE_BASKET_SET_SET_CPP_
#define INCLUDE_BASKET_SET_SET_CPP_

/* Constructor to deallocate the shared memory*/
template<typename KeyType, typename Compare>
set<KeyType, Compare>::~set() {
    if (is_server)
        boost::interprocess::shared_memory_object::remove(name.c_str());
}

template<typename KeyType, typename Compare>
set<KeyType, Compare>::set()
        : is_server(BASKET_CONF->IS_SERVER), my_server(BASKET_CONF->MY_SERVER),
          num_servers(BASKET_CONF->NUM_SERVERS),
          comm_size(1), my_rank(0), memory_allocated(1024ULL * 1024ULL * 128ULL),
          name(BASKET_CONF->SHMEM_NAME), segment(), myset(), func_prefix(BASKET_CONF->SHMEM_NAME),
          server_on_node(BASKET_CONF->SERVER_ON_NODE) {
    AutoTrace trace = AutoTrace("basket::set");
    /* Initialize MPI rank and size of world */
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    /* create per server name for shared memory. Needed if multiple servers are
       spawned on one node*/
    this->name += "_" + std::to_string(my_server);
    /* if current rank is a server */
    rpc = Singleton<RPC>::GetInstance();
    if (is_server) {
        /* Delete existing instance of shared memory space*/
        boost::interprocess::shared_memory_object::remove(name.c_str());
        /* allocate new shared memory space */
        segment = boost::interprocess::managed_shared_memory(
            boost::interprocess::create_only, name.c_str(), memory_allocated);
        ShmemAllocator alloc_inst(segment.get_segment_manager());
        /* Construct set in the shared memory space. */
        myset = segment.construct<MySet>(name.c_str())(Compare(), alloc_inst);
        mutex = segment.construct<boost::interprocess::interprocess_mutex>(
            "mtx")();
        /* Create a RPC server and map the methods to it. */
        switch (BASKET_CONF->RPC_IMPLEMENTATION) {
#ifdef BASKET_ENABLE_RPCLIB
            case RPCLIB: {
                std::function<bool(KeyType &)> putFunc(
                    std::bind(&set<KeyType, Compare>::LocalPut, this,
                              std::placeholders::_1));
                std::function<std::pair<bool, KeyType>(KeyType &)> getFunc(
                    std::bind(&set<KeyType, Compare>::LocalGet, this,
                              std::placeholders::_1));
                std::function<std::pair<bool, MappedType>(KeyType &)> eraseFunc(
                    std::bind(&set<KeyType, Compare>::LocalErase, this,
                              std::placeholders::_1));
                std::function<std::vector<std::pair<KeyType, MappedType>>(void)>
                        getAllDataInServerFunc(std::bind(
                            &set<KeyType, Compare>::LocalGetAllDataInServer,
                            this));
                std::function<std::vector<std::pair<KeyType, MappedType>>(KeyType &)>
                        containsInServerFunc(std::bind(&set<KeyType,
                                                       Compare>::LocalContainsInServer, this,
                                                       std::placeholders::_1));

                rpc->bind(func_prefix+"_Put", putFunc);
                rpc->bind(func_prefix+"_Get", getFunc);
                rpc->bind(func_prefix+"_Erase", eraseFunc);
                rpc->bind(func_prefix+"_GetAllData", getAllDataInServerFunc);
                rpc->bind(func_prefix+"_Contains", containsInServerFunc);
                break;
            }
#endif
#ifdef BASKET_ENABLE_THALLIUM_TCP
            case THALLIUM_TCP:
#endif
#ifdef BASKET_ENABLE_THALLIUM_ROCE
            case THALLIUM_ROCE:
#endif
#if defined(BASKET_ENABLE_THALLIUM_TCP) || defined(BASKET_ENABLE_THALLIUM_ROCE)
                {

                    std::function<void(const tl::request &, KeyType &, MappedType &)> putFunc(
                        std::bind(&set<KeyType, Compare>::ThalliumLocalPut, this,
                                  std::placeholders::_1, std::placeholders::_2,
                                  std::placeholders::_3));
                    std::function<void(const tl::request &, KeyType &)> getFunc(
                        std::bind(&set<KeyType, Compare>::ThalliumLocalGet, this,
                                  std::placeholders::_1, std::placeholders::_2));
                    std::function<void(const tl::request &, KeyType &)> eraseFunc(
                        std::bind(&set<KeyType, Compare>::ThalliumLocalErase, this,
                                  std::placeholders::_1, std::placeholders::_2));
                    std::function<void(const tl::request &)>
                            getAllDataInServerFunc(std::bind(
                                &set<KeyType, Compare>::ThalliumLocalGetAllDataInServer,
                                this, std::placeholders::_1));
                    std::function<void(const tl::request &, KeyType &)>
                            containsInServerFunc(std::bind(&set<KeyType,
                                                           Compare>::ThalliumLocalContainsInServer, this,
                                                           std::placeholders::_1));

                    rpc->bind(func_prefix+"_Put", putFunc);
                    rpc->bind(func_prefix+"_Get", getFunc);
                    rpc->bind(func_prefix+"_Erase", eraseFunc);
                    rpc->bind(func_prefix+"_GetAllData", getAllDataInServerFunc);
                    rpc->bind(func_prefix+"_Contains", containsInServerFunc);
                    break;
                }
#endif
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    /* Map the clients to their respective memory pools */
    if (!is_server && server_on_node) {
        segment = boost::interprocess::managed_shared_memory(
            boost::interprocess::open_only, name.c_str());
        std::pair<MySet*,
                  boost::interprocess::managed_shared_memory::size_type> res;
        res = segment.find<MySet> (name.c_str());
        myset = res.first;
        std::pair<boost::interprocess::interprocess_mutex *,
                  boost::interprocess::managed_shared_memory::size_type> res2;
        res2 = segment.find<boost::interprocess::interprocess_mutex>("mtx");
        mutex = res2.first;
    }
    MPI_Barrier(MPI_COMM_WORLD);
}

template<typename KeyType, typename Compare>
set<KeyType, Compare>::set(std::string name_,
                                       bool is_server_,
                                       uint16_t my_server_,
                                       int num_servers_,
                                       bool server_on_node_,
                                       std::string processor_name_)
        : is_server(is_server_), my_server(my_server_), num_servers(num_servers_),
          comm_size(1), my_rank(0), memory_allocated(1024ULL * 1024ULL * 128ULL),
          name(name_), segment(), myset(), func_prefix(name_),
          server_on_node(server_on_node_) {
    AutoTrace trace = AutoTrace("basket::set", name_, is_server_, my_server_,
                                num_servers_, server_on_node_, processor_name_);
    /* Initialize MPI rank and size of world */
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    /* create per server name for shared memory. Needed if multiple servers are
       spawned on one node*/
    this->name += "_" + std::to_string(my_server);
    /* if current rank is a server */
    rpc = Singleton<RPC>::GetInstance("RPC_SERVER_LIST", is_server_, my_server_,
                                      num_servers_, server_on_node_,
                                      processor_name_);
    if (is_server) {
        /* Delete existing instance of shared memory space*/
        boost::interprocess::shared_memory_object::remove(name.c_str());
        /* allocate new shared memory space */
        segment = boost::interprocess::managed_shared_memory(
            boost::interprocess::create_only, name.c_str(), memory_allocated);
        ShmemAllocator alloc_inst(segment.get_segment_manager());
        /* Construct set in the shared memory space. */
        myset = segment.construct<MySet>(name.c_str())(Compare(), alloc_inst);
        mutex = segment.construct<boost::interprocess::interprocess_mutex>(
            "mtx")();
        /* Create a RPC server and map the methods to it. */
        switch (BASKET_CONF->RPC_IMPLEMENTATION) {
#ifdef BASKET_ENABLE_RPCLIB
            case RPCLIB: {
                std::function<bool(KeyType &, MappedType &)> putFunc(
                    std::bind(&set<KeyType, Compare>::LocalPut, this,
                              std::placeholders::_1, std::placeholders::_2));
                std::function<std::pair<bool, MappedType>(KeyType &)> getFunc(
                    std::bind(&set<KeyType, Compare>::LocalGet, this,
                              std::placeholders::_1));
                std::function<std::pair<bool, MappedType>(KeyType &)> eraseFunc(
                    std::bind(&set<KeyType, Compare>::LocalErase, this,
                              std::placeholders::_1));
                std::function<std::vector<std::pair<KeyType, MappedType>>(void)>
                        getAllDataInServerFunc(std::bind(
                            &set<KeyType, Compare>::LocalGetAllDataInServer,
                            this));
                std::function<std::vector<std::pair<KeyType, MappedType>>(KeyType &)>
                        containsInServerFunc(std::bind(&set<KeyType,
                                                       Compare>::LocalContainsInServer, this,
                                                       std::placeholders::_1));

                rpc->bind(func_prefix+"_Put", putFunc);
                rpc->bind(func_prefix+"_Get", getFunc);
                rpc->bind(func_prefix+"_Erase", eraseFunc);
                rpc->bind(func_prefix+"_GetAllData", getAllDataInServerFunc);
                rpc->bind(func_prefix+"_Contains", containsInServerFunc);
                break;
            }
#endif
#ifdef BASKET_ENABLE_THALLIUM_TCP
            case THALLIUM_TCP:
#endif
#ifdef BASKET_ENABLE_THALLIUM_ROCE
            case THALLIUM_ROCE:
#endif
#if defined(BASKET_ENABLE_THALLIUM_TCP) || defined(BASKET_ENABLE_THALLIUM_ROCE)
                {

                    std::function<void(const tl::request &, KeyType &, MappedType &)> putFunc(
                        std::bind(&set<KeyType, Compare>::ThalliumLocalPut, this,
                                  std::placeholders::_1, std::placeholders::_2,
                                  std::placeholders::_3));
                    std::function<void(const tl::request &, KeyType &)> getFunc(
                        std::bind(&set<KeyType, Compare>::ThalliumLocalGet, this,
                                  std::placeholders::_1, std::placeholders::_2));
                    std::function<void(const tl::request &, KeyType &)> eraseFunc(
                        std::bind(&set<KeyType, Compare>::ThalliumLocalErase, this,
                                  std::placeholders::_1, std::placeholders::_2));
                    std::function<void(const tl::request &)>
                            getAllDataInServerFunc(std::bind(
                                &set<KeyType, Compare>::ThalliumLocalGetAllDataInServer,
                                this, std::placeholders::_1));
                    std::function<void(const tl::request &, KeyType &)>
                            containsInServerFunc(std::bind(&set<KeyType,
                                                           Compare>::ThalliumLocalContainsInServer, this,
                                                           std::placeholders::_1));

                    rpc->bind(func_prefix+"_Put", putFunc);
                    rpc->bind(func_prefix+"_Get", getFunc);
                    rpc->bind(func_prefix+"_Erase", eraseFunc);
                    rpc->bind(func_prefix+"_GetAllData", getAllDataInServerFunc);
                    rpc->bind(func_prefix+"_Contains", containsInServerFunc);
                    break;
                }
#endif
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    /* Map the clients to their respective memory pools */
    if (!is_server && server_on_node) {
        segment = boost::interprocess::managed_shared_memory(
            boost::interprocess::open_only, name.c_str());
        std::pair<MySet*,
                  boost::interprocess::managed_shared_memory::size_type> res;
        res = segment.find<MySet> (name.c_str());
        myset = res.first;
        std::pair<boost::interprocess::interprocess_mutex *,
                  boost::interprocess::managed_shared_memory::size_type> res2;
        res2 = segment.find<boost::interprocess::interprocess_mutex>("mtx");
        mutex = res2.first;
    }
    MPI_Barrier(MPI_COMM_WORLD);
}

/**
 * Put the data into the local set.
 * @param key, the key for put
 * @param data, the value for put
 * @return bool, true if Put was successful else false.
 */
template<typename KeyType, typename Compare>
bool set<KeyType, Compare>::LocalPut(KeyType &key) {
    AutoTrace trace = AutoTrace("basket::set::Put(local)", key);
    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(*mutex);
    myset->insert_or_assign(key);
    return true;
}

/**
 * Put the data into the set. Uses key to decide the server to hash it to,
 * @param key, the key for put
 * @param data, the value for put
 * @return bool, true if Put was successful else false.
 */
template<typename KeyType, typename Compare>
bool set<KeyType, Compare>::Put(KeyType &key) {
    size_t key_hash = keyHash(key);
    uint16_t key_int = static_cast<uint16_t>(key_hash % num_servers);
    if (key_int == my_server && server_on_node) {
        return LocalPut(key);
    } else {
        AutoTrace trace = AutoTrace("basket::set::Put(remote)", key);
        return RPC_CALL_WRAPPER("_Put", key_int, bool, key);
    }
}

/**
 * Get the data in the local set.
 * @param key, key to get
 * @return return a pair of bool and Value. If bool is true then
 * data was found and is present in value part else bool is set to false
 */
template<typename KeyType, typename Compare>
bool set<KeyType, Compare>::LocalGet(KeyType &key) {
    AutoTrace trace = AutoTrace("basket::set::Get(local)", key);
    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex>
            lock(*mutex);
    typename MySet::iterator iterator = myset->find(key);
    if (iterator != myset->end()) {
        return true;
    } else {
        return false;
    }
}

/**
 * Get the data in the set. Uses key to decide the server to hash it to,
 * @param key, key to get
 * @return return a pair of bool and Value. If bool is true then
 * data was found and is present in value part else bool is set to false
 */
template<typename KeyType, typename Compare>
bool set<KeyType, Compare>::Get(KeyType &key) {
    size_t key_hash = keyHash(key);
    uint16_t key_int = key_hash % num_servers;
    if (key_int == my_server && server_on_node) {
        return LocalGet(key);
    } else {
        AutoTrace trace = AutoTrace("basket::set::Get(remote)", key);
        typedef bool ret_type;
        return RPC_CALL_WRAPPER("_Get", key_int, ret_type, key);
    }
}

template<typename KeyType, typename Compare>
bool set<KeyType, Compare>::LocalErase(KeyType &key) {
    AutoTrace trace = AutoTrace("basket::set::Erase(local)", key);
    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(*mutex);
    size_t s = myset->erase(key);
    return s > 0;
}

template<typename KeyType, typename Compare>
bool
set<KeyType, Compare>::Erase(KeyType &key) {
    size_t key_hash = keyHash(key);
    uint16_t key_int = key_hash % num_servers;
    if (key_int == my_server && server_on_node) {
        return LocalErase(key);
    } else {
        AutoTrace trace = AutoTrace("basket::set::Erase(remote)", key);
        typedef bool ret_type;
        return RPC_CALL_WRAPPER("_Erase", key_int, ret_type, key);
    }
}

/**
 * Get the data into the set. Uses key to decide the server to hash it to,
 * @param key, key to get
 * @return return a pair of bool and Value. If bool is true then data was
 * found and is present in value part else bool is set to false
 */
template<typename KeyType, typename Compare>
std::vector<KeyType>
set<KeyType, Compare>::Contains(KeyType &key) {
    AutoTrace trace = AutoTrace("basket::set::Contains", key);
    std::vector<KeyType> final_values = std::vector<KeyType>();
    auto current_server = ContainsInServer(key);
    final_values.insert(final_values.end(), current_server.begin(), current_server.end());
    for (int i = 0; i < num_servers; ++i) {
        if (i != my_server) {
            typedef std::vector<KeyType> ret_type;
            auto server = RPC_CALL_WRAPPER("_Contains", i, ret_type, key);
            final_values.insert(final_values.end(), server.begin(), server.end());
        }
    }
    return final_values;
}

template<typename KeyType, typename Compare>
std::vector<KeyType> set<KeyType, Compare>::GetAllData() {
    AutoTrace trace = AutoTrace("basket::set::GetAllData");
    std::vector<KeyType> final_values = std::vector<KeyType>();
    auto current_server = GetAllDataInServer();
    final_values.insert(final_values.end(), current_server.begin(), current_server.end());
    for (int i = 0; i < num_servers ; ++i) {
        if (i != my_server) {
            typedef std::vector<KeyType> ret_type;
            auto server = RPC_CALL_WRAPPER1("_GetAllData", i, ret_type);
            final_values.insert(final_values.end(), server.begin(), server.end());
        }
    }
    return final_values;
}

template<typename KeyType, typename Compare>
std::vector<KeyType> set<KeyType, Compare>::LocalContainsInServer(KeyType &key) {
    AutoTrace trace = AutoTrace("basket::set::ContainsInServer", key);
    std::vector<KeyType> final_values = std::vector<KeyType>();
    {
        boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(*mutex);
        typename MySet::iterator lower_bound;
        size_t size = myset->size();
        if (size == 0) {
        } else if (size == 1) {
            lower_bound = myset->begin();
            final_values.insert(final_values.end(), lower_bound->first);
        } else {
            lower_bound = myset->lower_bound(key);
            if (lower_bound == myset->end()) return final_values;
            if (lower_bound != myset->begin()) {
                --lower_bound;
                if (!key.Contains(lower_bound->first)) lower_bound++;
            }
            while (lower_bound != myset->end()) {
                if (!(key.Contains(lower_bound->first) ||
                      lower_bound->first.Contains(key))) break;
                        final_values.insert(final_values.end(), lower_bound->first);
                lower_bound++;
            }
        }
    }
    return final_values;
}

template<typename KeyType, typename Compare>
std::vector<std::pair<KeyType, MappedType>>
set<KeyType, Compare>::ContainsInServer(KeyType &key) {
    if (server_on_node) {
        return LocalContainsInServer(key);
    }
    else {
        typedef std::vector<KeyType> ret_type;
        auto my_server_i = my_server;
        return RPC_CALL_WRAPPER("_Contains", my_server_i, ret_type, key);
    }
}

template<typename KeyType, typename Compare>
std::vector<KeyType> set<KeyType, Compare>::LocalGetAllDataInServer() {
    AutoTrace trace = AutoTrace("basket::set::GetAllDataInServer", NULL);
    std::vector<KeyType> final_values = std::vector<KeyType>();
    {
        boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex>
                lock(*mutex);
        typename MySet::iterator lower_bound;
        lower_bound = myset->begin();
        while (lower_bound != myset->end()) {
            final_values.insert(final_values.end(),  lower_bound->first);
            lower_bound++;
        }
    }
    return final_values;
}

template<typename KeyType, typename Compare>
std::vector<KeyType>
set<KeyType, Compare>::GetAllDataInServer() {
    if (server_on_node) {
        return LocalGetAllDataInServer();
    }
    else {
        typedef std::vector<KeyType> ret_type;
        auto my_server_i = my_server;
        return RPC_CALL_WRAPPER1("_GetAllData", my_server_i, ret_type);
   }
}
#endif  // INCLUDE_BASKET_SET_SET_CPP_
