#include "raft.pb.h"
#include "vosfs/raft/persister.hpp"

auto main() -> int {
    std::filesystem::path path{};
    path /= "/";
    std::cout << path << std::endl;
}