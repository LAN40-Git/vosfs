#pragma once
#include "vosfs/rpc/invoke_task.hpp"
#include "vosfs/common/util/spsc_queue.hpp"

namespace vosfs::rpc {
class RpcProvider;
} // namespace vosfs::rpc

namespace vosfs::rpc::detail {
struct Session {
    bool                        is_authorized;
    uint64_t                    id;
    kosio::net::TcpStream       stream;
    kosio::net::SocketAddr      addr;
    util::SPSCQueue<InvokeTask> invoke_queue;
};

class SessionManager {
    friend class vosfs::rpc::RpcProvider;
public:
    [[nodiscard]]
    auto assign_session(kosio::net::TcpStream&& stream, kosio::net::SocketAddr addr) -> std::shared_ptr<Session>;

    [[nodiscard]]
    auto find_session(uint64_t session_id) const -> std::shared_ptr<Session>;

    void remove_session(uint64_t session_id);

private:
    using SessionMap = tbb::concurrent_hash_map<uint64_t, std::shared_ptr<Session>>;
    SessionMap sessions_;
    uint64_t   id_{0};
};
} // namespace vosfs::rpc::detail