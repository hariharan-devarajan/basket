/*
 * Copyright (C) 2019  Chris Hogan
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

#include <cassert>
#include <cstdlib>
#include <ctime>

#include <mpi.h>
#include "util.h"
#include <basket/unordered_map/unordered_map.h>

int main (int argc, char* argv[])
{
    MpiData mpi = initMpiData(&argc, &argv);

    if (mpi.comm_size != 2) {
        fprintf(stderr, "thallium_bulk_test is meant to be run with 2 MPI processes\n");
        exit(EXIT_FAILURE);
    }

    int num_requests = 1;
    bool is_server = mpi.rank == mpi.comm_size - 1;

    BASKET_CONF->IS_SERVER = is_server;
    BASKET_CONF->MY_SERVER = 0;
    BASKET_CONF->NUM_SERVERS = 1;
    BASKET_CONF->SERVER_ON_NODE = is_server;
    BASKET_CONF->SERVER_LIST_PATH = "./server_list";

    constexpr int array_size= 1000;
    constexpr size_t size_of_elem = sizeof(array_size);
    using MyArray = std::array<int, array_size>;
    using MyMap = basket::unordered_map<KeyType, MyArray>;

    MyMap *map;
    CharStruct shm_name("THALLIUM_BULK_TEST");
    if (is_server) {
        map = new MyMap(shm_name);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    if (!is_server) {
        map = new MyMap(shm_name);
    }

    if (!is_server) {
        for(int i = 0; i < num_requests; ++i) {
            size_t val = i;
            auto key = KeyType(val);
            MyArray arr;
            for (auto &x : arr) {
                x = 5;
            }
            std::cout << "Put " << i << "\n";
            map->BulkPut(key, arr);
        }

        for(int i = 0; i < num_requests; ++i) {
            size_t val = i;
            auto key = KeyType(val);
            std::cout << "Get " << i << "\n";
            auto result = map->BulkGet(key);
            assert(result.first);
            MyArray arr = result.second;
            assert(arr.size() == array_size);
            for (const auto &x : arr) {
                assert(x == 5);
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    delete(map);
    MPI_Finalize();

    return 0;
}
