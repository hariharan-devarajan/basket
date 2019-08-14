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

//
// Created by hariharan on 3/19/19.
//

#ifndef INCLUDE_BASKET_COMMON_CONFIGURATION_MANAGER_H
#define INCLUDE_BASKET_COMMON_CONFIGURATION_MANAGER_H

#include <basket/common/debug.h>
#include <basket/common/enumerations.h>
#include <basket/common/singleton.h>

namespace basket{

    class ConfigurationManager {
    public:
        uint16_t RPC_PORT;
        uint16_t RPC_THREADS;
        RPCImplementation RPC_IMPLEMENTATION;
        int MPI_RANK, COMM_SIZE;
      std::string TCP_CONF;
      std::string VERBS_CONF;
      std::string VERBS_DOMAIN;

      ConfigurationManager():
        RPC_PORT(8080), RPC_THREADS(1),
#if defined(BASKET_ENABLE_RPCLIB)
        RPC_IMPLEMENTATION(RPCLIB),
#elif defined(BASKET_ENABLE_THALLIUM_TCP)
        RPC_IMPLEMENTATION(THALLIUM_TCP),
#elif defined(BASKET_ENABLE_THALLIUM_ROCE)
        RPC_IMPLEMENTATION(THALLIUM_ROCE),
#endif
        TCP_CONF("ofi+tcp"), VERBS_CONF("verbs"), VERBS_DOMAIN("mlx5_0") {
            AutoTrace trace = AutoTrace("ConfigurationManager");
            MPI_Comm_size(MPI_COMM_WORLD, &COMM_SIZE);
            MPI_Comm_rank(MPI_COMM_WORLD, &MPI_RANK);
        }
    };

}

#endif //INCLUDE_BASKET_COMMON_CONFIGURATION_MANAGER_H
