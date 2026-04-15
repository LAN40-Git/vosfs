#include <mysql/mysql.h>
#include <iostream>

auto main() -> int {
    MYSQL mysql;
    mysql_init(&mysql);

    if (mysql_real_connect(&mysql, "local", "root", "123456", "vosfs", 3306, NULL, 0)) {
        std::cout << "MySQL 连接成功！\n" << std::endl;
    } else {
        std::cout << "MySQL 连接失败！\n" << std::endl;
    }

    mysql_close(&mysql);
    return 0;
}
