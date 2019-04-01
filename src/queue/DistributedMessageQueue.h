//
// Created by HariharanDevarajan on 2/1/2019.
//

#ifndef HERMES_PROJECT_DISTRIBUTEDMESSAGEQUEUE_H
#define HERMES_PROJECT_DISTRIBUTEDMESSAGEQUEUE_H

/**
 * Include Headers
 */
/** Standard C++ Headers**/
#include <iostream>
#include <functional>
#include <utility>
/** MPI Headers**/
#include <mpi.h>
/** RPC Lib Headers**/
#include "../../../../external/rpclib/include/rpc/server.h"
#include "../../../../external/rpclib/include/rpc/client.h"
/** Boost Headers **/
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/deque.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/algorithm/string.hpp>
#include <src/common/constants.h>
#include <src/common/distributed_ds/communication/rpc_lib.h>
#include <src/common/singleton.h>

/** Namespaces Uses **/
namespace bip=boost::interprocess;

/**
 * This is a Distributed HashMap Class. It uses shared memory + RPC + MPI to achieve the data structure.
 *
 * @tparam MappedType, the value of the HashMap
 */
template<typename MappedType>
class DistributedMessageQueue {
private:
    /** Class Typedefs for ease of use **/
    typedef bip::allocator<MappedType, bip::managed_shared_memory::segment_manager> ShmemAllocator;
    typedef boost::interprocess::deque<MappedType, ShmemAllocator> Queue;

    /** Class attributes**/
    int comm_size, my_rank,num_servers;
    uint16_t  my_server;
    std::shared_ptr<RPC> rpc;
    really_long memory_allocated;
    bool is_server;
    boost::interprocess::managed_shared_memory segment;
    std::string name,func_prefix;
    Queue *queue;
    boost::interprocess::interprocess_mutex* mutex;
public:
    /* Constructor to deallocate the shared memory*/
    ~DistributedMessageQueue(){
        if(is_server) bip::shared_memory_object::remove(name.c_str());
    }

