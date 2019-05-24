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

#ifndef INCLUDE_BASKET_HASHMAP_DISTRIBUTED_HASH_MAP_CPP_
#define INCLUDE_BASKET_HASHMAP_DISTRIBUTED_HASH_MAP_CPP_

/* Constructor to deallocate the shared memory*/
template<typename KeyType, typename MappedType>
DistributedHashMap<KeyType, MappedType>::~DistributedHashMap() {
  if (is_server) {
    boost::interprocess::shared_memory_object::remove(name.c_str());
  }
}

template<typename KeyType, typename MappedType>
DistributedHashMap<KeyType, MappedType>::DistributedHashMap(std::string name_,
                                                            bool is_server_,
                                                            uint16_t my_server_,
                                                            int num_servers_)
    : is_server(is_server_), my_server(my_server_), num_servers(num_servers_),
      comm_size(1), my_rank(0), memory_allocated(1024ULL * 1024ULL * 128ULL),
      name(name_), segment(), myHashMap(), func_prefix(name_) {
  /* Initialize MPI rank and size of world */
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  /* create per server name for shared memory. Needed if multiple servers are
     spawned on one node*/
  this->name += "_" + std::to_string(my_server);
  /* if current rank is a server */
  rpc = Singleton<RPC>::GetInstance("RPC_SERVER_LIST", is_server_, my_server_,
                                    num_servers_);
  if (is_server) {
    /* Delete existing instance of shared memory space*/
    boost::interprocess::shared_memory_object::remove(name.c_str());
    /* allocate new shared memory space */
    segment = boost::interprocess::managed_shared_memory(
        boost::interprocess::create_only, name.c_str(), memory_allocated);
    ShmemAllocator alloc_inst(segment.get_segment_manager());
    mutex = segment.construct<boost::interprocess::interprocess_mutex>(
        "mtx")();
    /* Construct Hashmap in the shared memory space. */
    myHashMap = segment.construct<MyHashMap>(name.c_str())(
        128, std::hash<KeyType>(), std::equal_to<KeyType>(),
        segment.get_allocator<ValueType>());
    /* Create a RPC server and map the methods to it. */
    std::function<bool(KeyType, MappedType)> putFunc(
        std::bind(&DistributedHashMap<KeyType, MappedType>::Put, this,
                  std::placeholders::_1, std::placeholders::_2));
    std::function<std::pair<bool, MappedType>(KeyType)> getFunc(
        std::bind(&DistributedHashMap<KeyType, MappedType>::Get, this,
                  std::placeholders::_1));
    std::function<std::pair<bool, MappedType>(KeyType)> eraseFunc(
        std::bind(&DistributedHashMap<KeyType, MappedType>::Erase, this,
                  std::placeholders::_1));
    std::function<std::vector<std::pair<KeyType, MappedType>>(void)>
        getAllDataInServerFunc(std::bind(
            &DistributedHashMap<KeyType, MappedType>::GetAllDataInServer,
            this));
    rpc->bind(func_prefix+"_Put", putFunc);
    rpc->bind(func_prefix+"_Get", getFunc);
    rpc->bind(func_prefix+"_Erase", eraseFunc);
    rpc->bind(func_prefix+"_GetAllData", getAllDataInServerFunc);
    // srv->suppress_exceptions(true);
  }
  /* Make clients wait untill all servers reach here*/
  MPI_Barrier(MPI_COMM_WORLD);
  /* Map the clients to their respective memory pools */
  if (!is_server) {
    segment = boost::interprocess::managed_shared_memory(
        boost::interprocess::open_only, name.c_str());
    std::pair<MyHashMap *,
              boost::interprocess::managed_shared_memory::size_type> res;
    res = segment.find<MyHashMap>(name.c_str());
    myHashMap = res.first;
    size_t size = myHashMap->size();
    std::pair<boost::interprocess::interprocess_mutex *,
              boost::interprocess::managed_shared_memory::size_type> res2;
    res2 = segment.find<boost::interprocess::interprocess_mutex>("mtx");
    mutex = res2.first;
  }
  MPI_Barrier(MPI_COMM_WORLD);
}

