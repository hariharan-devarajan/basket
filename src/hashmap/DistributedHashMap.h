//
// Created by HariharanDevarajan on 2/1/2019.
//

#ifndef HERMES_PROJECT_DISTRIBUTEDHASHMAP_H
#define HERMES_PROJECT_DISTRIBUTEDHASHMAP_H

/**
 * Include Headers
 */
/** Standard C++ Headers**/
#include <iostream>
#include <functional>
#include <utility>
#include <format.h>
#include <stdexcept>
/** MPI Headers**/
#include <mpi.h>
/** RPC Lib Headers**/
#include <rpc/server.h>
#include <rpc/client.h>
/** Boost Headers **/
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/unordered_map.hpp>
//#include <unordered_map>
#include <functional>
#include <boost/functional/hash.hpp>
#include <boost/algorithm/string.hpp>
#include <src/common/constants.h>
#include <rpc/rpc_error.h>
#include <src/common/distributed_ds/communication/rpc_lib.h>
#include <src/common/singleton.h>
#include <src/common/macros.h>

/** Namespaces Uses **/

/** Global Typedefs **/
typedef unsigned long long int really_long;



/**
 * This is a Distributed HashMap Class. It uses shared memory + RPC + MPI to achieve the data structure.
 *
 * @tparam MappedType, the value of the HashMap
 */
    template<typename KeyType, typename MappedType>
    class DistributedHashMap {
    private:
        std::hash<KeyType> keyHash;
        /** Class Typedefs for ease of use **/
        typedef std::pair<const KeyType, MappedType> ValueType;
        typedef boost::interprocess::allocator<ValueType, boost::interprocess::managed_shared_memory::segment_manager> ShmemAllocator;
        typedef boost::unordered_map<KeyType, MappedType, std::hash<KeyType>, std::equal_to<KeyType>, ShmemAllocator> MyHashMap;
        /** Class attributes**/
        int comm_size, my_rank,num_servers;
        uint16_t  my_server;
        std::shared_ptr<RPC> rpc;
        really_long memory_allocated;
        bool is_server;
        boost::interprocess::managed_shared_memory segment;
        std::string name,func_prefix;
        MyHashMap *myHashMap;
        boost::interprocess::interprocess_mutex* mutex;
    public:

        /* Constructor to deallocate the shared memory*/
        ~DistributedHashMap() {
            if(is_server){
                boost::interprocess::shared_memory_object::remove(name.c_str());
            }
        }

        explicit DistributedHashMap(std::string name_,
                                    bool is_server_,
                                    uint16_t my_server_,
                                    int num_servers_)
                : is_server(is_server_), my_server(my_server_), num_servers(num_servers_),
                  comm_size(1), my_rank(0), memory_allocated(1024ULL * 1024ULL * 128ULL), name(name_), segment(),
                  myHashMap(),func_prefix(name_) {

            /* Initialize MPI rank and size of world */
            MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
            MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
            /* create per server name for shared memory. Needed if multiple servers are spawned on one node*/
            this->name += "_" + std::to_string(my_server);
            /* if current rank is a server */
            rpc=Singleton<RPC>::GetInstance("RPC_SERVER_LIST",is_server_,my_server_,num_servers_);
            if (is_server) {
                /* Delete existing instance of shared memory space*/
                boost::interprocess::shared_memory_object::remove(name.c_str());
                /* allocate new shared memory space */
                segment = boost::interprocess::managed_shared_memory(boost::interprocess::create_only, name.c_str(), memory_allocated);
                ShmemAllocator alloc_inst(segment.get_segment_manager());
                mutex = segment.construct<boost::interprocess::interprocess_mutex>("mtx")();
                /* Construct Hashmap in the shared memory space. */
                myHashMap = segment.construct<MyHashMap>(name.c_str())(128, std::hash<KeyType>(), std::equal_to<KeyType>(), segment.get_allocator<ValueType>());
                /* Create a RPC server and map the methods to it. */
                std::function<bool(KeyType, MappedType)> putFunc(
                        std::bind(&DistributedHashMap<KeyType, MappedType>::Put, this, std::placeholders::_1,
                                  std::placeholders::_2));
                std::function<std::pair<bool, MappedType>(KeyType)> getFunc(
                        std::bind(&DistributedHashMap<KeyType, MappedType>::Get, this, std::placeholders::_1));
                std::function<std::pair<bool,MappedType>(KeyType)> eraseFunc(std::bind(&DistributedHashMap<KeyType,MappedType>::Erase, this, std::placeholders::_1 ));
                std::function<std::vector<std::pair<KeyType,MappedType>>(void)> getAllDataInServerFunc(std::bind(&DistributedHashMap<KeyType,MappedType>::GetAllDataInServer, this ));
                rpc->bind(func_prefix+"_Put", putFunc);
                rpc->bind(func_prefix+"_Get", getFunc);
                rpc->bind(func_prefix+"_Erase", eraseFunc);
                rpc->bind(func_prefix+"_GetAllData", getAllDataInServerFunc);
                //srv->suppress_exceptions(true);
            }
            /* Make clients wait untill all servers reach here*/
            MPI_Barrier(MPI_COMM_WORLD);
            /* Map the clients to their respective memory pools */
            if (!is_server) {
                segment = boost::interprocess::managed_shared_memory(boost::interprocess::open_only, name.c_str());
                std::pair<MyHashMap *, boost::interprocess::managed_shared_memory::size_type> res;
                res = segment.find<MyHashMap>(name.c_str());
                myHashMap = res.first;
                size_t size=myHashMap->size();
                std::pair<boost::interprocess::interprocess_mutex *, boost::interprocess::managed_shared_memory::size_type> res2;
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
        bool Put(KeyType key, MappedType data) {

            size_t key_hash = keyHash(key);
            uint16_t key_int = static_cast<uint16_t>(key_hash % num_servers);
            if (key_int == my_server) {
                boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(*mutex);
                typename MyHashMap::iterator iterator = myHashMap->find(key);
                if (iterator != myHashMap->end()) {
                    myHashMap->erase(key);
                }
                myHashMap->insert(std::pair<KeyType,MappedType>(key, data));
                return true;
            } else {
                return rpc->call(key_int,func_prefix+"_Put",key, data).template as<bool>();
            }
        }

        /**
         * Get the data into the hashmap. Uses key to decide the server to hash it to,
         * @param key, key to get
         * @return return a pair of bool and Value. If bool is true then data was found
         *          and is present in value part else bool is set to false
         */
        std::pair<bool, MappedType> Get(KeyType key) {
            size_t key_hash = keyHash(key);
            uint16_t key_int = static_cast<uint16_t>(key_hash % num_servers);
            if (key_int == my_server) {
                boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(*mutex);
                typename MyHashMap::iterator iterator = myHashMap->find(key);
                if (iterator != myHashMap->end()) {
                    return std::pair<bool, MappedType>(true, iterator->second);
                } else {
                    return std::pair<bool, MappedType>(false, MappedType());
                }
            } else {
                return rpc->call(key_int,func_prefix+"_Get",key).template as<std::pair<bool, MappedType>>();
            }
        }
        std::pair<bool,MappedType> Erase(KeyType key) {
            size_t key_hash = keyHash(key);
            uint16_t key_int = static_cast<uint16_t>(key_hash % num_servers);
            if (key_int == my_server) {
                boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(*mutex);
                size_t s = myHashMap->erase(key);
                return std::pair<bool, MappedType>(s>0, MappedType());
            } else {
                return rpc->call(key_int,func_prefix+"_Erase",key).template as<std::pair<bool, MappedType>>();
            }
        }
        std::vector<std::pair<KeyType,MappedType>> GetAllData() {
            std::vector<std::pair<KeyType,MappedType>> final_values=std::vector<std::pair<KeyType,MappedType>>();
            auto current_server=GetAllDataInServer();
            final_values.insert(final_values.end(),current_server.begin(),current_server.end());
            for(int i=0;i<num_servers ;++i){
                if(i!=my_server){
                    auto server=rpc->call(i,func_prefix+"_GetAllData").template as<std::vector<std::pair<KeyType,MappedType>>>();
                    final_values.insert(final_values.end(),server.begin(),server.end());
                }

            }
            return final_values;

        }
        std::vector<std::pair<KeyType,MappedType>> GetAllDataInServer() {
            std::vector<std::pair<KeyType, MappedType>> final_values = std::vector<std::pair<KeyType, MappedType>>();
            {
                boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(*mutex);
                typename MyHashMap::iterator lower_bound;
                if(myHashMap->size() > 0){
                    lower_bound = myHashMap->begin();
                    while (lower_bound != myHashMap->end()){
                        final_values.push_back(std::pair<KeyType,MappedType>(lower_bound->first, lower_bound->second));
                        lower_bound++;
                    }
                }
            }
            return final_values;
        }
    };


#endif //HERMES_PROJECT_DISTRIBUTEDHASHMAP_H
