#include "vosfs/rpc/session_manager.hpp"

auto vosfs::rpc::detail::SessionManager::assign_session(kosio::net::TcpStream&& stream, kosio::net::SocketAddr addr)
-> std::shared_ptr<Session> {
    auto id = id_++;
    auto new_session = std::make_shared<Session>(id, std::move(stream), addr);
    auto copy_session = new_session;
    sessions_.insert({id, std::move(new_session)});
    return copy_session;
}

auto vosfs::rpc::detail::SessionManager::find_session(uint64_t session_id) const -> std::shared_ptr<Session> {
    SessionMap::accessor acc;
    if (sessions_.find(acc, session_id)) {
        return acc->second;
    }
    return nullptr;
}

void vosfs::rpc::detail::SessionManager::remove_session(uint64_t session_id) {
    sessions_.erase(session_id);
}
