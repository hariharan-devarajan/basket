//
// Created by hariharan on 2/19/19.
//

#ifndef HERMES_PROJECT_GLOBAL_SEQUENCE_H
#define HERMES_PROJECT_GLOBAL_SEQUENCE_H

#include <stdint-gcc.h>
#include <mpi.h>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <src/common/distributed_ds/communication/rpc_lib.h>
#include <src/common/singleton.h>

namespace bip=boost::interprocess;

class GlobalSequence{
private:
    uint64_t* value;
    bool is_server;
    bip::interprocess_mutex* mutex;
    int my_rank,comm_size,num_servers;
    uint16_t my_server;
    really_long memory_allocated;
    bip::managed_shared_memory segment;
    std::string name,func_prefix;
    std::shared_ptr<RPC> rpc;
public:
    ~GlobalSequence(){
        if(is_server) bip::shared_memory_object::remove(name.c_str());
    }
    GlobalSequence(std::string name_,
                   bool is_server_,
                   uint16_t my_server_,
                   int num_servers_)
            : is_server(is_server_), my_server(my_server_), num_servers(num_servers_),
              comm_size(1), my_rank(0), memory_allocated(1024ULL * 1024ULL * 128ULL), name(name_), segment(),func_prefix(name_){
        AutoTrace trace = AutoTrace("GlobalSequence", name_,is_server_,my_server_,num_servers_);
        MPI_Comm_size(MPI_COMM_WORLD,&comm_size);
        MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);
        name=name+"_"+std::to_string(my_server);
        rpc=Singleton<RPC>::GetInstance("RPC_SERVER_LIST",is_server_,my_server_,num_servers_);
        if(is_server){
            std::function<uint64_t(void)> getNextSequence(std::bind(&GlobalSequence::GetNextSequence, this));
            rpc->bind(func_prefix+"_GetNextSequence",getNextSequence);
            bip::shared_memory_object::remove(name.c_str());
            segment=bip::managed_shared_memory(bip::create_only, name.c_str(), 65536);
            value = segment.construct<uint64_t>(name.c_str())(0);
            mutex = segment.construct<boost::interprocess::interprocess_mutex>("mtx")();
        }
        MPI_Barrier(MPI_COMM_WORLD);
        if(!is_server){
            segment=bip::managed_shared_memory(bip::open_only,name.c_str());
            std::pair<uint64_t*,bip::managed_shared_memory::size_type> res;
            res = segment.find<uint64_t> (name.c_str());
            value = res.first;
            std::pair<bip::interprocess_mutex *, bip::managed_shared_memory::size_type> res2;
            res2 = segment.find<bip::interprocess_mutex>("mtx");
            mutex = res2.first;
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }

    uint64_t GetNextSequence(){
        boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(*mutex);
        return ++*value;
    }
    uint64_t GetNextSequenceServer(uint16_t server){
        if(my_server==server){
            boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(*mutex);
            return ++*value;
        }return
        rpc->call(server,func_prefix+"_GetNextSequence").as<uint64_t>();

    }


};

#endif //HERMES_PROJECT_GLOBAL_SEQUENCE_H
