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

#ifndef INCLUDE_BASKET_COMMUNICATION_RPC_LIB_CPP_
#define INCLUDE_BASKET_COMMUNICATION_RPC_LIB_CPP_

#include <future>

template <typename F> void RPC::bind(std::string str, F func) {
  server->bind(str, func);
}
template <typename... Args>
RPCLIB_MSGPACK::object_handle RPC::call(uint16_t server_index,
                                        std::string const &func_name,
                                        Args... args) {
  AutoTrace trace = AutoTrace("RPC::call", server_index, func_name);
  int16_t port = server_port + server_index;
  /* Connect to Server */
  rpc::client client(server_list->at(server_index).c_str(), port);
  // client.set_timeout(5000);
  return client.call(func_name, std::forward<Args>(args)...);
}
template <typename... Args>
std::future<RPCLIB_MSGPACK::object_handle> RPC::async_call(
    uint16_t server_index, std::string const &func_name,
    Args... args) {
  AutoTrace trace = AutoTrace("RPC::async_call", server_index, func_name);
  int16_t port = server_port + server_index;
  /* Connect to Server */
  rpc::client client(server_list->at(server_index).c_str(), port);
  // client.set_timeout(5000);
  return client.async_call(func_name, std::forward<Args>(args)...);
}
#endif  // INCLUDE_BASKET_COMMUNICATION_RPC_LIB_CPP_
