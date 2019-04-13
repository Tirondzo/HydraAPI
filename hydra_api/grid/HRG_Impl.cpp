//
// Created by Alex on 22.03.2019.
//

#include "HRG_Impl.h"
const std::string HRG_Impl::HRG_ProtoV1::MSG_DELIM = "\n";

//void HRG_Impl::HRG_AcceptedClient::regHandler(const asio::error_code& e, const RESPONSE r, const std::string str){
//    if(r == RESPONSE::OK){
//        server.srv_mutex_.lock();
//        *log << "REGISTERED" << std::endl << std::flush;
//        /*auto found = std::find_if(server.registered_.begin(), server.registered_.end(),
//                                  [this](const HRG_Executor &i){return i.addr==socket.remote_endpoint();});
//
//        if(found == server.registered_.end()){
//            server.registered_.push_back(HRG_Executor{socket.remote_endpoint(), STATUS::ONLINE});
//        }else{
//            (*found).status = STATUS::ONLINE;
//        }*/
//        server.srv_mutex_.unlock();
//    }
//}

void HRG_Impl::HRG_ProtoV1::setLog(std::shared_ptr<std::ostream> &&log) {
    HRG_ProtoV1::log = std::move(log);
}

void HRG_Impl::HRG_AcceptedClient::setLog(std::shared_ptr<std::ostream> &&log) {
    HRG_AcceptedClient::log = std::move(log);
}
