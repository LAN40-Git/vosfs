#include "vosfs/common/error.hpp"

auto vosfs::Error::message() const noexcept -> std::string_view {
    switch (error_code_) {
        case kUnknown:
            return "Unknown error.";
        case kCreateSnapshotFailed:
            return "Failed to create snapshot.";
        case kInvalidLogIndex:
            return "Invalid log index.";
        case kProtoSerializeFailed:
            return "Failed to serialize proto.";
        case kProtoParseFailed:
            return "Failed to parse proto.";
        case kTruncateFailed:
            return "Failed to truncate data.";
        case kRecoverFailed:
            return "Failed to recover data.";
        case kRocksDBEngineCreateFailed:
            return "Failed to create rocksdb engine.";
        case kConnectToPeerFailed:
            return "Failed to connect to peer.";
        case kCreateProviderFailed:
            return "Failed to create provider.";
        case kCreatePeerFailed:
            return "Failed to create peer.";
        case kInvalidAddress:
            return "Invalid address.";
        case kConnectToServerFailed:
            return "Failed to connect to server.";
        case kSendRpcRequestFailed:
            return "Failed to send rpc request.";
        case kJsonParseFailed:
            return "Failed to parse json object.";
        case kRepeatedPeer:
            return "Find repeated peer.";
        case kLocalNodeNotFound:
            return "Failed to find local node.";
        case kBindFailed:
            return "Failed to bind to address.";
        case kConnectionShutdown:
            return "Connection shutdown.";
        case kQueueEmpty:
            return "Empty queue.";
        case kQueueShutdown:
            return "Shutdown queue.";
        case kProviderHasShutdown:
            return "The provider has shutdown.";
        case kConsumerShutdown:
            return "Consumer has shutdown.";
        case kConsumerRunning:
            return "Consumer is running.";
        default:
            return strerror(error_code_);
    }
}

auto vosfs::Error::value() const noexcept -> int {
    return error_code_;
}
