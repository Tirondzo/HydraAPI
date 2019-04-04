//
// Created by Alex on 22.03.2019.
//

#ifndef HYDRAAPI_EX_HRG_IMPL_H
#define HYDRAAPI_EX_HRG_IMPL_H

#include "pugixml.hpp"

#include "asio.hpp"
#include <vector>
#include <list>
#include <functional>
#include <regex>
#include <algorithm>

#include <fstream>
#include <iostream>

class HRG_Server;


enum MESSAGES{
    REGISTRATION,
    GET_STATUS,
    SEND_TASK,
    GET_INFO
};
enum STATUS{
    OFFLINE,
    ONLINE,
    DOWNLOADING,
    RENDERING,
    UPLOADING
};
enum RESPONSE{
    OK,
    ERR
};

namespace HRG_Impl {
    using namespace asio::ip;

    //Null buffers to disable logging
    class NullBuffer : public std::streambuf {
    public: int overflow(int c) { return c; }
    };
    class NullStream : public std::ostream {
    public: NullStream() : std::ostream(&m_sb) {}
    private: NullBuffer m_sb;
    };

    //This code underline allows us to omit placeholders in std::bind
    //So you don't need to write code like std::bind(Class::f, class, _1, _2, _3, _4);
    //Enough to write bind_this(Class::f, class);
    template <class C, typename Ret, typename ... Ts>
    std::function<Ret(Ts...)> bind_this(Ret (C::*m)(Ts...), C* c)
    {
        return [=](auto&&... args) { return (c->*m)(std::forward<decltype(args)>(args)...); };
    }

    template <class C, typename Ret, typename ... Ts>
    std::function<Ret(Ts...)> bind_this(Ret (C::*m)(Ts...) const, const C* c)
    {
        return [=](auto&&... args) { return (c->*m)(std::forward<decltype(args)>(args)...); };
    }


    struct ResponseResult {
        RESPONSE response;
        asio::error_code error_code;
        std::string error_message;
    };

    struct Executor{
        std::string client_id;
        std::list<std::string> shared_path;
        bool busy;

        //TODO: Preferences (GPU, CPU, Bandwidth, etc...)
    };

    struct Config{
        std::string scene_id;
        bool allow_to_share;
        std::string download;

        //TODO: Config preferences (...)
    };

    struct Job{
        std::string job_id;
        std::string exe_id;

        //TODO: Samples details (samples, ...)
    };

    class HRG_Protocol{
    public:
        typedef std::function<bool(const ResponseResult&, const std::string)> GetClientHandler;
        typedef std::function<void(const ResponseResult&)> GetServerHandler;
        typedef std::function<void(const ResponseResult&)> GetConfigHandler;
        typedef std::function<void(const ResponseResult&)> GetExecutorsHandler;
        typedef std::function<void(const ResponseResult&)> GetJobHandler;
        typedef std::function<void(const ResponseResult&)> GetJobStatusHandler;
        typedef std::function<void(const ResponseResult&)> StopJobHandler;

        GetClientHandler getClientHandler;
        GetServerHandler getServerHandler;

        GetConfigHandler getConfigHandler;
        GetExecutorsHandler getExecutorsHandler;

        GetJobHandler getJobHandler;
        GetJobStatusHandler getJobStatusHandler;
        StopJobHandler stopJobHandler;

        void setGetClientHandler(const GetClientHandler &getClientHandler) {
            HRG_Protocol::getClientHandler = getClientHandler;
        }
        void setGetServerHandler(const GetServerHandler &getServerHandler) {
            HRG_Protocol::getServerHandler = getServerHandler;
        }
        void setGetConfigHandler(const GetConfigHandler &getConfigHandler) {
            HRG_Protocol::getConfigHandler = getConfigHandler;
        }
        void setGetExecutorsHandler(const GetExecutorsHandler &getExecutorsHandler) {
            HRG_Protocol::getExecutorsHandler = getExecutorsHandler;
        }
        void setGetJobHandler(const GetJobHandler &getJobHandler) {
            HRG_Protocol::getJobHandler = getJobHandler;
        }
        void setGetJobStatusHandler(const GetJobStatusHandler &getJobStatusHandler) {
            HRG_Protocol::getJobStatusHandler = getJobStatusHandler;
        }
        void setStopJobHandler(const StopJobHandler &stopJobHandler) {
            HRG_Protocol::stopJobHandler = stopJobHandler;
        }


        virtual void asyncListen() = 0;

