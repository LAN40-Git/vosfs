# vosfs

 ![Static Badge](https://img.shields.io/badge/standard-c++23-blue?logo=cplusplus) ![Static Badge](https://img.shields.io/badge/cmake-black?logo=cmake) ![Static Badge](https://img.shields.io/badge/linux-gray?logo=linux)

## About



## Getting started

### 安装

```shell
sudo apt update
# 安装依赖
sudo apt install -y build-essential git cmake pkg-config liburing-dev protobuf-compiler libprotobuf-dev librocksdb-dev libtbb-dev libsqlite3-dev libspdlog-dev libhiredis-dev

# 安装 kosio
git clone git@github.com:LAN40-Git/kosio.git
cd kosio
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install

# 安装 vrpc
git clone git@github.com:LAN40-Git/vrpc.git
cd vrpc
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install

# 安装 vosfs
git clone git@github.com:LAN40-Git/vosfs.git
cd vosfs
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
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



### 



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
