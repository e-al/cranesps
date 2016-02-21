//
// Created by e-al on 28.08.15.
//

#ifndef DFILE_TYPES_H
#define DFILE_TYPES_H

#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <vector>
#include <chrono>
#include <list>
#include <boost/lockfree/queue.hpp>

namespace google {
    namespace protobuf {
        class Message;
    }
}

namespace zmq {
    class socket_t;
}

namespace dmember {
    class WrappedMessage;
    class HeartBeat;
    class PingRequest;
    class ConnectionUpdateRequest;
    class PingAck;
    class UpdateAck;
    class JoinRequest;
    class JoinAck;
}

namespace dfile {
    class WrappedDFileMessage;
    class ServiceRequest;
    class ServiceRequest_PutRequest;
    class ServiceRequest_GetRequest;
    class ServiceRequest_DeleteRequest;
    class ServiceRequest_UpdateRequest;
    class ServiceRequest_ListRequest;
    class ServiceReply;
    class ServiceReply_PutReply;
    class ServiceReply_GetReply;
    class ServiceReply_DeleteReply;
    class ServiceReply_UpdateReply;
    class ServiceReply_ListReply;
    class SendRequest;
    class SendReply;
    class FetchRequest;
    class FetchReply;
    class CommandRequest;
    class CommandRequest_ReplicateCommand;
    class CommandRequest_ListCommand;
    class CommandRequest_DeleteCommand;
    class CommandReply;
    class CommandReply_ReplicateReply;
    class CommandReply_ListReply;
    class CommandReply_DeleteReply;
}

namespace crane{
    // Top level message
    class CraneWrappedMessage;
    // Client -> Nimbus and Nimbus -> Client
    class CraneClientRequest;
    class CraneClientReply;
    // Client -> Nimbus submessages
    class CraneClientRequest_SubmitRequest;
    class CraneClientRequest_StartRequest;
    class CraneClientRequest_StopRequest;
    class CraneClientRequest_KillRequest;
    class CraneClientRequest_StatusRequest;
    class CraneClientRequest_SendRequest;
    // Nimbus -> Client submessages
    class CraneClientReply_SubmitReply;
    class CraneClientReply_StartReply;
    class CraneClientReply_StopReply;
    class CraneClientReply_KillReply;
    class CraneClientReply_StatusReply;
    class CraneClientReply_SendReply;
    class CraneClientRequest_BoltSpec;
    class CraneClientRequest_SpoutSpec;
    class CraneClientRequest_Parent;
//    class CraneConfigureConnections;
    // Nimbus -> Supervisor and Supervisor -> Nimbus
    class CraneNimbusRequest;
    class CraneNimbusReply;
    // Nimbus -> Supervisor submessages
    class CraneNimbusRequest_AddWorkerRequest;
    class CraneNimbusRequest_StartWorkerRequest;
    class CraneNimbusRequest_KillWorkerRequest;
    class CraneNimbusRequest_ConfigureConnectionsRequest;
    // Supervisor -> Nimbus sumbessages
    class CraneNimbusReply_AddWorkerReply;
    class CraneNimbusReply_StartWorkerReply;
    class CraneNimbusReply_KillWorkerReply;
    class CraneNimbusReply_ConfigureConnectionsReply;
    // Supervisor <-> Worker
    class CraneSupervisorRequest;
    class CraneSupervisorReply;
    class CraneSupervisorRequest_ConfigureConnectionsRequest;
    class CraneSupervisorRequest_StartRequest;
    class CraneSupervisorRequest_KillRequest;
    class CraneSupervisorReply_ConfigureConnectionsReply;
    class CraneSupervisorReply_StartReply;
    class CraneSupervisorReply_KillReply;
    // Generic messages
    class CraneTuple;
    class CraneTupleBatch;
    class WorkerConfigureConnectionsRequest;
}

namespace election {
    class ElectionStart;
    class OkMessage;
    class NewLeader;
    class ElectionConfirm;
    class ElectionWrappedMessage;
}

class DMember;
class Introducer;
class MessageHandler;
class Member;
class EventHandler;
class ElectionMessageHandler;
class DFileMessageHandler;
class CraneMessageHandler;
class SystemHelper;
class FileHelper;
class DFile;
class Tuple;
class Worker;


class FileInfo
{
public:
    FileInfo() {}
    FileInfo(const std::string& filename, const std::string& filemd5,  int filesize)
            : name(filename)
            , md5(filemd5)
            , size(filesize)
    {}
    ~FileInfo() = default;

