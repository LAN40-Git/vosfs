protoc -I=src/vosfs/api/mathpb src/vosfs/api/mathpb/*.proto --cpp_out=include/vosfs/api/mathpb
mv include/vosfs/api/mathpb/*.cc src/vosfs/api/mathpb