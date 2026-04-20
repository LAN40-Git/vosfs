#pragma once
#include <vrpc/core/type.hpp>

namespace vosfs::auth::detail {
enum class ServiceType : vrpc::Type {
    kUser = 0,
    kMaxService
};

enum class InvokeType : vrpc::Type {
    kRegisterUser,
    kDeleteUser,
    kLoginUserByUserName,
    kLoginUserByEmail,
    kLoginUserByPhone,
    kLogoutUser,
    kUpdateUserName,
    kUpdateUserAvatar,
    kUpdateUserPassword,
    kUpdateUserRole,
    kUpdateUserStatus,
    kMaxMethod
};
} // namespace vosfs::auth::detail