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


#define MB 1024*1024

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
    if(argc > 4)    debug = (bool)atoi(argv[5]);


    if(comm_size/ranks_per_server < 2){
        perror("comm_size/ranks_per_server should be atleast 2 for this test\n");
        exit(-1);
    }
    if(debug && my_rank==0){
        printf("%d ready for attach\n", comm_size);
        fflush(stdout);
        getchar();
    }
    MPI_Barrier(MPI_COMM_WORLD);

    // every node should have 1 server
    bool is_server=(my_rank+1) % ranks_per_server == 0;

    int num_servers=comm_size/ranks_per_server;
    int my_server = my_rank / ranks_per_server;
    // int my_server=((my_rank + 2) / ranks_per_server) % num_servers;
    // if (is_server) {
    //     my_server = my_rank / ranks_per_server;
    // }
    int len;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    MPI_Get_processor_name(processor_name, &len);

    std::string proc_name = std::string(processor_name);
    /*int split_loc = proc_name.find('.');
    std::string node_name = proc_name.substr(0, split_loc);
    std::string extra_info = proc_name.substr(split_loc+1, string::npos);
    proc_name = node_name + "-40g." + extra_info;*/
    std::string my_values = printRandomString(num_of_int);
    for(int i=0;i<num_of_int;i++) my_values.push_back(0);
    if(debug){
        printf("node %s, rank %d, is_server %d, my_server %d, num_servers %d\n",proc_name.c_str(),my_rank,is_server,my_server,num_servers);
    }

    {
        basket::unordered_map<KeyType, std::string> map("test", is_server, my_server, num_servers,
                                                        server_on_node || is_server, proc_name);

        int size_of_data = sizeof(KeyType) + sizeof(int) * my_values.size();
        double local_map_bandwidth;
        double local_get_map_bw;
        Timer local_put_map_timer = Timer();
        /*Local map test*/
        for (int i = 0; i < num_request; i++) {
            size_t val = my_server + i * num_servers * my_rank;
            local_put_map_timer.resumeTime();
            map.Put(KeyType(val), my_values);
            local_put_map_timer.pauseTime();
        }
        local_map_bandwidth = (num_request * size_of_data * 1000.0) / (MB * local_put_map_timer.getElapsedTime()*1.0);
        Timer local_get_map_timer = Timer();
        /*Local map test*/
        for (int i = 0; i < num_request; i++) {
            size_t val = my_server + i * num_servers * my_rank;
            local_get_map_timer.resumeTime();
            auto ret = map.Get(KeyType(val));
            local_get_map_timer.pauseTime();
        }
        local_get_map_bw = (num_request * size_of_data * 1000.0) / (MB * local_get_map_timer.getElapsedTime()*1.0);
        double local_get_bw_sum, local_put_bw_sum;

        MPI_Reduce(&local_map_bandwidth, &local_put_bw_sum, 1,
                   MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Reduce(&local_get_map_bw, &local_get_bw_sum, 1,
                   MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

        if (my_rank == 0) {
            printf("local bw:\t put %f MB/s,\t get %f MB/s\n",
                   local_put_bw_sum / comm_size,
                   local_get_bw_sum / comm_size);
        }

        MPI_Barrier(MPI_COMM_WORLD);
        Timer remote_put_map_timer = Timer();
        /*Remote map test*/
        for (int i = 0; i < num_request; i++) {
            usleep(100);
            size_t val = my_server + i * num_servers + num_servers * my_rank + 1;
            remote_put_map_timer.resumeTime();
            map.Put(KeyType(val), my_values);
            remote_put_map_timer.pauseTime();
        }
        double remote_map_bw = (num_request * size_of_data * 1000.0) / (MB * remote_put_map_timer.getElapsedTime()*1.0);
        Timer remote_get_map_timer = Timer();
        /*Remote map test*/
        for (int i = 0; i < num_request; i++) {
            usleep(100);
            size_t val = my_server + i * num_servers + num_servers * my_rank + 1;
            remote_get_map_timer.resumeTime();
            map.Get(KeyType(val));
            remote_get_map_timer.pauseTime();
        }
        double remote_get_map_bw = (num_request * size_of_data * 1000.0) / (MB * remote_get_map_timer.getElapsedTime()*1.0);
        double remote_put_bw_sum, remote_get_bw_sum;
        MPI_Reduce(&remote_map_bw, &remote_put_bw_sum, 1,
                   MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

        MPI_Reduce(&remote_get_map_bw, &remote_get_bw_sum, 1,
                   MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
        if (my_rank == 0) {
            printf("remote map bw:\t put: %f MB/s,\t get: %f MB/s\n",
                   remote_put_bw_sum / comm_size,
                   remote_get_bw_sum / comm_size);
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
    MPI_Finalize();
    return 1;
}
