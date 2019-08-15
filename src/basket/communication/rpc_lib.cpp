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

#include <basket/communication/rpc_lib.h>

RPC::~RPC() {
    delete server_list;

    if (is_server) {
        switch (BASKET_CONF->RPC_IMPLEMENTATION) {
#ifdef BASKET_ENABLE_RPCLIB
            case RPCLIB: {
          // Twiddle thumbs
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
                thallium_engine->finalize();
                break;
            }
#endif
        }

    }
}

RPC::RPC() : isInitialized(false), shared_init(false),
             is_server(BASKET_CONF->IS_SERVER),
             my_server(BASKET_CONF->MY_SERVER),
             num_servers(BASKET_CONF->NUM_SERVERS),
             server_on_node(BASKET_CONF->SERVER_ON_NODE),
             server_port(RPC_PORT) {
    AutoTrace trace = AutoTrace("RPC");
    if (!isInitialized) {
        int len;
        char proc_name[MPI_MAX_PROCESSOR_NAME];
        MPI_Get_processor_name(proc_name, &len);
        processor_name = std::string(proc_name);
        // so we can compare to servers and for server init

        server_list = new std::vector<std::string>();
        fstream file;
        file.open(BASKET_CONF->SERVER_LIST,ios::in);
        if (file.is_open()) {
            std::string file_line;
            std::string server_node;
            std::string server_network;
            server_on_node = false;  // in case there is no server on node
            while (getline(file, file_line)) {
                int split_loc = file_line.find(':');  // split to node and net
                if (split_loc != std::string::npos) {
                    server_node = file_line.substr(0, split_loc);
                    server_network = file_line.substr(split_loc+1, std::string::npos);
                    if (is_server) {
                        processor_name = server_network;  // set network to suggestion
                    }
                } else {
                    // no special network
                    server_node = file_line;
                    server_network = file_line;
                }
                // server list is list of network interfaces
                server_list->push_back(std::string(server_network));
            }
        } else {
            printf("Error: Can't open server list file %s\n", BASKET_CONF->SERVER_LIST.c_str());
            exit(EXIT_FAILURE);
        }
        file.close();

        /* Initialize MPI rank and size of world */
        MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
        MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

        /* if current rank is a server */
        if (is_server) {
            switch (BASKET_CONF->RPC_IMPLEMENTATION) {
#ifdef BASKET_ENABLE_RPCLIB
                case RPCLIB: {
                  rpclib_server = std::make_shared<rpc::server>(server_port+my_server);
                  break;
                }
#endif
#ifdef BASKET_ENABLE_THALLIUM_TCP
                case THALLIUM_TCP: {
                   engine_init_str = BASKET_CONF->TCP_CONF + "://" +
                                                  std::string(processor_name) +
                                                  ":" +
                                                  std::to_string(server_port + my_server);
                    break;
                }
#endif
#ifdef BASKET_ENABLE_THALLIUM_ROCE
                case THALLIUM_ROCE: {
                    engine_init_str = BASKET_CONF->VERBS_CONF + "://" +
                            BASKET_CONF->VERBS_DOMAIN + "://" +
                            std::string(processor_name) +
                            ":" +
                            std::to_string(server_port+my_server);
                    break;
                }
#endif
            }
        } else {
            switch (BASKET_CONF->RPC_IMPLEMENTATION) {
#ifdef BASKET_ENABLE_RPCLIB
                case RPCLIB: {
                  break;
                }
#endif
#ifdef BASKET_ENABLE_THALLIUM_TCP
                case THALLIUM_TCP: {
                    thallium_engine = Singleton<tl::engine>::GetInstance(BASKET_CONF->TCP_CONF,
                                                                         MARGO_CLIENT_MODE);
                    break;
                }
#endif
#ifdef BASKET_ENABLE_THALLIUM_ROCE
                case THALLIUM_ROCE: {
                  thallium_engine = Singleton<tl::engine>::GetInstance(BASKET_CONF->VERBS_CONF,
                                           MARGO_CLIENT_MODE);
                  break;
                }
#endif
            }
        }
        /* Create server list from the broadcast list*/
        isInitialized = true;
        run();
    }
}

void RPC::run(size_t workers) {
    AutoTrace trace = AutoTrace("RPC::run", workers);
    if (is_server){
        switch (BASKET_CONF->RPC_IMPLEMENTATION) {
#ifdef BASKET_ENABLE_RPCLIB
            case RPCLIB: {
                    rpclib_server->async_run(workers);
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
                    thallium_engine = Singleton<tl::engine>::GetInstance(engine_init_str, THALLIUM_SERVER_MODE,true,RPC_THREADS);
                    break;
                }
#endif
        }
    }
}
