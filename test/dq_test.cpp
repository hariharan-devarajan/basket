#include "../include/basket.h"
#include <iostream>
#include <mpi.h>

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int my_server = 0;
    if (rank == 0) {
        DistributedMessageQueue<int> int_queue("QUEUE", true, my_server, 1);
        int_queue.WaitForElement(my_server);
        auto result = int_queue.Pop(my_server);
        if (result.first) {
            std::cout << result.second << std::endl;
        }
    } else {
        DistributedMessageQueue<int> int_queue("QUEUE", false, my_server, 1);
        int_queue.Push(42, my_server);
    }
    MPI_Finalize();
}
