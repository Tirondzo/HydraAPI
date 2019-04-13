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
#include <exception>
#include <string>

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
    ERR,
    MEMLIM
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

        operator bool() const{
            return response == RESPONSE::OK;
        }
    };

    struct Executor{
        std::string client_id;
        std::list<std::string> shared_path;
        bool busy;

        //TODO: Preferences (GPU, CPU, Bandwidth, etc...)

        Executor(const std::string &client_id, const std::list<std::string> &shared_path, bool busy) :
                client_id(client_id), shared_path(shared_path), busy(busy) {}
    };

    enum DOWNLOAD_METHOD{
        DIRECT,
        YANDEX_DISK, //TODO: other methods
        GOOGLE_DRIVE,
        URL,
        ETC
    };
    struct Config{
        std::string scene_id;
        bool allow_to_share;
        std::string download_addr;
        DOWNLOAD_METHOD download_method;

        //TODO: Config preferences (...)

        Config(const std::string &scene_id, bool allow_to_share, DOWNLOAD_METHOD method, const std::string &download) :
                scene_id(scene_id), allow_to_share(allow_to_share), download_method(method), download_addr(download) {}
    };

    enum JOB_STATUS{
        CREATED,
        DECLINED,
        PAUSED,
        DOWNLOADING,
        RENDERING,
        UPLOADING,
        DONE,
        ERR
    };
    static const std::array<std::string, JOB_STATUS::DONE+1> JOB_STATUS_STR={
            "CREATED",
            "DECLINED",
            "PAUSED",
            "DOWNLOADING",
            "RENDERING",
            "UPLOADING",
            "DONE"
    };
    struct Job{
        std::string job_id;
        std::string clt_id;
        std::string exe_id;
        std::string scn_id;
        JOB_STATUS status;
        std::pair<int, int> progress;

        //TODO: Samples details (samples, ...)

        Job(const std::string &job_id, const std::string &clt_id,
                const std::string &exe_id, const std::string &scn_id,
                JOB_STATUS status, std::pair<int,int> progress = std::pair<int,int>(0,0)) :
                job_id(job_id), clt_id(clt_id), exe_id(exe_id), scn_id(scn_id), status(status), progress(progress) {}
    };

    class HRG_Protocol{
    public:
        using Executors = std::vector<Executor>;

        //Requests:
        typedef std::function<bool(const ResponseResult&, const std::string)> GetClientHandler;
        typedef std::function<std::string(const ResponseResult&)> GetServerHandler;
        typedef std::function<bool(const ResponseResult&, const Config)> GotConfigHandler;
        typedef std::function<Executors(const ResponseResult&)> GetExecutorsHandler;
        typedef std::function<Job(const ResponseResult&, const Job)> GotJobHandler;
        typedef std::function<Job(const ResponseResult&, const std::string&)> GetJobStatusHandler;
        typedef std::function<Job(const ResponseResult&, const std::string&)> StopJobHandler;

        GetClientHandler getClientHandler;
        GetServerHandler getServerHandler;

        GotConfigHandler gotConfigHandler;
        GetExecutorsHandler getExecutorsHandler;

        GotJobHandler gotJobHandler;
        GetJobStatusHandler getJobStatusHandler;
        StopJobHandler stopJobHandler;

        //Responses:
        typedef std::function<void(const ResponseResult&)> ToClientResponse;
        typedef std::function<void(const ResponseResult&)> ToServerResponse;
        typedef std::function<void(const ResponseResult&)> SendConfigResponse;
        typedef std::function<void(const ResponseResult&, const std::vector<Executor>)> GetExecutorsResponse;
        typedef std::function<void(const ResponseResult&, const Job)> SendJobResponse;
        typedef std::function<void(const ResponseResult&, const Job)> GetJobStatusResponse;
        typedef std::function<void(const ResponseResult&, const Job)> StopJobResponse;


        void setGetClientHandler(const GetClientHandler &getClientHandler) {
            HRG_Protocol::getClientHandler = getClientHandler;
        }
        void setGetServerHandler(const GetServerHandler &getServerHandler) {
            HRG_Protocol::getServerHandler = getServerHandler;
        }
        void setGotConfigHandler(const GotConfigHandler &gotConfigHandler) {
            HRG_Protocol::gotConfigHandler = gotConfigHandler;
        }
        void setGetExecutorsHandler(const GetExecutorsHandler &getExecutorsHandler) {
            HRG_Protocol::getExecutorsHandler = getExecutorsHandler;
        }
        void setGotJobHandler(const GotJobHandler &getJobHandler) {
            HRG_Protocol::gotJobHandler = getJobHandler;
        }
        void setGetJobStatusHandler(const GetJobStatusHandler &getJobStatusHandler) {
            HRG_Protocol::getJobStatusHandler = getJobStatusHandler;
        }
        void setStopJobHandler(const StopJobHandler &stopJobHandler) {
            HRG_Protocol::stopJobHandler = stopJobHandler;
        }


        virtual void asyncListen() = 0;

        virtual void asyncToClientReg(ToClientResponse response) = 0;
        virtual void asyncToServerReg(const std::string &clientId, ToServerResponse response) = 0;
        virtual void asyncSendConfig(const Config &config, SendConfigResponse response) = 0;
        virtual void asyncGetExecutors(const std::vector<Executor> &executors, GetExecutorsResponse response) = 0;
        virtual void asyncSendJob(const Job &job, SendJobResponse response) = 0;
        virtual void asyncGetJobStatus(const std::string &job_id, GetJoыbStatusResponse response) = 0;
        virtual void asyncStopJob(const std::string &job_id, StopJobResponse response) = 0;
    };
    class HRG_ProtoV1 : public HRG_Protocol, public std::enable_shared_from_this<HRG_ProtoV1>{
        constexpr static std::size_t MAX_BUF_LENGTH = 1024;
        asio::streambuf data;

        tcp::socket &socket;
        std::shared_ptr<std::ostream> log;

        typedef std::function<void(HRG_Impl::ResponseResult)> usualHandler;
        typedef std::function<void(HRG_Impl::ResponseResult)> downloadedHandler;
        typedef std::function<void(HRG_Impl::ResponseResult, pugi::xml_parse_result, std::shared_ptr<pugi::xml_document>)> readedXmlHandler;
        typedef std::function<void(HRG_Impl::ResponseResult, const std::string line)> LineHandler;
    private:
        std::string ip_str() const {
            return socket.remote_endpoint().address().to_string() + ':'
                + std::to_string(socket.remote_endpoint().port());
        }

        static const std::string MSG_DELIM;

        //XML Parsing functions
        Config parseConfig(pugi::xml_document &doc){
            pugi::xml_node root = doc.document_element();
            std::string scene_id = pugi::as_utf8(root.child_value(L"scene_id"));
            pugi::xml_node download = root.child(L"download");
            std::string download_addr = pugi::as_utf8(download.value());
            std::string download_method = pugi::as_utf8(download.attribute(L"method").value());

            bool allow_share = download.child(L"allow_share").text().as_bool(true);

            DOWNLOAD_METHOD method = DIRECT;
            //TODO: other methods parsing

            return Config(scene_id, allow_share, method, download_addr);
        }
        std::vector<Executor> parseExecutors(pugi::xml_document &doc){
            pugi::xml_node root = doc.document_element();
            constexpr pugi::char_t EXEC[] = L"Executor";
            std::vector<Executor> executors{};
            for (pugi::xml_node exec = root.child(EXEC); exec; exec = exec.next_sibling(EXEC)){
                std::string client_id = pugi::as_utf8(exec.child_value(L"client_id"));
                //TODO: shared path
                std::list<std::string> shared_path;
                bool is_busy = exec.child(L"is_busy").text().as_bool(false);
                executors.push_back(Executor(client_id, shared_path, is_busy));
            }
        }
        Job parseJob(pugi::xml_document &doc){
            pugi::xml_node root = doc.document_element();
            std::string job_id = pugi::as_utf8(root.child_value(L"job_id"));
            std::string client_id = pugi::as_utf8(root.child_value(L"client_id"));
            std::string exec_id = pugi::as_utf8(root.child_value(L"exec_id"));
            std::string scene_id = pugi::as_utf8(root.child_value(L"scene_id"));

            JOB_STATUS stat = JOB_STATUS::CREATED;
            std::string status = pugi::as_utf8(root.child_value(L"status"));
            for(int i = 0; i < JOB_STATUS_STR.size(); i++){
                if(status == JOB_STATUS_STR[i]){
                    stat = JOB_STATUS (i);
                    break;
                }
            }

            pugi::xml_node progress = root.child(L"progress");
            int val = progress.attribute(L"val").as_int(0);
            int max = progress.attribute(L"max").as_int(0);

            return Job(job_id, client_id, exec_id, scene_id, stat);
        }

        //XML Serialization functions
        pugi::xml_node serializeConfig(Config config){
            pugi::xml_node node;
            node.set_name(L"Config");
            const std::string test = "ss";
            node.append_child(L"scene_id").set_value(pugi::as_wide(config.scene_id).c_str());
            pugi::xml_node download = node.append_child(L"download");
            download.append_attribute(L"method").set_value(L"DIRECT");
            download.set_value(pugi::as_wide(config.download_addr).c_str());

            node.append_child(L"allow_share").text().set(config.allow_to_share);

            return node;
        }
        pugi::xml_node serializeExecutors(Executors executors){
            pugi::xml_node node;
            node.set_name(L"Executors");
            for(Executors::iterator i = executors.begin(); i != executors.end(); i++){
                pugi::xml_node exec = node.append_child(L"Executor");
                exec.append_child(L"client_id").set_value(pugi::as_wide((*i).client_id).c_str());
                exec.append_child(L"is_busy").text().set((*i).busy);
                //TODO: shared path
            }
            return node;
        }
        pugi::xml_node serializeJob(Job job){
            pugi::xml_node node;
            node.set_name(L"Job");
            node.append_child(L"job_id").set_value(pugi::as_wide(job.job_id).c_str());
            node.append_child(L"client_id").set_value(pugi::as_wide(job.clt_id).c_str());
            node.append_child(L"exec_id").set_value(pugi::as_wide(job.exe_id).c_str());
            node.append_child(L"scene_id").set_value(pugi::as_wide(job.scn_id).c_str());

            node.append_child(L"status").set_value(pugi::as_wide(JOB_STATUS_STR[job.status]).c_str());
            pugi::xml_node progress = node.append_child(L"progress");
            progress.append_attribute(L"val").set_value(job.progress.first);
            progress.append_attribute(L"max").set_value(job.progress.second);

            return node;
        }

        void processReadXml(size_t bytes, size_t limit, readedXmlHandler handler){
            auto self(shared_from_this());
            if(bytes > limit) asyncNotOk();
            else{
                asyncOk([self, bytes, handler](ResponseResult res){
                   if(res){
                       self->async_read_xml(bytes, handler);
                   }
                });
            }
        }


        void line_handler(ResponseResult responseResult, const std::string line){
            if(responseResult.response == RESPONSE::OK){
                asio::error_code &e = responseResult.error_code;

                //TODO: Normal logging
                *log << "[" << ip_str() << "] " << "line: " << line << std::endl << std::flush;


                const std::regex got_client("^I'M CLIENT: (\\w{16})$");
                const std::regex got_server("^I'M SERVER$");
                const std::regex got_config("^CONFIG.XML: ([0-9]+)$");
                //const std::regex got_executors("^EXECUTORS.XML: ([0-9]+)$");
                const std::regex get_executors("^GET EXECUTORS$");
                const std::regex got_job("^JOB.XML: ([0-9]+)$");
                const std::regex stop_job("^STOP JOB: ([0-9]+)$");
                const std::regex status_job("^STATUS JOB: ([0-9]+)$");
                //const std::regex got_status_job("^STATUSJOB.XML: ([0-9]+)$");

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
                        std::string client_id = getServerHandler(ResponseResult{RESPONSE::OK, e, ""});
                        if(client_id.size()) { //if accepted
                            asyncToServerReg(client_id);
                        }else{ asyncNotOk(); }
                    }else{
                        asyncNotOk();
                    }
                }else if(std::regex_match(line, pieces_match, got_config)){
                    //Got config
                    size_t length = std::stoul(pieces_match[1].str());
                    auto self(shared_from_this());
                    processReadXml(length, 4096, [self](ResponseResult res,
                                                       pugi::xml_parse_result prs, std::shared_ptr<pugi::xml_document> doc){
                        if(res && self->gotConfigHandler && self->gotConfigHandler(res, self->parseConfig(*doc))){
                            self->asyncOk();
                        }else{
                            self->asyncNotOk();
                        }
                    });
                }else if(std::regex_match(line, pieces_match, get_executors)){
                    //Get executors
                    if(getExecutorsHandler){
                        Executors executors = getExecutorsHandler(ResponseResult{RESPONSE::OK, e, ""});
                        asyncSendExecutors(executors);
                    }else{
                        asyncNotOk();
                    }
                }else if(std::regex_match(line, pieces_match, got_job)){
                    //Got job
                    size_t length = std::stoul(pieces_match[1].str());
                    auto self(shared_from_this());
                    processReadXml(length, 4096, [self](ResponseResult res,
                                                        pugi::xml_parse_result prs, std::shared_ptr<pugi::xml_document> doc){
                        if(res && self->gotJobHandler){
                            //TODO: Aware recursion
                            Job job = self->gotJobHandler(res, self->parseJob(*doc));
                            self->asyncSendJob(job);
                        }
                    });
                }else if(std::regex_match(line, pieces_match, stop_job)){
                    if(stopJobHandler){
                        Job job = stopJobHandler(ResponseResult{RESPONSE::OK, e, ""}, pieces_match[1].str());
                        asyncSendJob(job);
                    }else{
                        asyncNotOk();
                    }
                }else if(std::regex_match(line, pieces_match, status_job)){
                    if(getJobStatusHandler){
                        Job job = getJobStatusHandler(ResponseResult{RESPONSE::OK, e, ""}, pieces_match[1].str());
                        asyncSendJob(job);
                    }else{
                        asyncNotOk();
                    }
                }
            }
        }

        inline void async_read_line(){
            auto self(shared_from_this());
            using namespace std::placeholders;
            LineHandler handler = std::bind(&HRG_Impl::HRG_ProtoV1::line_handler, self, _1, _2);
            async_read_line(handler);
        }
        void async_read_line(LineHandler handler){
            auto self(shared_from_this());
            *log << "[" << ip_str() << "]" << " Reading line" << std::endl << std::flush;
            asio::async_read_until(socket, data, MSG_DELIM, [self, handler](asio::error_code e, size_t size){
                if(!e) {
                    *(self->log) << "Buffer old:" << self->data.size() << std::endl << std::flush;
                    std::istream is(&(self->data));
                    std::string line;
                    std::getline(is, line);
                    *(self->log) << "Buffer new:" << self->data.size() << std::endl << std::flush;
                    handler(ResponseResult{RESPONSE::OK,e,""}, line);
                }else{
                    handler(ResponseResult{RESPONSE::ERR,e,""}, "");
                }
            });
        }


        void async_read_xml(size_t bytes, const readedXmlHandler handler){
            //if(bytes > 4096) throw std::invalid_argument("Memory size limit. 'bytes' must be <= 4096");
            std::shared_ptr<asio::streambuf> filebuf = std::make_shared<asio::streambuf>(bytes);
            std::shared_ptr<std::iostream> stream = std::make_shared<std::iostream>(&*filebuf);
            auto self(shared_from_this());
            async_download_block(bytes, stream, [self, stream, filebuf, bytes, handler](ResponseResult res){
                std::shared_ptr<pugi::xml_document> doc = std::make_shared<pugi::xml_document>();
                pugi::xml_parse_result prs = doc->load(*stream);
                if(filebuf->max_size() < bytes) res.response = RESPONSE::MEMLIM; //memory limit
                handler(res, prs, doc);
            });
        }

        void async_download_file(size_t bytes, std::shared_ptr<std::ostream> stream, const downloadedHandler handler=[](){}){
            async_download_block(bytes, stream, handler);
        }

        void async_download_block(size_t bytes, std::shared_ptr<std::ostream> stream,
                const downloadedHandler handler=[](){}){
            auto self(shared_from_this());
            *log << std::to_string(std::min<size_t>(512, bytes-std::min<size_t>(bytes, data.size()))) << std::endl << std::flush;
            *(self->log) << "Buff size: " << self->data.size() << std::endl << std::flush;
            *(self->log) << "Bytes: " << bytes << std::endl << std::flush;
            asio::async_read(socket, data.prepare(std::min<size_t>(512, bytes-std::min<size_t>(bytes, data.size()))),
                    [self, bytes, handler, stream](asio::error_code e, std::size_t size){
               if(!e){
                   self->data.commit(size);
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

        void asyncSendExecutors(Executors executors, usualHandler response){
            auto self(shared_from_this());
            //std::shared_ptr<pugi::xml_document> doc = std::make_shared<pugi::xml_document>();
            pugi::xml_document doc;
            doc.append_move(serializeExecutors(executors));
            std::shared_ptr<asio::streambuf> buff = std::make_shared<asio::streambuf>(4096);
            std::ostream os(&(*buff));
            doc.save(os);
            if(os.fail()){
                response(ResponseResult{RESPONSE::ERR, asio::error::no_memory, "Serialized xml is too big or smth else"});
            }else{
                asio::async_write(socket, asio::buffer("EXECUTORS.XML: "+std::to_string(buff->size())+MSG_DELIM),
                                  [self, response, buff](asio::error_code ec, size_t size){
                                      if(ec){
                                          response(ResponseResult{RESPONSE::ERR, ec, "Error on sending headers"});
                                          return;
                                      }
                                      self->async_read_line([self, response, buff](ResponseResult res, std::string line){
                                          if(line!="OK"){
                                              res.response=RESPONSE::ERR;
                                              res.error_message="Executors uploading declined";
                                              response(res);
                                              return;
                                          }
                                          asio::async_write(self->socket, *buff, [self, response](asio::error_code ec2, size_t size){
                                              if(ec2){
                                                  response(ResponseResult{RESPONSE::ERR, ec2, "Error on transferring"});
                                                  return;
                                              }
                                          });
                                      });
                                  });
            }
        }


        void asyncListenJob(GotJobHandler handler){
            if(!handler)
                return;

            auto self(shared_from_this());
            async_read_line([self, handler](ResponseResult res, std::string line){
                const std::regex got_job("^JOB.XML: ([0-9]+)$");
                std::smatch pieces_match;
                if(std::regex_match(line, pieces_match, got_job)){
                    size_t length = std::stoul(pieces_match[1].str());
                    self->processReadXml(length, 4096, [self, handler](ResponseResult res,
                                                        pugi::xml_parse_result prs, std::shared_ptr<pugi::xml_document> doc){
                        if(res){
                            handler(res, self->parseJob(*doc));
                        }else{
                            handler(res, Job("","","","",JOB_STATUS::ERR));
                        }
                    });
                }else handler(res, Job("","","","",JOB_STATUS::ERR));
            });
        }

        void asyncListenExecutors(GetExecutorsResponse handler){
            if(!handler)
                return;

            auto self(shared_from_this());
            async_read_line([self, handler](ResponseResult res, std::string line){
                const std::regex got_executors("^EXECUTORS.XML: ([0-9]+)$");
                std::smatch pieces_match;
                if(std::regex_match(line, pieces_match, got_executors)){
                    size_t length = std::stoul(pieces_match[1].str());
                    self->processReadXml(length, 4096, [self, handler](ResponseResult res,
                                                                       pugi::xml_parse_result prs, std::shared_ptr<pugi::xml_document> doc){
                        if(res){
                            handler(res, self->parseExecutors(*doc));
                        }else{
                            handler(res, Executors{});
                        }
                    });
                }else handler(res, Executors{});
            });
        }

    public:
        HRG_ProtoV1(tcp::socket &socket) : socket(socket), data(MAX_BUF_LENGTH), log(new HRG_Impl::NullStream()) { }
        ~HRG_ProtoV1(){*log<<"Destructor Protocol"<<std::endl<<std::flush;}

        void setLog(std::shared_ptr<std::ostream> &&log);

        void asyncListen(){
            async_read_line();
        }

        void asyncOk(const usualHandler handler=[](ResponseResult res){}){
            auto self(shared_from_this());
            asio::async_write(socket, asio::buffer("OK"+MSG_DELIM),
                    [this, self, handler](std::error_code ec, std::size_t){
                handler(ResponseResult{!ec ? RESPONSE::OK : RESPONSE::ERR, ec, ""});
            });
        }

        void asyncNotOk(const usualHandler handler=[](ResponseResult res){}){
            auto self(shared_from_this());
            asio::async_write(socket, asio::buffer("ERR"+MSG_DELIM),
                              [this, self, handler](std::error_code ec, std::size_t){
                handler(ResponseResult{!ec ? RESPONSE::OK : RESPONSE::ERR, ec, ""});
            });
        }

        void asyncToClientReg(ToClientResponse response){
            auto self(shared_from_this());
            asio::async_write(socket, asio::buffer("I'M SERVER"+MSG_DELIM),
                    [self, response](std::error_code ec, std::size_t size){
                        if(!ec) {
                            response(ResponseResult{RESPONSE::OK, ec, ""});
                        }else{
                            response(ResponseResult{RESPONSE::ERR, ec, ""});
                        }
            });
        }

        void asyncToServerReg(const std::string &clientId, ToServerResponse response) {
            auto self(shared_from_this());
            asio::async_write(socket, asio::buffer("I'M CLIENT: "+clientId+MSG_DELIM),
                              [self, response](std::error_code ec, std::size_t size){
                                  if (!ec){
                                      self->async_read_line([self, response](ResponseResult res, std::string line){
                                          if(line=="OK"){
                                              res.response=RESPONSE::OK;
                                              response(res);
                                          }else{
                                              res.response=RESPONSE::ERR;
                                              res.error_message="Not logged in";
                                              response(res);
                                          }
                                      });
                                  }else{
                                      response(ResponseResult{RESPONSE::ERR, ec, ""});
                                  }
                              });
        }

        void asyncSendConfig(const Config &config, SendConfigResponse response){
            auto self(shared_from_this());
            //std::shared_ptr<pugi::xml_document> doc = std::make_shared<pugi::xml_document>();
            pugi::xml_document doc;
            doc.append_move(serializeConfig(config));
            std::shared_ptr<asio::streambuf> buff = std::make_shared<asio::streambuf>(4096);
            std::ostream os(&(*buff));
            doc.save(os);
            if(os.fail()){
               response(ResponseResult{RESPONSE::ERR, asio::error::no_memory, "Serialized xml is too big or smth else"});
            }else{
                asio::async_write(socket, asio::buffer("CONFIG.XML: "+std::to_string(buff->size())+MSG_DELIM),
                        [self, response, buff](asio::error_code ec, size_t size){
                            if(ec){
                                response(ResponseResult{RESPONSE::ERR, ec, "Error on sending headers"});
                                return;
                            }
                            self->async_read_line([self, response, buff](ResponseResult res, std::string line){
                                if(line!="OK"){
                                    res.response=RESPONSE::ERR;
                                    res.error_message="Config uploading declined";
                                    response(res);
                                    return;
                                }
                                asio::async_write(self->socket, *buff, [self, response](asio::error_code ec2, size_t size){
                                    if(ec2){
                                        response(ResponseResult{RESPONSE::ERR, ec2, "Error on transferring"});
                                        return;
                                    }
                                    self->async_read_line([self, response](ResponseResult res2, std::string line2){
                                        if(line2!="OK"){
                                            res2.response=RESPONSE::ERR;
                                            res2.error_message="Config declined";
                                            response(res2);
                                            return;
                                        }
                                        res2.response=RESPONSE::OK;
                                        response(res2);
                                    });
                                });
                            });
                        });
            }
        }

        void asyncSendJob(const Job &job, SendJobResponse response){
            auto self(shared_from_this());
            //std::shared_ptr<pugi::xml_document> doc = std::make_shared<pugi::xml_document>();
            pugi::xml_document doc;
            doc.append_move(serializeJob(job));
            std::shared_ptr<asio::streambuf> buff = std::make_shared<asio::streambuf>(4096);
            std::ostream os(&(*buff));
            doc.save(os);
            if(os.fail()){
                response(ResponseResult{RESPONSE::ERR, asio::error::no_memory, "Serialized xml is too big or smth else"}, job);
            }else{
                asio::async_write(socket, asio::buffer("JOB.XML: "+std::to_string(buff->size())+MSG_DELIM),
                        [self, response, buff](asio::error_code ec, size_t size){
                            if(ec){
                                response(ResponseResult{RESPONSE::ERR, ec, ""}, Job("","","","",JOB_STATUS::ERR));
                                return;
                            }
                            self->asyncListenJob([self, response](ResponseResult res, Job job){
                                response(res, job);
                            });

                });
            }
        }

        void asyncGetExecutors(const std::vector<Executor> &executors, GetExecutorsResponse response){
            auto self(shared_from_this());
            asio::async_write(socket, asio::buffer("GET EXECUTORS"+MSG_DELIM),
                    [self, response](asio::error_code ec, size_t size){
                        if(ec){
                            response(ResponseResult{RESPONSE::ERR, ec, ""}, Executors{});
                            return;
                        }
                        self->asyncListenExecutors(response);
            });
        }

        void asyncGetJobStatus(const std::string &job_id, GetJobStatusResponse response){
            auto self(shared_from_this());
            asio::async_write(socket, asio::buffer("STATUS JOB: "+job_id+MSG_DELIM),
                              [self, response](asio::error_code ec, size_t size){
                                  if(ec){
                                      response(ResponseResult{RESPONSE::ERR, ec, ""}, Job("","","","",JOB_STATUS::ERR));
                                      return;
                                  }
                                  self->asyncListenJob([self, response](ResponseResult res, Job job){
                                      response(res, job);
                                  });
                              });
        }

        void asyncStopJob(const std::string &job_id, StopJobResponse response){
            auto self(shared_from_this());
            asio::async_write(socket, asio::buffer("STOP JOB: "+job_id+MSG_DELIM),
                              [self, response](asio::error_code ec, size_t size){
                                  if(ec){
                                      response(ResponseResult{RESPONSE::ERR, ec, ""}, Job("","","","",JOB_STATUS::ERR));
                                      return;
                                  }
                                  self->asyncListenJob([self, response](ResponseResult res, Job job){
                                      response(res, job);
                                  });
                              });
        }
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
