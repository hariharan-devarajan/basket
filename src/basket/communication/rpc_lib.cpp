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
    if (BASKET_CONF->IS_SERVER) {
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

RPC::RPC() : server_list(),
             server_port(RPC_PORT) {
    AutoTrace trace = AutoTrace("RPC");

    server_list = BASKET_CONF->LoadLayers();

    /* if current rank is a server */
    if (BASKET_CONF->IS_SERVER) {
        switch (BASKET_CONF->RPC_IMPLEMENTATION) {
#ifdef BASKET_ENABLE_RPCLIB
            case RPCLIB: {
              rpclib_server = std::make_shared<rpc::server>(server_port+BASKET_CONF->MY_SERVER);
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
    run();
}

void RPC::run(size_t workers) {
    AutoTrace trace = AutoTrace("RPC::run", workers);
    if (BASKET_CONF->IS_SERVER){
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