/**
 * Put the data into the hashmap. Uses key to decide the server to hash it to,
 * @param key, the key for put
 * @param data, the value for put
 * @return bool, true if Put was successful else false.
 */
template<typename KeyType, typename MappedType>
bool DistributedHashMap<KeyType, MappedType>::Put(KeyType key,
                                                  MappedType data) {
  size_t key_hash = keyHash(key);
  uint16_t key_int = static_cast<uint16_t>(key_hash % num_servers);
  if (key_int == my_server) {
    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex>
        lock(*mutex);
    typename MyHashMap::iterator iterator = myHashMap->find(key);
    if (iterator != myHashMap->end()) {
      myHashMap->erase(key);
    }
    myHashMap->insert(std::pair<KeyType, MappedType>(key, data));
    return true;
  } else {
    return rpc->call(key_int, func_prefix+"_Put", key,
                     data).template as<bool>();
  }
}

/**
 * Get the data into the hashmap. Uses key to decide the server to hash it to,
 * @param key, key to get
 * @return return a pair of bool and Value. If bool is true then data was
 * found and is present in value part else bool is set to false
 */
template<typename KeyType, typename MappedType>
std::pair<bool, MappedType>
DistributedHashMap<KeyType, MappedType>::Get(KeyType key) {
  size_t key_hash = keyHash(key);
  uint16_t key_int = static_cast<uint16_t>(key_hash % num_servers);
  if (key_int == my_server) {
    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex>
        lock(*mutex);
    typename MyHashMap::iterator iterator = myHashMap->find(key);
    if (iterator != myHashMap->end()) {
      return std::pair<bool, MappedType>(true, iterator->second);
    } else {
      return std::pair<bool, MappedType>(false, MappedType());
    }
  } else {
    return rpc->call(key_int, func_prefix+"_Get",
                     key).template as<std::pair<bool, MappedType>>();
  }
}
template<typename KeyType, typename MappedType>
std::pair<bool, MappedType>
DistributedHashMap<KeyType, MappedType>::Erase(KeyType key) {
  size_t key_hash = keyHash(key);
  uint16_t key_int = static_cast<uint16_t>(key_hash % num_servers);
  if (key_int == my_server) {
    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex>
        lock(*mutex);
    size_t s = myHashMap->erase(key);
    return std::pair<bool, MappedType>(s > 0, MappedType());
  } else {
    return rpc->call(key_int, func_prefix+"_Erase",
                     key).template as<std::pair<bool, MappedType>>();
  }
}
template<typename KeyType, typename MappedType>
std::vector<std::pair<KeyType, MappedType>>
DistributedHashMap<KeyType, MappedType>::GetAllData() {
  std::vector<std::pair<KeyType, MappedType>> final_values =
      std::vector<std::pair<KeyType, MappedType>>();
  auto current_server = GetAllDataInServer();
  final_values.insert(final_values.end(), current_server.begin(),
                      current_server.end());
  for (int i = 0; i < num_servers; ++i) {
    if (i != my_server) {
      auto server = rpc->call(
          i, func_prefix+"_GetAllData").template
          as<std::vector<std::pair<KeyType, MappedType>>>();
      final_values.insert(final_values.end(), server.begin(), server.end());
    }
  }
  return final_values;
}
template<typename KeyType, typename MappedType>
std::vector<std::pair<KeyType, MappedType>>
DistributedHashMap<KeyType, MappedType>::GetAllDataInServer() {
  std::vector<std::pair<KeyType, MappedType>> final_values =
      std::vector<std::pair<KeyType, MappedType>>();
  {
    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex>
        lock(*mutex);
    typename MyHashMap::iterator lower_bound;
    if (myHashMap->size() > 0) {
      lower_bound = myHashMap->begin();
      while (lower_bound != myHashMap->end()) {
        final_values.push_back(std::pair<KeyType, MappedType>(
            lower_bound->first, lower_bound->second));
        lower_bound++;
      }
    }
  }
  return final_values;
}
#endif  // INCLUDE_BASKET_HASHMAP_DISTRIBUTED_HASH_MAP_CPP_
