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

#include <basket/clock/global_clock.h>

namespace basket {
/*
 * Destructor removes shared memory from the server
 */
global_clock::~global_clock() {
  AutoTrace trace = AutoTrace("basket::~global_clock", NULL);
  if (is_server) bip::shared_memory_object::remove(name.c_str());
}

/*
 * Constructor gets the start time in a node for later use, binds RPC
 * calls, and sets up mutexes for later locking.
 */
global_clock::global_clock(std::string name_,
                           bool is_server_,
                           uint16_t my_server_,
                           int num_servers_)
    : is_server(is_server_), my_server(my_server_), num_servers(num_servers_),
      comm_size(1), my_rank(0), memory_allocated(1024ULL), name(name_),
      segment(), func_prefix(name_) {
  AutoTrace trace = AutoTrace("basket::global_clock", name_, is_server_, my_server_,
                              num_servers_);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  name = name+"_"+std::to_string(my_server);
  rpc = Singleton<RPC>::GetInstance("RPC_SERVER_LIST", is_server_, my_server_,
                                    num_servers_);
  if (is_server) {
    std::function<HTime(void)> getTimeFunction(
        std::bind(&global_clock::GetTime, this));
    rpc->bind(func_prefix+"_GetTime", getTimeFunction);
    bip::shared_memory_object::remove(name.c_str());
    segment = bip::managed_shared_memory(bip::create_only, name.c_str(),
                                         65536);
    start = segment.construct<chrono_time>("Time")(
        std::chrono::high_resolution_clock::now());
    mutex = segment.construct<boost::interprocess::interprocess_mutex>(
        "mtx")();
  }
  MPI_Barrier(MPI_COMM_WORLD);

  if (!is_server) {
    segment = bip::managed_shared_memory(bip::open_only, name.c_str());
    std::pair<chrono_time*, bip::managed_shared_memory::size_type> res;
    res = segment.find<chrono_time> ("Time");
    start = res.first;
    std::pair<bip::interprocess_mutex *,
              bip::managed_shared_memory::size_type> res2;
    res2 = segment.find<bip::interprocess_mutex>("mtx");
    mutex = res2.first;
  }
  MPI_Barrier(MPI_COMM_WORLD);
}

/*
 * GetTime() returns the time locally within a node using chrono
 * high_resolution_clock
 */
HTime global_clock::GetTime() {
  AutoTrace trace = AutoTrace("basket::global_clock::GetTime", NULL);
  boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex>
      lock(*mutex);
  auto t2 = std::chrono::high_resolution_clock::now();
  auto t =  std::chrono::duration_cast<std::chrono::microseconds>(
      t2 - *start).count();
  return t;
}

/*
 * GetTimeServer() returns the time on the requested server using RPC calls, or
 * the local time if the server requested is the current client server
 */
HTime global_clock::GetTimeServer(uint16_t server) {
  AutoTrace trace = AutoTrace("basket::global_clock::GetTimeServer", server);
  if (my_server == server) {
    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex>
        lock(*mutex);
    auto t2 = std::chrono::high_resolution_clock::now();
    auto t =  std::chrono::duration_cast<std::chrono::microseconds>(
        t2 - *start).count();
    return t;
  }return rpc->call(server, func_prefix+"_GetTime").as<HTime>();
}

}  // namespace basket