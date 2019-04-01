//
// Created by Alex on 07.03.2019.
//

#ifndef HYDRAAPI_EX_HRG_SERVER_H
#define HYDRAAPI_EX_HRG_SERVER_H

#include "HRG_ServerConfig.h"
#include "asio.hpp"
#include <vector>
#include <functional>
#include <iostream>
#include "HRG_Impl.h"
using asio::ip::tcp;



class HRG_Executor{
public:
    tcp::endpoint addr;
    STATUS status;
};

class HRG_Server {
    HRG_ServerConfig config;

    asio::io_context io_context;
    tcp::acceptor acceptor;

    std::shared_ptr<std::ostream> log;
public:
    HRG_Server(HRG_ServerConfig config) :
        config(config),
        io_context(),
        acceptor(io_context, tcp::endpoint(tcp::v4(), config.port)),
        log(new HRG_Impl::NullStream()){}

    asio::detail::mutex srv_mutex_;
    std::vector<HRG_Executor> registered_;

    void setLog(std::shared_ptr<std::ostream> &&log);

    void start(){
        //asio::io_context io_context;
        //acceptor = tcp::acceptor(io_context, tcp::endpoint(tcp::v4(), config.port));
        io_context.run();
    }


    void do_accept(){
        *log << "Start accepting addr: " << acceptor.local_endpoint().address().to_string() << ' '
            << acceptor.local_endpoint().port() << std::endl << std::flush;

        //auto self(shared_from_this());
        using namespace std::placeholders;
        acceptor.async_accept(std::bind(&HRG_Server::accept_handler, this, _1, _2));
    }
    void accept_handler(asio::error_code error,
                       tcp::socket peer)
    {
        if (!error)
        {
            *log << "Accepted client " << peer.remote_endpoint().address().to_string() << std::endl << std::flush;
            //auto self(shared_from_this());
            std::make_shared<HRG_Impl::HRG_AcceptedClient>(std::move(peer), *this)->start();
            // Accept succeeded.
        }else{
            *log << "Error occurred " << error << std::endl << std::flush;
        }
        do_accept();
    }
};


#endif //HYDRAAPI_EX_HRG_SERVER_H