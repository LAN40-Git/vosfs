#include "vosfs/rpc/session_manager.hpp"

void vosfs::rpc::detail::SessionManager::assign_session(kosio::net::TcpStream&& stream, kosio::net::SocketAddr addr) {
    auto id = id_++;
    auto new_session = std::make_shared<Session>(id, std::move(stream), addr);
    sessions_.insert({id, std::move(new_session)});
}

auto vosfs::rpc::detail::SessionManager::find_session(uint64_t session_id) const -> std::shared_ptr<Session> {
    SessionMap::accessor acc;
    if (sessions_.find(acc, session_id)) {
        return acc->second;
    }
    return nullptr;
}

auto vosfs::rpc::detail::SessionManager::remove_session(uint64_t session_id) -> bool {
    return sessions_.erase(session_id);
}