    explicit DistributedMessageQueue(std::string name_,
                                     bool is_server_,
                                     uint16_t my_server_,
                                     int num_servers_)
            : is_server(is_server_), my_server(my_server_), num_servers(num_servers_),
              comm_size(1), my_rank(0), memory_allocated(1024ULL * 1024ULL * 128ULL), name(name_), segment(),
              queue(),func_prefix(name_){
        AutoTrace trace = AutoTrace("DistributedMessageQueue(local)",name_,is_server_,my_server_,num_servers_);
        /* Initialize MPI rank and size of world */
        MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
        MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
        /* create per server name for shared memory. Needed if multiple servers are spawned on one node*/
        this->name += "_" + std::to_string(my_server);
        rpc=Singleton<RPC>::GetInstance("RPC_SERVER_LIST",is_server_,my_server_,num_servers_);
        if (is_server) {
            /* Delete existing instance of shared memory space*/
            bip::shared_memory_object::remove(name.c_str());
            /* allocate new shared memory space */
            segment=bip::managed_shared_memory(bip::create_only,name.c_str(),memory_allocated);
            ShmemAllocator alloc_inst (segment.get_segment_manager());
            /* Construct Hashmap in the shared memory space. */
            queue = segment.construct<Queue>("Queue")(alloc_inst);
            mutex = segment.construct<bip::interprocess_mutex>("mtx")();
            /* Create a RPC server and map the methods to it. */
            std::function<bool(MappedType,uint16_t)> pushFunc(std::bind(&DistributedMessageQueue<MappedType>::Push, this, std::placeholders::_1, std::placeholders::_2));
            std::function<std::pair<bool,MappedType>(uint16_t)> popFunc(std::bind(&DistributedMessageQueue::Pop, this, std::placeholders::_1));
            std::function<size_t(uint16_t)> sizeFunc(std::bind(&DistributedMessageQueue::Size, this, std::placeholders::_1));
            std::function<bool(uint16_t)> waitForElementFunc(std::bind(&DistributedMessageQueue::WaitForElement, this, std::placeholders::_1));
            rpc->bind(func_prefix+"_Push", pushFunc);
            rpc->bind(func_prefix+"_Pop", popFunc);
            rpc->bind(func_prefix+"_WaitForElement", waitForElementFunc);
            rpc->bind(func_prefix+"_Size", sizeFunc);
        }
        /* Make clients wait untill all servers reach here*/
        MPI_Barrier(MPI_COMM_WORLD);
        /* Map the clients to their respective memory pools */
        if(!is_server){
            segment=bip::managed_shared_memory(bip::open_only,name.c_str());
            std::pair<Queue*, bip::managed_shared_memory::size_type> res;
            res = segment.find<Queue> ("Queue");
            queue = res.first;
            std::pair<bip::interprocess_mutex *, bip::managed_shared_memory::size_type> res2;
            res2 = segment.find<bip::interprocess_mutex>("mtx");
            mutex = res2.first;
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
    /**
     * Push the data into the queue. Uses key to decide the server to hash it to,
     * @param key, the key for put
     * @param data, the value for put
     * @return bool, true if Put was successful else false.
     */
    bool Push(MappedType data, uint16_t key_int){
        if(key_int == my_server){
            AutoTrace trace = AutoTrace("DistributedMessageQueue::Push(local)",data, key_int);
            bip::scoped_lock<bip::interprocess_mutex> lock(*mutex);
            queue->push_back(std::move(data));
            return true;
        }else{
            AutoTrace trace = AutoTrace("DistributedMessageQueue::Push(remote)",data, key_int);
            return rpc->call(key_int,func_prefix+"_Push", data).template as<bool>();
        }
    }
    /**
     * Get the data from the queue. Uses key_int to decide the server to hash it to,
     * @param key_int, key_int to know which server
     * @return return a pair of bool and Value. If bool is true then data was found
     *          and is present in value part else bool is set to false
     */
    std::pair<bool,MappedType> Pop(uint16_t key_int) {
        if (key_int == my_server) {
            AutoTrace trace = AutoTrace("DistributedMessageQueue::Pop(local)", key_int);
            bip::scoped_lock<bip::interprocess_mutex> lock(*mutex);
            if(queue->size()>0){
                MappedType value = queue->front();
                queue->pop_front();
                return std::pair<bool,MappedType>(true,value);
            }
            return std::pair<bool,MappedType>(false,MappedType());;
        } else {
            AutoTrace trace = AutoTrace("DistributedMessageQueue::Pop(remote)", key_int);
            return rpc->call(key_int,func_prefix+"_Pop").template as<std::pair<bool, MappedType>>();
        }
    }

    bool WaitForElement(uint16_t key_int) {
        if (key_int == my_server) {
            AutoTrace trace = AutoTrace("DistributedMessageQueue::WaitForElement(local)", key_int);
            int count=0;
            while(queue->size()==0){
                usleep(10);
                if(count==0) printf("Server %d, No Events in Queue\n",key_int);
                count++;
            }
            return true;
        } else {
            AutoTrace trace = AutoTrace("DistributedMessageQueue::WaitForElement(remote)", key_int);
            return rpc->call(key_int,func_prefix+"_WaitForElement").template as<bool>();
        }
    }

    /**
     * Get the size of the queue. Uses key_int to decide the server to hash it to,
     * @param key_int, key_int to know which server
     * @return return a size of the queue
     */
    size_t Size(uint16_t key_int) {
        if (key_int == my_server) {
            AutoTrace trace = AutoTrace("DistributedMessageQueue::Size(local)", key_int);
            bip::scoped_lock<bip::interprocess_mutex> lock(*mutex);
            size_t value= queue->size();
            return value;
        } else {
            AutoTrace trace = AutoTrace("DistributedMessageQueue::Size(remote)", key_int);
            return rpc->call(key_int,func_prefix+"_Size").template as<size_t>();;
        }
    }

};

#endif //HERMES_PROJECT_DISTRIBUTEDMESSAGEQUEUE_H