    std::string name;
    std::string md5;
    int size;
};

using Clock = std::chrono::high_resolution_clock;
using TimeStamp = std::chrono::high_resolution_clock::time_point;

using ByteValue = std::uint8_t;
using Buffer = std::vector<ByteValue>;
using RawBuffer = void *;

template<typename T> using Pointer = std::shared_ptr<T>;
template<typename T> using UniquePointer = std::unique_ptr<T>;


using BufferPointer = Pointer<Buffer>;
using DFileMessageHandlerPointer = Pointer<DFileMessageHandler>;
using MessagePointer = Pointer<::google::protobuf::Message>;
using SocketPointer = Pointer<zmq::socket_t>;
using WrappedDFileMessagePointer = Pointer<dfile::WrappedDFileMessage>;

using ServiceRequestPointer = Pointer<dfile::ServiceRequest>;
using ServiceReplyPointer = Pointer<dfile::ServiceReply>;
using PutServicePointer = Pointer<dfile::ServiceRequest_PutRequest>;
using GetServicePointer = Pointer<dfile::ServiceRequest_GetRequest>;
using DeleteServicePointer = Pointer<dfile::ServiceRequest_DeleteRequest>;
using ListServicePointer = Pointer<dfile::ServiceRequest_ListRequest>;
using UpdateServicePointer = Pointer<dfile::ServiceRequest_UpdateRequest>;
using PutServiceReplyPointer = Pointer<dfile::ServiceReply_PutReply>;
using GetServiceReplyPointer = Pointer<dfile::ServiceReply_GetReply>;
using DeleteServiceReplyPointer = Pointer<dfile::ServiceReply_DeleteReply>;
using UpdateServiceReplyPointer = Pointer<dfile::ServiceReply_UpdateReply>;
using ListServiceReplyPointer = Pointer<dfile::ServiceReply_ListReply>;

using SendRequestPointer = Pointer<dfile::SendRequest>;
using SendReplyPointer = Pointer<dfile::SendReply>;
using FetchRequestPointer = Pointer<dfile::FetchRequest>;
using FetchReplyPointer = Pointer<dfile::FetchReply>;
using CommandRequestPointer = Pointer<dfile::CommandRequest>;
using CommandReplyPointer = Pointer<dfile::CommandReply>;
using ReplicateCommandPointer = Pointer<dfile::CommandRequest_ReplicateCommand>;
using DeleteCommandPointer = Pointer<dfile::CommandRequest_DeleteCommand>;
using ListCommandPointer = Pointer<dfile::CommandRequest_ListCommand>;
using ReplicateReplyPointer = Pointer<dfile::CommandReply_ReplicateReply>;
using DeleteReplyPointer = Pointer<dfile::CommandReply_DeleteReply>;
using ListReplyPointer = Pointer<dfile::CommandReply_ListReply>;
using MessageHandlerPointer = Pointer<MessageHandler>;
using WrappedMessagePointer = Pointer<dmember::WrappedMessage>;
using HeartBeatPointer = Pointer<dmember::HeartBeat>;
using PingRequestPointer = Pointer<dmember::PingRequest>;
using PingAckPointer = Pointer<dmember::PingAck>;
using ConnectionUpdateRequestPointer = Pointer<dmember::ConnectionUpdateRequest>;
using UpdateAckPointer = Pointer<dmember::UpdateAck>;
using JoinRequestPointer = Pointer<dmember::JoinRequest>;
using JoinAckPointer = Pointer<dmember::JoinAck>;
using MemberPointer = Pointer<Member>;
using MemberId = std::string;

using ElectionWrappedMessagePointer = Pointer<election::ElectionWrappedMessage>;
using ElectionStartPointer = Pointer<election::ElectionStart>;

using ElectionOkMessagePointer = Pointer<election::OkMessage>;
using ElectionNewLeaderPointer = Pointer<election::NewLeader>;
using ElectionConfirmPointer = Pointer<election::ElectionConfirm>;

using MemberMap = std::map<MemberId, MemberPointer>;
using MemberMapPointer = Pointer<MemberMap>;

