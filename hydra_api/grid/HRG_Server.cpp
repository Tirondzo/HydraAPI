//
// Created by Alex on 07.03.2019.
//

#include "HRG_Server.h"

#include "asio/impl/src.hpp"

void HRG_Server::setLog(std::shared_ptr<std::ostream> &&log) {
    HRG_Server::log = std::move(log);
}
