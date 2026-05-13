#include "vosfs/storage/node.hpp"

auto vosfs::storage::DataNode::wait() -> kosio::async::Task<void> {
    auto rpc_server = vrpc::TcpServer{host_, port_};
    co_await rpc_server
        // .register_method<RegisterUserRequest, RegisterUserResponse>(
        //     "user", "register",
        //     [this](const RegisterUserRequest& request) -> kosio::async::Task<RegisterUserResponse> {
        //         co_return co_await this->handle_register_user_request(request);
        //     })
        // .register_method<DeleteUserRequest, DeleteUserResponse>(
        //     "user", "delete",
        //     [this](const DeleteUserRequest& request) -> kosio::async::Task<DeleteUserResponse> {
        //         co_return co_await this->handle_delete_user_request(request);
        //     })
        // .register_method<LoginUserByNameRequest, LoginUserByNameResponse>(
        //     "user", "loginbyname",
        //     [this](const LoginUserByNameRequest& request) -> kosio::async::Task<LoginUserByNameResponse> {
        //         co_return co_await this->handle_login_user_by_user_name_request(request);
        //     })
        .wait();
}
