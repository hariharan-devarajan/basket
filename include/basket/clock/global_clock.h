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

#ifndef INCLUDE_BASKET_CLOCK_GLOBAL_CLOCK_H_
#define INCLUDE_BASKET_CLOCK_GLOBAL_CLOCK_H_

#include <stdint-gcc.h>
#include <mpi.h>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <basket/common/singleton.h>
#include <basket/common/data_structures.h>
#include <basket/common/typedefs.h>
#include <basket/communication/rpc_lib.h>
#include <basket/common/debug.h>
#include <utility>
#include <memory>
#include <string>

namespace bip = boost::interprocess;

namespace basket {
class global_clock {
 private:
  typedef std::chrono::high_resolution_clock::time_point chrono_time;
  chrono_time *start;
  bool is_server;
  bip::interprocess_mutex* mutex;
  really_long memory_allocated;
  int my_rank, comm_size, num_servers;
  uint16_t my_server;
  bip::managed_shared_memory segment;
  std::string name, func_prefix;
  std::shared_ptr<RPC> rpc;

 public:
  ~global_clock();
  global_clock(std::string name_,
              bool is_server_,
              uint16_t my_server_,
              int num_servers_);

  HTime GetTime();
  HTime GetTimeServer(uint16_t server);
};

}  // namespace basket

#endif  // INCLUDE_BASKET_CLOCK_GLOBAL_CLOCK_H_
