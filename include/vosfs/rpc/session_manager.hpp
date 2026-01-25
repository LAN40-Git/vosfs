#pragma once
#include "vosfs/rpc/invoke_task.hpp"

namespace vosfs::rpc::detail {
struct Session {
    uint64_t               id;
    kosio::net::TcpStream  stream;
    kosio::net::SocketAddr addr;
};

class SessionManager {

};
} // namespace vosfs::rpc::detail