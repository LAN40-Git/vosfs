protoc -I=src/vosfs/rpc/pb src/vosfs/rpc/pb/*.proto --cpp_out=include/vosfs/rpc/pb
mv include/vosfs/rpc/pb/*.cc src/vosfs/rpc/pb