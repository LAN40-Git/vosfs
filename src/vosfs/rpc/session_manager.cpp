#include "vosfs/rpc/session_manager.hpp"

auto vosfs::rpc::detail::SessionManager::assign_unauth_session(kosio::net::TcpStream&& stream, kosio::net::SocketAddr addr)
-> std::shared_ptr<Session> {
    auto id = std::to_string(id_++);
    auto new_session = std::make_shared<Session>(id, std::move(stream), addr);
    auto copy_session = new_session;
    unauth_sessions_.insert({id, std::move(new_session)});
    return copy_session;
}

auto vosfs::rpc::detail::SessionManager::assign_auth_session(std::string id, kosio::net::TcpStream&& stream,
    kosio::net::SocketAddr addr) -> std::shared_ptr<Session> {
    auto new_session = std::make_shared<Session>(id, std::move(stream), addr);
    auto copy_session = new_session;
    auth_sessions_.insert({id, std::move(new_session)});
    return copy_session;
}

auto vosfs::rpc::detail::SessionManager::find_session(bool is_auth, std::string session_id) const -> std::shared_ptr<Session> {
    SessionMap::accessor acc;
    if (is_auth && auth_sessions_.find(acc, session_id)) {
        return acc->second;
    }
    if (!is_auth && unauth_sessions_.find(acc, session_id)) {
        return acc->second;
    }
    return nullptr;
}

void vosfs::rpc::detail::SessionManager::remove_session(bool is_auth, std::string session_id) {
    if (is_auth) {
        auth_sessions_.erase(session_id);
    } else {
        unauth_sessions_.erase(session_id);
    }
}
