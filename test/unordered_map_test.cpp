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

// #include <redox.hpp>

#include <functional>
#include <basket/unordered_map/unordered_map.h>
#include <utility>
#include <mpi.h>
#include "util.h"
#include <iostream>
#include <signal.h>
#include <execinfo.h>
#include <chrono>
#include <rpc/client.h>
#include <thallium.hpp>
#include <basket/common/data_structures.h>
#include <thallium/serialization/stl/string.hpp>

#include <sys/types.h>
#include <unistd.h>

#define MB 1024*1024
#define TESTSIZE 64000000


// using namespace redox;

// arguments: ranks_per_server, num_request, debug
int main (int argc,char* argv[])
{
    SetSignal();
    MPI_Init(&argc,&argv);
    int comm_size,my_rank;
    MPI_Comm_size(MPI_COMM_WORLD,&comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);
    int ranks_per_server=comm_size,num_request=10000,num_of_int=1;
    bool debug=false;
    bool server_on_node=false;
    if(argc > 1)    ranks_per_server = atoi(argv[1]);
    if(argc > 2)    num_request = atoi(argv[2]);
    if(argc > 3)    server_on_node = (bool)atoi(argv[3]);
    if(argc > 4)    num_of_int = atoi(argv[4]);
    if(argc > 5)    debug = (bool)atoi(argv[5]);

    if(comm_size/ranks_per_server < 2){
        perror("comm_size/ranks_per_server should be atleast 2 for this test\n");
        exit(-1);
    }

    int len;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    MPI_Get_processor_name(processor_name, &len);
    if (debug) {
        printf("%s/%d: %d\n", processor_name, my_rank, getpid());
    }
    if(debug && my_rank==0){
        printf("%d ready for attach\n", comm_size);
        fflush(stdout);
        getchar();
    }
    MPI_Barrier(MPI_COMM_WORLD);

    // every node should have 1 server
    // bool is_server=(my_rank+1) % ranks_per_server == 0;
    bool is_server = (my_rank == 1) ? true : false;
    
    // int num_servers=comm_size/ranks_per_server;
    // int my_server = my_rank / ranks_per_server;

    int num_servers = 1;
    int my_server = 0;

    std::string proc_name = std::string(processor_name);
    // int split_loc = proc_name.find('.');
    // std::string node_name = proc_name.substr(0, split_loc);
    // std::string extra_info = proc_name.substr(split_loc+1, string::npos);
    // proc_name = node_name + "-40g." + extra_info;
    
    // int my_values = 0;

    // MappedType my_values;
    std::string my_values = "hello world";
    my_values.resize(TESTSIZE, 'c');
    size_t size_my_values = my_values.size();
    if(debug){
        printf("node %s, rank %d, is_server %d, my_server %d, num_servers %d\n",proc_name.c_str(),my_rank,is_server,my_server,num_servers);
    }
    if (my_rank == 0) {
        printf("Test with size %d\n", num_request * size_my_values);
    }

    {
        basket::unordered_map<KeyType, std::string> map("test", is_server, my_server, num_servers,
                                                       server_on_node || is_server, proc_name);

        // std::unordered_map<KeyType, std::string> std_map;
        typedef std::pair<const KeyType, std::string> ValueType;
        typedef boost::interprocess::allocator<ValueType, boost::interprocess::
                                               managed_shared_memory::segment_manager>
                ShmemAllocator;

        boost::unordered::unordered_map<KeyType, std::string, std::hash<KeyType>,
                                        std::equal_to<KeyType>> std_map;                                        
        // basket::unordered_map<std::string, std::string> map("test", is_server, my_server, num_servers,
        //                                                     server_on_node || is_server, proc_name);

        MPI_Comm client_comm;
        int client_comm_size;
        MPI_Comm_split(MPI_COMM_WORLD, is_server, my_rank, &client_comm);
        MPI_Comm_size(client_comm, &client_comm_size);
        if (!is_server) {
            size_t size_of_put = sizeof(KeyType) + size_my_values + sizeof(bool);
            size_t size_of_get = 2 * sizeof(KeyType) + size_my_values;

            double local_map_bandwidth;
            double local_get_map_bw;
            Timer local_put_map_timer = Timer();
            /*Local map test*/
            for (int i = 0; i < num_request; i++) {
                usleep(100);
                size_t val = my_server + i * num_servers * my_rank;
                local_put_map_timer.resumeTime();
                if (map.Put(KeyType(val), my_values)) {
                    printf("success\n");
                }
                // map.Put(std::to_string(val), my_values);
                local_put_map_timer.pauseTime();
            }
            local_map_bandwidth = (num_request * size_of_put * 1000.0) / (MB * local_put_map_timer.getElapsedTime()*1.0);

            Timer local_get_map_timer = Timer();
            /*Local map test*/
            for (int i = 0; i < num_request; i++) {
                usleep(100);
                size_t val = my_server + i * num_servers * my_rank;
                local_get_map_timer.resumeTime();
                auto ret = map.Get(KeyType(val));
                // auto ret = map.Get(std::to_string(val));
                local_get_map_timer.pauseTime();
            }
            local_get_map_bw = (num_request * size_of_get * 1000.0) / (MB * local_get_map_timer.getElapsedTime()*1.0);
            double local_get_bw_sum, local_put_bw_sum;
            if (client_comm_size == 1) {
                local_put_bw_sum = local_map_bandwidth;
                local_get_bw_sum = local_get_map_bw;
            }
            else {
                MPI_Reduce(&local_map_bandwidth, &local_put_bw_sum, 1,
                           MPI_DOUBLE, MPI_SUM, 0, client_comm);
                MPI_Reduce(&local_get_map_bw, &local_get_bw_sum, 1,
                           MPI_DOUBLE, MPI_SUM, 0, client_comm);
            }
            if (my_rank == 0) {
                printf("local bw:\t put %f MB/s,\t get %f MB/s\n",
                       local_put_bw_sum / client_comm_size,
                       local_get_bw_sum / client_comm_size);
            }

            // MPI_Barrier(client_comm);
            // Redox rdx_map;
            // if (!rdx_map.connect("ares-comp-02", 6379)) return 1;

            // double local_rdx_map_bandwidth;
            // double local_rdx_get_map_bw;
            // Timer local_rdx_put_map_timer = Timer();

            // /*Local rdx test*/
            // for (int i = 0; i < num_request; i++) {
            //     size_t val = my_server + i * num_servers * my_rank;
            //     local_rdx_put_map_timer.resumeTime();
            //     // std_map.emplace(KeyType(val), my_values);
            //     rdx_map.set(std::to_string(val), my_values);
            //     local_rdx_put_map_timer.pauseTime();
            // }
            // local_rdx_map_bandwidth = (num_request * size_of_put * 1000.0) / (MB * local_rdx_put_map_timer.getElapsedTime()*1.0);
            // Timer local_rdx_get_map_timer = Timer();
            // /*Local rdx test*/
            // for (int i = 0; i < num_request; i++) {
            //     size_t val = my_server + i * num_servers * my_rank;
            //     local_rdx_get_map_timer.resumeTime();
            //     auto ret = rdx_map.get(std::to_string(val));
            //     // auto ret = std_map.at(KeyType(val));
            //     local_rdx_get_map_timer.pauseTime();
            // }
            // local_rdx_get_map_bw = (num_request * size_of_get * 1000.0) / (MB * local_rdx_get_map_timer.getElapsedTime()*1.0);
            // double local_rdx_get_bw_sum, local_rdx_put_bw_sum;
            // if (client_comm_size == 1) {
            //     local_rdx_put_bw_sum = local_rdx_map_bandwidth;
            //     local_rdx_get_bw_sum = local_rdx_get_map_bw;
            // }
            // else {
            //     MPI_Reduce(&local_rdx_map_bandwidth, &local_rdx_put_bw_sum, 1,
            //                MPI_DOUBLE, MPI_SUM, 0, client_comm);
            //     MPI_Reduce(&local_rdx_get_map_bw, &local_rdx_get_bw_sum, 1,
            //                MPI_DOUBLE, MPI_SUM, 0, client_comm);
            // }
            // rdx_map.disconnect();
            // if (my_rank == 0) {
            //     printf("local rdx bw:\t put %f MB/s,\t get %f MB/s\n",
            //            local_rdx_put_bw_sum / client_comm_size,
            //            local_rdx_get_bw_sum / client_comm_size);
            // }

            MPI_Barrier(client_comm);

            double local_std_map_bandwidth;
            double local_std_get_map_bw;
            Timer local_std_put_map_timer = Timer();

            /*Local std::um test*/
            for (int i = 0; i < num_request; i++) {
                size_t val = my_server + i * num_servers * my_rank;
                local_std_put_map_timer.resumeTime();
                // std_map.emplace(KeyType(val), my_values);
                auto retval = std_map.insert_or_assign(KeyType(val), my_values);
                if (retval.second) {
                    printf("std::um success\n");
                }
                local_std_put_map_timer.pauseTime();
            }
            local_std_map_bandwidth = (num_request * size_of_put * 1000.0) / (MB * local_std_put_map_timer.getElapsedTime()*1.0);
            Timer local_std_get_map_timer = Timer();
            /*Local std::um test*/
            for (int i = 0; i < num_request; i++) {
                size_t val = my_server + i * num_servers * my_rank;
                local_std_get_map_timer.resumeTime();
                auto iterator = std_map.find(KeyType(val));
                auto ret = iterator->second;
                // auto ret = std_map.at(KeyType(val));
                local_std_get_map_timer.pauseTime();
            }
            local_std_get_map_bw = (num_request * size_of_get * 1000.0) / (MB * local_std_get_map_timer.getElapsedTime()*1.0);
            double local_std_get_bw_sum, local_std_put_bw_sum;
            if (client_comm_size == 1) {
                local_std_put_bw_sum = local_std_map_bandwidth;
                local_std_get_bw_sum = local_std_get_map_bw;
            }
            else {
                MPI_Reduce(&local_std_map_bandwidth, &local_std_put_bw_sum, 1,
                           MPI_DOUBLE, MPI_SUM, 0, client_comm);
                MPI_Reduce(&local_std_get_map_bw, &local_std_get_bw_sum, 1,
                           MPI_DOUBLE, MPI_SUM, 0, client_comm);
            }

            if (my_rank == 0) {
                printf("local std::um bw:\t put %f MB/s,\t get %f MB/s\n",
                       local_std_put_bw_sum / client_comm_size,
                       local_std_get_bw_sum / client_comm_size);
            }

            // MPI_Barrier(client_comm);
            // Timer remote_put_map_timer = Timer();
            // /*Remote map test*/
            // for (int i = 0; i < num_request; i++) {
            //     usleep(1000);
            //     size_t val = my_server + i * num_servers + num_servers * my_rank + 1;
            //     remote_put_map_timer.resumeTime();
            //     map.Put(KeyType(val), my_values);
            //     remote_put_map_timer.pauseTime();
            // }
            // double remote_map_bw = (num_request * size_of_put * 1000.0) / (MB * remote_put_map_timer.getElapsedTime()*1.0);
            // Timer remote_get_map_timer = Timer();
            // /*Remote map test*/
            // for (int i = 0; i < num_request; i++) {
            //     usleep(1000);
            //     size_t val = my_server + i * num_servers + num_servers * my_rank + 1;
            //     remote_get_map_timer.resumeTime();
            //     map.Get(KeyType(val));
            //     remote_get_map_timer.pauseTime();
            // }
            // double remote_get_map_bw = (num_request * size_of_get * 1000.0) / (MB * remote_get_map_timer.getElapsedTime()*1.0);
            // double remote_put_bw_sum, remote_get_bw_sum;
            // remote_put_bw_sum = remote_map_bw;
            // remote_get_bw_sum = remote_get_map_bw;
            // MPI_Reduce(&remote_map_bw, &remote_put_bw_sum, 1,
            //            MPI_DOUBLE, MPI_SUM, 0, client_comm);

            // MPI_Reduce(&remote_get_map_bw, &remote_get_bw_sum, 1,
            //            MPI_DOUBLE, MPI_SUM, 0, client_comm);
            // if (my_rank == 0) {
            //     printf("remote map bw:\t put: %f MB/s,\t get: %f MB/s\n",
            //            remote_put_bw_sum / client_comm_size,
            //            remote_get_bw_sum / client_comm_size);
            // }
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 1;
}
