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
#include <basket.h>
#include <utility>
#include <mpi.h>
#include "util.h"
#include <iostream>
#include <signal.h>
#include <execinfo.h>
#include <chrono>
#include <rpc/client.h>
#include <basket/common/data_structures.h>

// arguments: ranks_per_server, num_request, debug
int main (int argc,char* argv[])
{
    SetSignal();
    MPI_Init(&argc,&argv);
    int comm_size,my_rank;
    MPI_Comm_size(MPI_COMM_WORLD,&comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);
    int ranks_per_server=comm_size,num_request=10000;
    bool debug=false;
    if(argc > 1)    ranks_per_server = atoi(argv[1]);
    if(argc > 2)    num_request = atoi(argv[2]);
    if(argc > 3)    debug = true;
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

    int my_server=my_rank / ranks_per_server;
    int num_servers=comm_size/ranks_per_server;
    const int array_size=1;
    //printf("rank %d, is_server %d, my_server %d, num_servers %d\n",my_rank,is_server,my_server,num_servers);
    basket::unordered_map<KeyType,int> map("test", is_server, my_server, num_servers, true);
    int myints;
    Timer local_map_timer=Timer();
    /*Local map test*/
    for(int i=0;i<num_request;i++){
        size_t val=my_server+i*num_servers*my_rank;
        local_map_timer.resumeTime();
        map.Put(KeyType(val),myints);
        local_map_timer.pauseTime();
    }
    double local_map_throughput=num_request/local_map_timer.getElapsedTime()*1000*ranks_per_server;
    Timer local_get_map_timer=Timer();
    /*Local map test*/
    for(int i=0;i<num_request;i++){
        size_t val=my_server+i*num_servers*my_rank;
        local_get_map_timer.resumeTime();
        map.Get(KeyType(val));
        local_get_map_timer.pauseTime();
    }
    double local_get_map_throughput=num_request/local_get_map_timer.getElapsedTime()*1000*ranks_per_server;
    if(my_rank==0){
        printf("map_throughput: put %f, get %f\n",local_map_throughput,local_get_map_throughput);
    }
    // Timer remote_map_timer=Timer();
    /*Remote map test*/
    // for(int i=0;i<num_request;i++){
    //     size_t val=my_server+1;
    //     remote_map_timer.resumeTime();
    //     map.Put(KeyType(val),myints);
    //     remote_map_timer.pauseTime();
    // }
    // double remote_map_throughput=num_request/remote_map_timer.getElapsedTime()*1000;
    // Timer remote_get_map_timer=Timer();
    /*Remote map test*/
    // for(int i=0;i<num_request;i++){
    //     size_t val=my_server+1;
    //     remote_get_map_timer.resumeTime();
    //     map.Get(KeyType(val));
    //     remote_get_map_timer.pauseTime();
    // }
    // double remote_get_map_throughput=num_request/remote_get_map_timer.getElapsedTime()*1000;
    // if(my_rank==0){
    //   printf("remote put map throughput: %f, remote get map throughput: %f\n",remote_map_throughput,remote_get_map_throughput);
    //   printf("\n");
    // }
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    return 1;
}
