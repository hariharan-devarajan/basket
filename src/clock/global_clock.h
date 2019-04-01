//
// Created by hariharan on 2/25/19.
//

#ifndef HERMES_PROJECT_GLOBAL_CLOCK_H
#define HERMES_PROJECT_GLOBAL_CLOCK_H

#include <stdint-gcc.h>
#include <mpi.h>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <src/common/singleton.h>
#include <src/common/distributed_ds/communication/rpc_lib.h>
#include <src/common/debug.h>

namespace bip=boost::interprocess;
typedef unsigned long long int really_long;
class GlobalClock{
private:
    typedef std::chrono::high_resolution_clock::time_point chrono_time;
    chrono_time *start;
    bool is_server;
    bip::interprocess_mutex* mutex;
    really_long memory_allocated;
    int my_rank,comm_size,num_servers;
    uint16_t my_server;
    bip::managed_shared_memory segment;
    std::string name,func_prefix;
    std::shared_ptr<RPC> rpc;
public:
    ~GlobalClock(){
        AutoTrace trace = AutoTrace("~GlobalClock");
        if(is_server) bip::shared_memory_object::remove(name.c_str());
    }
    GlobalClock(std::string name_,
                     bool is_server_,
                     uint16_t my_server_,
                     int num_servers_): is_server(is_server_), my_server(my_server_), num_servers(num_servers_),
              comm_size(1), my_rank(0), memory_allocated(1024ULL), name(name_), segment(),func_prefix(name_){
        AutoTrace trace = AutoTrace("GlobalClock",name_,is_server_,my_server_,num_servers_);
        MPI_Comm_size(MPI_COMM_WORLD,&comm_size);
        MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);
        name=name+"_"+std::to_string(my_server);
        rpc=Singleton<RPC>::GetInstance("RPC_SERVER_LIST",is_server_,my_server_,num_servers_);
        if(is_server){
            std::function<HTime(void)> getTimeFunction(std::bind(&GlobalClock::GetTime, this));
            rpc->bind(func_prefix+"_GetTime",getTimeFunction);
            bip::shared_memory_object::remove(name.c_str());
            segment=bip::managed_shared_memory(bip::create_only, name.c_str(), 65536);
            start = segment.construct<chrono_time>("Time")(std::chrono::high_resolution_clock::now());
            mutex = segment.construct<boost::interprocess::interprocess_mutex>("mtx")();
        }
        MPI_Barrier(MPI_COMM_WORLD);

        if(!is_server){
            segment=bip::managed_shared_memory(bip::open_only,name.c_str());
            std::pair<chrono_time*,bip::managed_shared_memory::size_type> res;
            res = segment.find<chrono_time> ("Time");
            start = res.first;
            std::pair<bip::interprocess_mutex *, bip::managed_shared_memory::size_type> res2;
            res2 = segment.find<bip::interprocess_mutex>("mtx");
            mutex = res2.first;
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }

    HTime GetTime(){
        AutoTrace trace = AutoTrace("GlobalClock::GetTime");
        boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(*mutex);
        auto t2 = std::chrono::high_resolution_clock::now();
        auto t =  std::chrono::duration_cast<std::chrono::microseconds>(
                t2 - *start).count();
        return t;
    }
    HTime GetTimeServer(uint16_t server){
        AutoTrace trace = AutoTrace("GlobalClock::GetTimeServer",server);
        if(my_server==server){
            boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(*mutex);
            auto t2 = std::chrono::high_resolution_clock::now();
            auto t =  std::chrono::duration_cast<std::chrono::microseconds>(t2 - *start).count();
            return t;
        }return rpc->call(server,func_prefix+"GetTime").as<HTime>();

    }


};
#endif //HERMES_PROJECT_GLOBAL_CLOCK_H
