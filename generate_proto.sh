protoc -I=api/authpb --cpp_out=api/authpb auth.proto
protoc -I=api/raftpb --cpp_out=api/raftpb raft.proto