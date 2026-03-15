protoc -I=src/vosfs/api/mathpb src/vosfs/api/mathpb/*.proto --cpp_out=include/vosfs/api/mathpb
mv include/vosfs/api/mathpb/*.cc src/vosfs/api/mathpb

protoc -I=src/vosfs/api/serverpb src/vosfs/api/serverpb/*.proto --cpp_out=include/vosfs/api/serverpb
mv include/vosfs/api/serverpb/*.cc src/vosfs/api/serverpb