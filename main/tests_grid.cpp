//
// Created by Alex on 08.03.2019.
//

#include "tests_grid.h"
#include <iostream>
#include <memory>
#include <functional>
#include <fstream>

bool test_client_ok() {
    HRG_Server server(HRG_ServerConfig{7777});
    //bind std::cout
    server.setLog(std::shared_ptr<std::ostream>(&std::cout, [](std::ostream*){}));
    //bind file
    //server.setLog(std::shared_ptr<std::ostream>(new std::ofstream("file.txt")));

    server.do_accept(); server.start();
    std::string str;
    std::cin >> str;

    return true;
}

bool test_server_ok() {
    return false;
}