// Crane message pointers
using CraneWrappedMessagePointer = Pointer<crane::CraneWrappedMessage>;
using CraneClientRequestPointer = Pointer<crane::CraneClientRequest>;
using CraneClientReplyPointer = Pointer<crane::CraneClientReply>;
using CraneClientSubmitRequestPointer = Pointer<crane::CraneClientRequest_SubmitRequest>;
using CraneClientStartRequestPointer = Pointer<crane::CraneClientRequest_StartRequest>;
using CraneClientStopRequestPointer = Pointer<crane::CraneClientRequest_StopRequest>;
using CraneClientKillRequestPointer = Pointer<crane::CraneClientRequest_KillRequest>;
using CraneClientStatusRequestPointer = Pointer<crane::CraneClientRequest_StatusRequest>;
using CraneClientSendRequestPointer = Pointer<crane::CraneClientRequest_SendRequest>;
using CraneClientSubmitReplyPointer = Pointer<crane::CraneClientReply_SubmitReply>;
using CraneClientStartReplyPointer = Pointer<crane::CraneClientReply_StartReply>;
using CraneClientStopReplyPointer = Pointer<crane::CraneClientReply_StopReply>;
using CraneClientKillReplyPointer = Pointer<crane::CraneClientReply_KillReply>;
using CraneClientStatusReplyPointer = Pointer<crane::CraneClientReply_StatusReply>;
using CraneClientSendReplyPointer = Pointer<crane::CraneClientReply_SendReply>;

using CraneNimbusRequestPointer = Pointer<crane::CraneNimbusRequest>;
using CraneNimbusReplyPointer = Pointer<crane::CraneNimbusReply>;
using CraneNimbusAddWorkerRequestPointer = Pointer<crane::CraneNimbusRequest_AddWorkerRequest>;
using CraneNimbusStartWorkerRequestPointer = Pointer<crane::CraneNimbusRequest_StartWorkerRequest>;
using CraneNimbusKillWorkerRequestPointer = Pointer<crane::CraneNimbusRequest_KillWorkerRequest>;
using CraneNimbusConfigureConnectionsRequestPointer = Pointer<crane::CraneNimbusRequest_ConfigureConnectionsRequest>;
using CraneNimbusAddWorkerReplyPointer = Pointer<crane::CraneNimbusReply_AddWorkerReply>;
using CraneNimbusStartWorkerReplyPointer = Pointer<crane::CraneNimbusReply_StartWorkerReply>;
using CraneNimbusKillWorkerReplyPointer = Pointer<crane::CraneNimbusReply_KillWorkerReply>;
using CraneNimbusConfigureConnectionsReplyPointer = Pointer<crane::CraneNimbusReply_ConfigureConnectionsReply>;

using CraneSupervisorRequestPointer = Pointer<crane::CraneSupervisorRequest>;
using CraneSupervisorReplyPointer = Pointer<crane::CraneSupervisorReply>;
using CraneSupervisorConfigureConnectionsRequestPointer = Pointer<crane::CraneSupervisorRequest_ConfigureConnectionsRequest>;
using CraneSupervisorConfigureConnectionsReplyPointer = Pointer<crane::CraneSupervisorReply_ConfigureConnectionsReply>;
using CraneSupervisorStartRequestPointer = Pointer<crane::CraneSupervisorRequest_StartRequest>;
using CraneSupervisorStartReplyPointer = Pointer<crane::CraneSupervisorReply_StartReply>;
using CraneSupervisorKillRequestPointer = Pointer<crane::CraneSupervisorRequest_KillRequest>;
using CraneSupervisorKillReplyPointer = Pointer<crane::CraneSupervisorReply_KillReply>;

using CraneTuplePointer = Pointer<crane::CraneTuple>;
using CraneTupleBatchPointer = Pointer<crane::CraneTupleBatch>;
using WorkerConfigureConnectionsRequestPointer = Pointer<crane::WorkerConfigureConnectionsRequest>;
//using CraneConfigureConnectionsPointer = Pointer<crane::CraneConfigureConnections>;

// Crane core entities
using TuplePointer = Pointer<Tuple>;
//using TupleStream = std::queue<TuplePointer>;
using TupleStream = boost::lockfree::queue<Tuple *>;
using TupleStreamPointer = Pointer<TupleStream>;
using WorkerPointer = Pointer<Worker>;


using SimpleBufferPointer = Buffer *;
using SimpleMessageHandlerPointer = MessageHandler *;
using SimpleDFileMessageHandlerPointer = DFileMessageHandler *;
using SimpleCraneMessageHandlerPointer = CraneMessageHandler *;
using SimpleMessagePointer = ::google::protobuf::Message *;

using SimpleElectionMessageHandlerPointer = ElectionMessageHandler *;

using SimpleEventHandlerPointer = EventHandler *;
using EventHandlers = std::list<SimpleEventHandlerPointer>;




#endif //DFILE_TYPES_H
