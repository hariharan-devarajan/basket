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

template <typename F> 
void RPC::bind(std::string str, F func) {
    switch (CONF->RPC_IMPLEMENTATION) {
#ifdef BASKET_ENABLE_RPCLIB
        case RPCLIB: {
            rpclib_server->bind(str, func);
            break;
        }
#endif
#ifdef BASKET_ENABLE_THALLIUM_TCP
        case THALLIUM_TCP:
#endif
#ifdef BASKET_ENABLE_THALLIUM_ROCE
        case THALLIUM_ROCE:
#endif
#if defined(BASKET_ENABLE_THALLIUM_TCP) || defined(BASKET_ENABLE_THALLIUM_ROCE)
            {
                thallium_engine->define(str, func);
                break;
            }
#endif
    }
}

template <typename Response, typename... Args>
Response RPC::call(uint16_t server_index,
                   std::string const &func_name,
                   Args... args) {
    AutoTrace trace = AutoTrace("RPC::call", server_index, func_name);
    int16_t port = server_port + server_index;
    
    switch (CONF->RPC_IMPLEMENTATION) {
#ifdef BASKET_ENABLE_RPCLIB
        case RPCLIB: {
            /* Connect to Server */
            rpc::client client(server_list->at(server_index).c_str(), port);
            // client.set_timeout(5000);
            return client.call(func_name, std::forward<Args>(args)...);
            break;
        }
#endif
#ifdef BASKET_ENABLE_THALLIUM_TCP
        case THALLIUM_TCP: {
            /* Connect to Server */

            std::shared_ptr<tl::engine> thallium_client;
            if (is_server) {
                thallium_client = std::make_shared<tl::engine>(CONF->TCP_CONF, MARGO_CLIENT_MODE);
            }
            else {
                thallium_client = thallium_engine;
            }

            tl::remote_procedure remote_procedure = thallium_client->define(func_name);
            // Setup args for RDMA bulk transfer
            // std::vector<std::pair<void*,std::size_t>> segments(num_args);

            // We use addr lookup because mercury addresses must be exactly 15 char
            char ip[16];
            struct hostent *he = gethostbyname(server_list->at(server_index).c_str());
            in_addr **addr_list = (struct in_addr **)he->h_addr_list;
            strcpy(ip, inet_ntoa(*addr_list[0]));

            std::string lookup_str = CONF->TCP_CONF + "://" + std::string(ip) + ":" + 
                    std::to_string(port);
            tl::endpoint server_endpoint = thallium_client->lookup(lookup_str);
            return remote_procedure.on(server_endpoint)(std::forward<Args>(args)...);
            break;
        }
#endif
#ifdef BASKET_ENABLE_THALLIUM_ROCE
        case THALLIUM_ROCE: {
            /* Connect to Server */

            std::shared_ptr<tl::engine> thallium_client;
            if (is_server) {
                thallium_client = std::make_shared<tl::engine>(CONF->VERBS_CONF, MARGO_CLIENT_MODE);
            }
            else {
                thallium_client = thallium_engine;
            }

            tl::remote_procedure remote_procedure = thallium_client->define(func_name);

            // Setup args for RDMA bulk transfer
            // std::vector<std::pair<void*,std::size_t>> segments(num_args);

            // We use addr lookup because mercury addresses must be exactly 15 char
            char ip[16];
            struct hostent *he = gethostbyname(server_list->at(server_index).c_str());
            in_addr **addr_list = (struct in_addr **)he->h_addr_list;
            strcpy(ip, inet_ntoa(*addr_list[0]));

            std::string lookup_str = CONF->VERBS_CONF + "://" + std::string(ip) + ":" + 
                    std::to_string(port);
            tl::endpoint server_endpoint = thallium_client->lookup(lookup_str);
            // if (func_name == "test_Get") {
            //     tl::bulk bulk_handle = remote_procedure.on(server_endpoint)(std::forward<Args>(args)...);
            //     return std::make_pair(lookup_str, bulk_handle);
            // }
            // else {
                return remote_procedure.on(server_endpoint)(std::forward<Args>(args)...);
            // }
            break;
        }
#endif
    }
}

#ifdef BASKET_ENABLE_THALLIUM_ROCE
// These are still experimental for using RDMA bulk transfers
template<typename MappedType>
MappedType RPC::prep_rdma_server(tl::endpoint endpoint, tl::bulk &bulk_handle) {
    // MappedType buffer;
    std::string buffer;
    buffer.resize(1000000, 'a');
    std::vector<std::pair<void *, size_t>> segments(1);
    segments[0].first = (void *)(&buffer[0]);
    segments[0].second = 1000000 + 1;
    tl::bulk local = thallium_engine->expose(segments, tl::bulk_mode::write_only);
    bulk_handle.on(endpoint) >> local;
    return buffer;
}

template<typename MappedType>
tl::bulk RPC::prep_rdma_client(MappedType &data) {
    MappedType my_buffer = data;
    std::vector<std::pair<void *, std::size_t>> segments(1);
    segments[0].first = (void *)&my_buffer[0];
    segments[0].second = 1000000 + 1;
    return thallium_engine->expose(segments, tl::bulk_mode::read_only);
}
#endif

#endif  // INCLUDE_BASKET_COMMUNICATION_RPC_LIB_CPP_
