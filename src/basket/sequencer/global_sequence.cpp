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

#include <basket/sequencer/global_sequence.h>

namespace basket {

global_sequence::~global_sequence() {
    if (is_server) bip::shared_memory_object::remove(name.c_str());
}
global_sequence::global_sequence(std::string name_,
                                 bool is_server_,
                                 uint16_t my_server_,
                                 int num_servers_,
                                 bool server_on_node_)
        : is_server(is_server_), my_server(my_server_), num_servers(num_servers_),
          comm_size(1), my_rank(0), memory_allocated(1024ULL * 1024ULL * 128ULL),
          name(name_), segment(), func_prefix(name_),
          server_on_node(server_on_node_) {
    AutoTrace trace = AutoTrace("basket::global_sequence", name_, is_server_, my_server_,
                                num_servers_);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    name = name+"_"+std::to_string(my_server);
    rpc = Singleton<RPC>::GetInstance("RPC_SERVER_LIST", is_server_, my_server_,
                                      num_servers_);
    if (is_server) {
        std::function<uint64_t(void)> getNextSequence(std::bind(
            &basket::global_sequence::LocalGetNextSequence, this));
        rpc->bind(func_prefix+"_GetNextSequence", getNextSequence);
        bip::shared_memory_object::remove(name.c_str());
        segment = bip::managed_shared_memory(bip::create_only, name.c_str(),
                                             65536);
        value = segment.construct<uint64_t>(name.c_str())(0);
        mutex = segment.construct<boost::interprocess::interprocess_mutex>(
            "mtx")();
    }
    MPI_Barrier(MPI_COMM_WORLD);
    if (!is_server) {
        segment = bip::managed_shared_memory(bip::open_only, name.c_str());
        std::pair<uint64_t*, bip::managed_shared_memory::size_type> res;
        res = segment.find<uint64_t> (name.c_str());
        value = res.first;
        std::pair<bip::interprocess_mutex *,
                  bip::managed_shared_memory::size_type> res2;
        res2 = segment.find<bip::interprocess_mutex>("mtx");
        mutex = res2.first;
    }
    MPI_Barrier(MPI_COMM_WORLD);
}

uint64_t global_sequence::LocalGetNextSequence() {
    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex>
            lock(*mutex);
    return ++*value;
}

uint64_t global_sequence::GetNextSequence() {
    if (server_on_node) {
        return LocalGetNextSequence();
    }
    else {
        return rpc->call(my_server, func_prefix+"_GetNextSequence").as<uint64_t>();
    }
}

uint64_t global_sequence::GetNextSequenceServer(uint16_t server) {
    if (my_server == server && server_on_node) {
        return LocalGetNextSequence();
    }
    else {
        return rpc->call(server, func_prefix+"_GetNextSequence").as<uint64_t>();
    }
}

}  // namespace basket
