#include "vosfs/ui/auth_client.hpp"

auto main_coro() -> kosio::async::Task<void> {
    auto auth_client = vosfs::ui::AuthClient{"127.0.0.1", 9000};
    while (true) {
        co_await kosio::time::sleep(100);
        co_await auth_client.send_register_user_request("lan", "123456", 1);
        co_await auth_client.send_register_user_request("lan", "123456", 1);
        co_await auth_client.send_login_user_by_name_request("lan", "123456", 1);
        co_await auth_client.send_login_user_by_name_request("lan", "123456", 1);
    }
}

auto main() -> int {
    kosio::runtime::MultiThreadBuilder::default_create().block_on(main_coro());
}