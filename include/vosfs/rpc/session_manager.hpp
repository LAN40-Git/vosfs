#pragma once
#include "vosfs/rpc/invoke_task.hpp"
#include "vosfs/common/util/spsc_queue.hpp"

namespace vosfs::rpc::detail {
struct Session {
    uint64_t                    id;
    kosio::net::TcpStream       stream;
    kosio::net::SocketAddr      addr;
    util::SPSCQueue<InvokeTask> invoke_queue;
};

class SessionManager {
public:
    void assign_session(kosio::net::TcpStream&& stream, kosio::net::SocketAddr addr);

    [[nodiscard]]
    auto find_session(uint64_t session_id) const -> std::shared_ptr<Session>;

    auto remove_session(uint64_t session_id) -> bool;

private:
    using SessionMap = tbb::concurrent_hash_map<uint64_t, std::shared_ptr<Session>>;
    SessionMap sessions_;
    uint64_t   id_{0};

};
} // namespace vosfs::rpc::detail