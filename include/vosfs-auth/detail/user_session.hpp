#pragma once
#include "authpb/auth.pb.h"

namespace vosfs::auth::detail {
struct UserSession {
    std::string token;
    int64_t     uid;
    User_Role   role;
    uint64_t    quota;
    std::string user_name;
    std::string avatar;
    std::string email;
    std::string phone;
    std::string create_time;

    void reset() {
        token.clear();
        user_name.clear();
        avatar.clear();
        email.clear();
        phone.clear();
        create_time.clear();
    }
};
} // namespace vosfs::auth::detail