        virtual void asyncToClientReg() = 0;
        virtual void asyncToServerReg(std::string &clientId) = 0;
        virtual void sendConfig() = 0;
        virtual void sendExecutors() = 0;
        virtual void asyncSendJob() = 0;
        virtual void asyncGetJobStatus() = 0;
        virtual void asyncStopJob() = 0;
    };

    class HRG_ProtoV1 : public HRG_Protocol, public std::enable_shared_from_this<HRG_ProtoV1>{
        constexpr static std::size_t MAX_BUF_LENGTH = 1024;
        asio::streambuf data;

        tcp::socket &socket;
        std::shared_ptr<std::ostream> log;

        typedef std::function<void(HRG_Impl::ResponseResult)> downloadedHandler;
    private:
        std::string ip_str() const {
            return socket.remote_endpoint().address().to_string() + ':'
                + std::to_string(socket.remote_endpoint().port());
        }

        void line_handler(const asio::error_code& e, std::size_t size){
            if(!e){
                *log << "Buffer old:" << data.size() << std::endl << std::flush;
                std::istream is(&data);
                std::string line;
                std::getline(is, line);
                *log << "Buffer new:" << data.size() << std::endl << std::flush;

                //TODO: Normal logging
                *log << "[" << ip_str() << "] " << "line: " << line << std::endl << std::flush;


                const std::regex got_client("^I'M CLIENT: (\\w{16})$");
                const std::regex got_server("^I'M SERVER$");
                const std::regex got_config("^CONFIG.XML: ([0-9]+)$");
                const std::regex got_executors("^EXECUTORS.XML: ([0-9]+)$");
                const std::regex get_executors("^GET EXECUTORS$");
                const std::regex got_job("^JOB.XML: ([0-9]+)$");
                const std::regex stop_job("^STOP JOB: ([0-9]+)$");
                const std::regex status_job("^STATUS JOB: ([0-9]+)$");
                const std::regex got_status_job("^STATUSJOB.XML: ([0-9]+)$");

                std::smatch pieces_match;

                if(std::regex_match(line, pieces_match, got_client)){
                    //Got client stage
                    //TODO: Some auth check
                    //There is easy to add some additional check
                    //Password, rsa key, etc...
                    if(getClientHandler && getClientHandler(ResponseResult{RESPONSE::OK, e, ""}, pieces_match[1].str())){
                        asyncOk();
                    }else{
                        asyncNotOk();
                    }
                }else if(std::regex_match(line, pieces_match, got_server)){
                    //Got server
                    if(getServerHandler){
                        getServerHandler(ResponseResult{RESPONSE::OK, e, ""});
                    }else{
                        asyncNotOk();
                    }
                }else if(std::regex_match(line, pieces_match, got_config)){
                    //Got config

                    //TODO: parse config xml
                    size_t length = std::stoul(pieces_match[1].str());
                    auto self(shared_from_this());
                    *log << "Config reading with size " << length << std::endl << std::flush;

                    //std::fstream test("why.txt"); test<<"Because"; test.close();
                    std::shared_ptr<std::fstream> file = std::make_shared<std::fstream>("hello.txt", std::fstream::out);
                    (*file) << "Hi" << std::endl << std::flush;
                    std::shared_ptr<std::iostream> fstream = std::static_pointer_cast<std::iostream>(file);
                    async_download_file(length, fstream, [self, fstream](ResponseResult res){*(self->log)<<"Downloaded"<<std::endl<<std::flush;});
                }else if(std::regex_match(line, pieces_match, get_executors)){
                    //Get executors

                    if(getExecutorsHandler){
                        getExecutorsHandler(ResponseResult{RESPONSE::OK, e, ""});
                    }else{
                        asyncNotOk();
                    }
                }else if(std::regex_match(line, pieces_match, got_executors)){
                    //Got executors

                    //TODO: parse executors xml
                }else if(std::regex_match(line, pieces_match, got_job)){
                    //Got job

                    //TODO: parse job xml
                }else if(std::regex_match(line, pieces_match, stop_job)){

                }else if(std::regex_match(line, pieces_match, status_job)){

                }else if(std::regex_match(line, pieces_match, got_status_job)){
                    //Got status

                    //TODO: parse status job
                }
            }
        }
        void read_line(){
            auto self(shared_from_this());
            using namespace std::placeholders;
            *log << "[" << ip_str() << "]" << " Reading line" << std::endl << std::flush;
            asio::async_read_until(socket, data, "\n", std::bind(&HRG_Impl::HRG_ProtoV1::line_handler, self, _1, _2));
        }


