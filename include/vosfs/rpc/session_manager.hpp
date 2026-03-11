#pragma once
#include "vosfs/rpc/invoke_task.hpp"
#include "vosfs/common/util/spsc_queue.hpp"

namespace vosfs::rpc {
class RpcProvider;
} // namespace vosfs::rpc

namespace vosfs::rpc::detail {
struct Session {
    bool                        is_auth{false};
    std::string                 id;
    kosio::net::TcpStream       stream;
    kosio::net::SocketAddr      addr;
    util::SPSCQueue<InvokeTask> invoke_queue;
};

class SessionManager {
    friend class vosfs::rpc::RpcProvider;
public:
    [[nodiscard]]
    auto assign_unauth_session(kosio::net::TcpStream&& stream, kosio::net::SocketAddr addr) -> std::shared_ptr<Session>;

    [[nodiscard]]
    auto assign_auth_session(std::string id, kosio::net::TcpStream&& stream, kosio::net::SocketAddr addr) -> std::shared_ptr<Session>;

    [[nodiscard]]
    auto find_session(bool is_auth, std::string session_id) const -> std::shared_ptr<Session>;

    void remove_session(bool is_auth, std::string session_id);

private:
    using SessionMap = tbb::concurrent_hash_map<std::string, std::shared_ptr<Session>>;
    SessionMap unauth_sessions_;
    SessionMap auth_sessions_;
    uint64_t   id_{0}; // used when assign unauthenticated session
};
} // namespace vosfs::rpc::detail