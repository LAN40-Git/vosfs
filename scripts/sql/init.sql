CREATE DATABASE IF NOT EXISTS vosfs;
USE vosfs;

CREATE TABLE `user` (
    `uid` BIGINT UNSIGNED PRIMARY KEY,          -- uint64 uid
    `name` VARCHAR(64) NOT NULL UNIQUE,         -- 用户名（唯一）
    `hashed_password` VARCHAR(256) NOT NULL,    -- 哈希密码
    `role` TINYINT NOT NULL,                    -- 角色 0=Admin 1=User
    `status` TINYINT NOT NULL,                  -- 状态 0=启用 1=禁用
    `create_time` BIGINT UNSIGNED NOT NULL,     -- 创建时间戳
    `modify_time` BIGINT UNSIGNED NOT NULL,     -- 修改时间戳
    `last_login_time` BIGINT UNSIGNED DEFAULT 0,-- 最后登录时间
    `last_login_ip` VARCHAR(64) DEFAULT '',     -- 最后登录IP
    UNIQUE KEY `uk_name` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;