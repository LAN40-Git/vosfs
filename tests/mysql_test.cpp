#include <mysql/mysql.h>
#include <iostream>

auto main() -> int {
    MYSQL mysql;
    mysql_init(&mysql);

    if (mysql_real_connect(&mysql, "localhost", "vosfs_root", "123456", "vosfs", 0, NULL, 0)) {
        std::cout << "连接成功！" << std::endl;
    } else {
        std::cout << "连接失败！原因：" << mysql_error(&mysql) << std::endl;
    }

    mysql_close(&mysql);
    return 0;
}