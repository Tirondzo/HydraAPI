//
// Created by Alex on 07.03.2019.
//

#ifndef HYDRAAPI_EX_HRG_SERVERCONFIG_H
#define HYDRAAPI_EX_HRG_SERVERCONFIG_H

#include "asio.hpp"

typedef unsigned short ushort;

using asio::ip::tcp;
class HRG_ServerConfig {
public:
    ushort port;
};


#endif //HYDRAAPI_EX_HRG_SERVERCONFIG_H
