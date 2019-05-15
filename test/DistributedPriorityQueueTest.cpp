#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <functional>
#include <utility>
#include "../include/basket.h"
#include <mpi.h>
#include <iostream>
#include <signal.h>
#include <execinfo.h>
#include <chrono>

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
namespace std {
    template<>
    struct less<ValueType>
    {
        bool operator()(const ValueType& k1, const ValueType& k2) const
        {
            return k1.a < k2.a;
        }
    };
}


int main (int argc,char* argv[])
{
    struct sigaction sa;

    sa.sa_handler = reinterpret_cast<__sighandler_t>(bt_sighandler);
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);

    MPI_Init(&argc,&argv);


    int comm_size,my_rank;
    MPI_Comm_size(MPI_COMM_WORLD,&comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);
    int ranks_per_server=comm_size,case_num=0;
    if(argc > 1)    ranks_per_server = atoi(argv[1]);
    if(argc > 2)    case_num = atoi(argv[2]);
    DistributedPriorityQueue<ValueType> queue("hi", my_rank==0, 0, 1);
    if(case_num==0){
        if(my_rank == comm_size-1){
            for(int i=0;i<comm_size-1;i++){
                ValueType v;
                v.a=i;
                queue.Push(v,i);
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);
        if(my_rank != comm_size-1){
            printf("my_rank: %d, my_value:%d\n",my_rank,queue.Pop(my_rank).second.a);
        }else{
        }
    }else if(case_num==1){
        if((my_rank+1)%ranks_per_server==0){
            for(int i=0;i<1024*1024-1;i++){
                ValueType v;
                v.a=i;
                queue.Push(v,i);
            }
        }
    }
    else if(case_num==2){
        ValueType v2;
        v2.a=1;
        queue.Push(v2,1);
        if((my_rank+1)%ranks_per_server==0){
            for(int i=0;i<1024*1024-1;i++){
                queue.Pop(1);
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
                queue.Push(v,my_rank/ranks_per_server);
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);
        auto end = std::chrono::steady_clock::now();
        double elapsed=std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()/1000000.0;
        if(my_rank==0)
            printf("time(sec),%f,OPS,%f\n",elapsed,loop*comm_size/elapsed);
    }else if(case_num==4){
        ValueType v2;
        v2.a=1;
        if((my_rank+1)%ranks_per_server==0){
            queue.Push(v2,my_rank/ranks_per_server);
        }
        MPI_Barrier(MPI_COMM_WORLD);
        if((my_rank+1)%ranks_per_server!=0){
            for(int i=0;i<1024*1024-1;i++){
                queue.Pop(my_rank/ranks_per_server);
            }
        }
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;
}