        void async_read_xml(unsigned int bytes){

        }

        void async_download_file(size_t bytes, std::shared_ptr<std::iostream> stream, const downloadedHandler &handler=[](){}){
            async_download_block(bytes, stream, handler);
        }

        void async_download_block(size_t bytes, std::shared_ptr<std::iostream> stream,
                const downloadedHandler &handler=[](){}){
            auto self(shared_from_this());
            asio::async_read(socket, asio::buffer(&data, std::min<size_t>(512, bytes-std::min(bytes, data.size()))),
                    [self, bytes, handler, stream](asio::error_code e, std::size_t size){
               if(!e){
                   *(self->log) << "Downloaded: " << size << std::endl << std::flush;
                   *(self->log) << "Buff size: " << self->data.size() << std::endl << std::flush;
                   *(self->log) << "Bytes: " << bytes << std::endl << std::flush;
                   std::size_t left = bytes - std::min<size_t>(bytes, self->data.size());
                   //(*stream)<<(&(self->data));

                   size_t bytes_needed = std::min(bytes, self->data.size());
                   asio::streambuf::const_buffers_type bufs = self->data.data();
                   std::copy(asio::buffers_begin(bufs),
                             asio::buffers_begin(bufs) + bytes_needed, std::ostream_iterator<char>(*stream));
                   self->data.consume(bytes_needed);
                   *(self->log) << "Buff size2: " << self->data.size() << std::endl << std::flush;

                   if(left > 0){
                       self->async_download_block(left, stream, handler);
                   }else{
                       handler(ResponseResult{RESPONSE::OK, e, ""});
                   }
               }else handler(ResponseResult{RESPONSE::ERR, e, "Error occurred"});
            });
        }

    public:
        HRG_ProtoV1(tcp::socket &socket) : socket(socket), data(MAX_BUF_LENGTH), log(new HRG_Impl::NullStream()) { }
        ~HRG_ProtoV1(){*log<<"Destructor Protocol"<<std::endl<<std::flush;}

        void setLog(std::shared_ptr<std::ostream> &&log);

        void asyncListen(){
            read_line();
        }

        void asyncOk(){
            auto self(shared_from_this());
            asio::async_write(socket, asio::buffer("OK\n\n"),
                              [this, self](std::error_code ec, std::size_t){});
        }

        void asyncNotOk(){
            auto self(shared_from_this());
            asio::async_write(socket, asio::buffer("ERR\n\n"),
                              [this, self](std::error_code ec, std::size_t){});
        }

        void asyncToServerReg(std::string &clientId) {
            auto self(shared_from_this());
            asio::async_write(socket, asio::buffer("REGISTRATION\n\n"),
                              [this, self](std::error_code ec, std::size_t){
                                  if (!ec){
                                      asyncListen();
                                  }
                              });
        }

        void asyncGetJobStatus(){}
        void asyncSendJob(){}
        void asyncStopJob(){}
        void sendConfig(){}
        void sendExecutors(){}
        void asyncToClientReg(){};
    };

    class HRG_AcceptedClient : public std::enable_shared_from_this<HRG_AcceptedClient> {
        HRG_Server &server;
        tcp::socket socket;
        std::shared_ptr<std::ostream> log;


        bool regHandler(const HRG_Impl::ResponseResult &res, const std::string str){
            return true;
        }

    public:
        HRG_AcceptedClient(tcp::socket socket, HRG_Server &server) :
            server(server),
            socket(std::move(socket)),
            log(new HRG_Impl::NullStream()) {
        }
        ~HRG_AcceptedClient(){*log<<"Destructor Accepted Client"<<std::endl<<std::flush;}

        void setLog(std::shared_ptr<std::ostream> &&log);

        void start(){
            std::shared_ptr<HRG_Impl::HRG_ProtoV1> hrg_protocol = std::make_shared<HRG_Impl::HRG_ProtoV1>(socket);

            setLog(std::shared_ptr<std::ostream>(&std::cout, [](std::ostream*){}));
            hrg_protocol->setLog(std::shared_ptr<std::ostream>(&std::cout, [](std::ostream*){}));

            auto self(shared_from_this());
            using namespace std::placeholders;

            hrg_protocol->setGetClientHandler(std::bind(&HRG_AcceptedClient::regHandler, self, _1, _2));
            hrg_protocol->asyncListen();
        }

    };
}

#include "HRG_Server.h"
#endif //HYDRAAPI_EX_HRG_IMPL_H
