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
#include <iostream>
#include <signal.h>
#include <execinfo.h>
#include <chrono>
#include <rpc/client.h>
#include <basket/common/data_structures.h>

void bt_sighandler(int sig, struct sigcontext ctx) {

    void *trace[16];
    char **messages = (char **)NULL;
    int i, trace_size = 0;

    if (sig == SIGSEGV)
        printf("Got signal %d, faulty address is %p, "
               "from %p\n", sig, ctx.cr2, ctx.rip);
    else
        printf("Got signal %d\n", sig);

    trace_size = backtrace(trace, 16);
    /* overwrite sigaction with caller's address */
    trace[1] = (void *)ctx.rip;
    messages = backtrace_symbols(trace, trace_size);
    /* skip first stack frame (points here) */
    printf("[bt] Execution path:\n");
    for (i=1; i<trace_size; ++i)
    {
        printf("[bt] #%d %s\n", i, messages[i]);

        /* find first occurence of '(' or ' ' in message[i] and assume
         * everything before that is the file name. (Don't go beyond 0 though
         * (string terminator)*/
        size_t p = 0;
        while(messages[i][p] != '(' && messages[i][p] != ' '
              && messages[i][p] != 0)
            ++p;

        char syscom[256];
        sprintf(syscom,"addr2line %p -e %.*s", trace[i], p, messages[i]);
        //last parameter is the file name of the symbol
        system(syscom);
    }

    exit(0);
}

struct ValueType{
    int a;
    MSGPACK_DEFINE(a);
};

int main (int argc,char* argv[])
{
   // struct sigaction sa;

   //  sa.sa_handler = reinterpret_cast<__sighandler_t>(bt_sighandler);
   //  sigemptyset(&sa.sa_mask);
   //  sa.sa_flags = SA_RESTART;

   //  sigaction(SIGSEGV, &sa, NULL);
   //  sigaction(SIGUSR1, &sa, NULL);
   //  sigaction(SIGABRT, &sa, NULL);

    MPI_Init(&argc,&argv);


    int comm_size,my_rank;
    MPI_Comm_size(MPI_COMM_WORLD,&comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);

    if(my_rank==0){
        char hostname[256];
        gethostname(hostname, sizeof(hostname));
        printf("%d ready for attach\n", comm_size);
        fflush(stdout);
        getchar();
    }
    MPI_Barrier(MPI_COMM_WORLD);
    int ranks_per_server=comm_size,case_num=0;
    if(argc > 1)    ranks_per_server = atoi(argv[1]);
    if(argc > 2)    case_num = atoi(argv[2]);
    // Layer::LAST=new Layer();
    DistributedHashMap<int,string> map("hi", my_rank == 0, 0, 1);
    DistributedHashMap<int,string> map2("hi2", my_rank == 1, 1, 1);

    if (my_rank != 0) {
        for(int i=0;i<8;i++){
            if(i%2==0) map.Put(i*my_rank+1,"H");
            else map2.Put(i*my_rank+1,"H");
        }
    }

    /*DistributedHashMap<int,ValueType> map=DistributedHashMap<int,ValueType>("hi", 1024ULL * 1024ULL * 1024ULL, ranks_per_server);
    if(case_num==0){
        if(my_rank == comm_size-1){
            for(int i=0;i<comm_size-1;i++){
                ValueType v;
                v.a=i;
                map.Put(i,v);
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);
        if(my_rank != comm_size-1){
            printf("my_rank: %d, my_value:%d\n",my_rank,map.Get(my_rank).second.a);
        }else{
        }
    }
    else if(case_num==1){
        if((my_rank+1)%ranks_per_server==0){
            for(int i=0;i<1024*1024-1;i++){
                ValueType v;
                v.a=i;
                map.Put(i,v);
            }
        }
    }
    else if(case_num==2){
        ValueType v2;
        v2.a=1;
        map.Put(1,v2);
        if((my_rank+1)%ranks_per_server==0){
            for(int i=0;i<1024*1024-1;i++){
                map.Get(1);
            }
        }
    }else if(case_num==3){
        int loop=1024*1024*1024;
        if(argc > 3) loop = atoi(argv[3]);
        auto start = std::chrono::steady_clock::now();
        if((my_rank+1)%ranks_per_server!=0){
            for(really_long i=0;i<loop;i++){
                ValueType v;
                v.a=i;
                map.Put((my_rank/ranks_per_server+1)%(comm_size/ranks_per_server),v);
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);
        auto end = std::chrono::steady_clock::now();
        double elapsed=std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()/1000000.0;
        if(my_rank==0)
            printf("time(sec),%f,OPS,%f\n",elapsed,loop*comm_size*sizeof(ValueType)/elapsed);
    }else if(case_num==4){
        ValueType v2;
        v2.a=1;
        if((my_rank+1)%ranks_per_server==0){
            map.Put(my_rank/ranks_per_server,v2);
        }
        MPI_Barrier(MPI_COMM_WORLD);
        if((my_rank+1)%ranks_per_server!=0){
            for(int i=0;i<1024*1024-1;i++){
                map.Get(my_rank/ranks_per_server);
            }
        }
    }*/

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;
}
