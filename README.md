# vosfs

 ![Static Badge](https://img.shields.io/badge/standard-c++23-blue?logo=cplusplus) ![Static Badge](https://img.shields.io/badge/cmake-black?logo=cmake) ![Static Badge](https://img.shields.io/badge/linux-gray?logo=linux)

## About



## Getting started

### 安装

安装所需的依赖

```shell
sudo apt update
# 安装依赖
sudo apt install -y build-essential git cmake pkg-config liburing-dev protobuf-compiler libprotobuf-dev librocksdb-dev libtbb-dev libsqlite3-dev libspdlog-dev nlohmann-json3-dev
```

[事件驱动库]-[kosio](https://github.com/LAN40-Git/kosio)，这是一个基于 `io_uring` 和 `c++无栈协程` 实现的事件驱动库，是本人为学习 IO 多路复用技术和现代 C++ 特性复刻自[zedio](https://github.com/8sileus/zedio)，目前仅重构了运行时中的分层时间轮和超时机制，其它模块无大改动

```shell
git clone git@github.com:LAN40-Git/kosio.git
cd kosio
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

[RPC 框架]-[vrpc](https://github.com/LAN40-Git/vrpc)，这是我基于[kosio](https://github.com/LAN40-Git/kosio)开发的轻量 RPC 框架，参考了 `grpc` 的部分实现，暂时不支持 IDL，需要手动写枚举和方法，是从此分布式文件系统中分离出的模块

```shell
git clone git@github.com:LAN40-Git/vrpc.git
cd vrpc
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

[Token库]-[jwt-cpp](https://github.com/Thalhammer/jwt-cpp)，这是一个 JWT token 库，用于用户 API 校验，

```shell
sudo apt install openssl libssl-dev
git clone git@github.com:Thalhammer/jwt-cpp.git
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

[分布式文件系统]-[vosfs](https://github.com/LAN40-Git/vosfs)

```

```

### 部署

#### 认证服务器（可选）

部署 `redis` 服务器

```shell
sudo apt install redis-server
sudo systemctl start redis-server
sudo systemctl enable redis-server
sudo systemctl status redis-server
# 默认端口为 6379
```



## TODO Lists

### vosfs-auth

- [x] 注册
- [x] 登录
- [x] 修改名称
- [x] 修改密码
- [x] 修改权限
- [x] 注销



### Raft 共识服务器

- [x] 选举
- [x] 日志同步
- [x] 快照
- [ ] 文件存储快照数据而非内存
- [ ] 成员变更
- [x] Raft状态持久化
- [x] 日志持久化
- [ ] 状态机
- [ ] 数据分片
- [ ] 控制锁的细粒度



### 客户端

- [ ] 上传文件功能
- [ ] 下载文件功能
- [ ] 删除文件功能
- [ ] 查看文件夹功能
- [ ] 文件预览功能
- [ ] 登录注册功能
