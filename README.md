# vosfs

 ![Static Badge](https://img.shields.io/badge/c++-23-blue?logo=cplusplus) ![Static Badge](https://img.shields.io/badge/cmake-black?logo=cmake) ![Static Badge](https://img.shields.io/badge/linux-gray?logo=linux)

## About



## Getting started

### 安装

Tips：`vosfs` 只支持 `linux` 系统

```shell
sudo apt update
# 安装依赖
sudo apt install -y build-essential git cmake pkg-config liburing-dev protobuf-compiler libprotobuf-dev librocksdb-dev libtbb-dev libxxhash-dev libsqlite3-dev
# 安装 kosio
git clone git@github.com:LAN40-Git/kosio.git
cd kosio
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



## TODO Lists

### 异步 IO 框架（协程调度器）

- [x] 超时机制
- [x] 同步机制
- [x] 信号机制
- [x] 文件 IO
- [x] 网络 IO
- [x] 异步日志
- [x] 性能测试



### RPC 服务

- [x] 服务端与客户端间的RPC通信
- [ ] 服务端流量限制，客户端数量限制
- [ ] TLS加密
- [ ] IDL
- [ ] 性能测试



### 认证服务器

- [x] 注册
- [x] 登录
- [x] 修改名称
- [x] 修改密码
- [x] 修改权限
- [x] 注销



### 数据服务器



### Raft 共识服务器

- [x] 选举
- [x] 日志同步
- [x] 快照
- [ ] 成员变更
- [x] Raft状态持久化
- [x] 日志持久化
- [ ] 状态机
- [ ] 数据分片



### 客户端

- [ ] 上传文件功能
- [ ] 下载文件功能
- [ ] 删除文件功能
- [ ] 查看文件夹功能
- [ ] 文件预览功能
- [ ] 登录注册功能
