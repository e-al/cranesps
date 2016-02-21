//
// Created by yura on 11/20/15.
//

#include "CraneMessageParser.h"
#include "CraneMessageHandler.h"
//#include "../../../../../../.CLion12/system/cmake/generated/7c3ee9e0/7c3ee9e0/Debug/protocol/crane.pb.h"
#include <easylogging++.h>
#include <crane.pb.h>

void CraneMessageParser::parse(BufferPointer buffer, SimpleCraneMessageHandlerPointer handler)
{ parse(buffer.get(), handler); }

void CraneMessageParser::parse(SimpleBufferPointer buffer, SimpleCraneMessageHandlerPointer handler)
{ parse(buffer->data(), buffer->size(), handler); }

void CraneMessageParser::parse(const void* buffer, int size, SimpleCraneMessageHandlerPointer handler)
{
    crane::CraneWrappedMessage msg;
    if (!msg.ParseFromArray(buffer, size))
    {
        throw std::logic_error("Cannot parse the message");
    }

    switch (msg.type())
    {
        case crane::CLIENT_REQ:
        {
            CraneClientRequestPointer clientReq(msg.release_craneclientrequest());
            switch (clientReq->type())
            {
                //TODO: might represent it in more compact form
                case crane::CraneClientRequest::SUBMIT:
                {
                    CraneClientSubmitRequestPointer submitReq(clientReq->release_submitrequest());
                    handler->process(submitReq);
                    break;
                }

                case crane::CraneClientRequest::SEND:
                {
                    CraneClientSendRequestPointer sendReq(clientReq->release_sendrequest());
                    handler->process(sendReq);
                    break;
                }
                case crane::CraneClientRequest::START:
                {
                    CraneClientStartRequestPointer startReq(clientReq->release_startrequest());
                    handler->process(startReq);
                    break;
                }
                case crane::CraneClientRequest::STOP:
                {
                    CraneClientStopRequestPointer stopReq(clientReq->release_stoprequest());
                    handler->process(stopReq);
                    break;
                }
                case crane::CraneClientRequest::KILL:
                {
                    CraneClientKillRequestPointer killReq(clientReq->release_killrequest());
                    handler->process(killReq);
                    break;
                }

                case crane::CraneClientRequest::STATUS:
                {
                    CraneClientStatusRequestPointer statusReq(clientReq->release_statusrequest());
                    handler->process(statusReq);
                    break;
                }
                default:
                {
                    LOG(ERROR) << "Invalid nimbus request";
                }
            }

            break;
        }

        case crane::NIMBUS_REQ:
        {
            CraneNimbusRequestPointer request(msg.release_cranenimbusrequest());
            switch (request->type())
            {
                case crane::CraneNimbusRequest::ADD_WORKER:
                {
                    CraneNimbusAddWorkerRequestPointer addWorkerRequest(request->release_addworkerrequest());
                    handler->process(addWorkerRequest);
                    break;
                }

                case crane::CraneNimbusRequest::START_WORKER:
                {
                    CraneNimbusStartWorkerRequestPointer startWorkerRequest(request->release_startworkerrequest());
                    handler->process(startWorkerRequest);
                    break;
                }

                case crane::CraneNimbusRequest::KILL_WORKER:
                {
                    CraneNimbusKillWorkerRequestPointer killWorkerRequest(request->release_killworkerrequest());
                    handler->process(killWorkerRequest);
                    break;
                }

                case crane::CraneNimbusRequest::CONFIG_CONNS:
                {
                    CraneNimbusConfigureConnectionsRequestPointer confConns(
                            request->release_configureconnectionsrequest());
                    handler->process(confConns);
                    break;
                }
                default:
                {
                    LOG(ERROR) << "Invalid nimbus request";
                }
            }
            break;
        }

        case crane::NIMBUS_REP:
        {
            CraneNimbusReplyPointer reply(msg.release_cranenimbusreply());
            switch (reply->type())
            {
                case crane::CraneNimbusReply::ADD_WORKER:
                {
                    CraneNimbusAddWorkerReplyPointer addWorkerReply(reply->release_addworkerreply());
                    handler->process(addWorkerReply);
                    break;
                }

                case crane::CraneNimbusReply::START_WORKER:
                {
                    CraneNimbusStartWorkerReplyPointer startWorkerReply(reply->release_startworkerreply());
                    handler->process(startWorkerReply);
                    break;
                }

                case crane::CraneNimbusReply::KILL_WORKER:
                {
                    CraneNimbusKillWorkerReplyPointer killWorkerReply(reply->release_killworkerreply());
                    handler->process(killWorkerReply);
                    break;
                }

                case crane::CraneNimbusReply::CONFIG_CONNS:
                {
                    CraneNimbusConfigureConnectionsReplyPointer confConns(
                            reply->release_configureconnectionsreply());
                    handler->process(confConns);
                    break;
                }
                default:
                {
                    LOG(ERROR) << "Invalid nimbus reply";
                }
            }
            break;
        }

        case crane::SUPERVISOR_REQ:
        {
            CraneSupervisorRequestPointer request(msg.release_cranesupervisorrequest());
            switch (request->type())
            {
                case crane::CraneSupervisorRequest::CONFIG_CONNS:
                {
                    CraneSupervisorConfigureConnectionsRequestPointer confConns(
                            request->release_configureconnectionsrequest());
                    handler->process(confConns);
                    break;
                }

                case crane::CraneSupervisorRequest::START:
                {
                    CraneSupervisorStartRequestPointer startRequest(request->release_startrequest());
                    handler->process(startRequest);
                    break;
                }

                case crane::CraneSupervisorRequest::KILL:
                {
                    CraneSupervisorKillRequestPointer killRequest(request->release_killrequest());
                    handler->process(killRequest);
                    break;
                }

                default:
                {
                    LOG(ERROR) << "Invalid supervisur request";
                }
            }
            break;
        }

        case crane::SUPERVISOR_REP:
        {
            CraneSupervisorReplyPointer reply(msg.release_cranesupervisorreply());
            switch (reply->type())
            {
                case crane::CraneSupervisorReply::CONFIG_CONNS:
                {
                    CraneSupervisorConfigureConnectionsReplyPointer confConns(
                            reply->release_configureconnectionsreply());
                    handler->process(confConns);
                    break;
                }

                case crane::CraneSupervisorReply::START:
                {
                    CraneSupervisorStartReplyPointer startReply(reply->release_startreply());
                    handler->process(startReply);
                    break;
                }

                case crane::CraneSupervisorReply::KILL:
                {
                    CraneSupervisorKillReplyPointer killReply(reply->release_killreply());
                    handler->process(killReply);
                    break;
                }

                default:
                {
                    LOG(ERROR) << "Invalid supervisor reply";
                }
            }
            break;
        }

        case crane::TUPLE:
        {
            CraneTuplePointer tuple(msg.release_cranetuple());
            handler->process(tuple);
            break;
        }

        case crane::TUPLE_BATCH:
        {
            CraneTupleBatchPointer tupleBatch(msg.release_cranetuplebatch());
            handler->process(tupleBatch);
            break;
        }

        default:
        {
            LOG(ERROR) << "Invalid nimbus request";
        }

    }

}