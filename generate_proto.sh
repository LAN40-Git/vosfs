protoc -I=src/vosfs/api/serverpb src/vosfs/api/serverpb/*.proto --cpp_out=include/vosfs/api/serverpb
mv include/vosfs/api/serverpb/*.cc src/vosfs/api/serverpb

protoc -I=api/authpb --cpp_out=api/authpb auth.proto