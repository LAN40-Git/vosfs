#include "vosfs/common/error.hpp"

auto vosfs::Error::message() const noexcept -> std::string_view {
    switch (error_code_) {
        case kUnknown:
            return "Unknown error.";
        case kInvalidAddress:
            return "Invalid address.";
        case kConnectToServerFailed:
            return "Failed to connect to server.";
        case kSendRpcRequestFailed:
            return "Failed to send rpc request.";
        case kParseRpcMessageFailed:
            return "Failed to parse rpc message.";
        case kBindFailed:
            return "Failed to bind to address.";
        case kConnectionShutdown:
            return "Connection shutdown.";
        case kQueueEmpty:
            return "Empty queue.";
        case kQueueShutdown:
            return "Shutdown queue.";
        case kMessageParseFailed:
            return "Failed to parse message.";
        case kMessageSerializeFailed:
            return "Failed to serialize message.";
        case kProviderIsRunning:
            return "The provider is running.";
        case kProviderHasShutdown:
            return "The provider has shutdown.";
        case kConsumerHasShutdown:
            return "The consumer has shutdown.";
        case kStopListeningFailed:
            return "Failed to stop listening.";
        case kNeedRedirect:
            return "Need to redirect to leader.";
        default:
            return strerror(error_code_);
    }
}