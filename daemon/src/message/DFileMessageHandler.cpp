//
// Created by e-al on 28.08.15.
//

#include <iostream>
#include <list>
#include <sstream>
#include <filesys/DFile.h>
#include <dfile.pb.h>
#include "DFileMessageHandler.h"
#include <easylogging++.h>

#include <election/Candidate.h>
#include <src/DMember.h>
#include <src/filesys/DFileMaster.h>

void DFileMessageHandler::process(const SendRequestPointer &request)
{
//    LOG(DEBUG) << "SendRequest message received";
    WrappedDFileMessagePointer wrappedDFileMessage = std::make_shared<dfile::WrappedDFileMessage>();
    wrappedDFileMessage->set_type(dfile::SEND_REP);
    if (!DFile::instance()->checkFileAvailable(request->filename()))
    {
        wrappedDFileMessage->mutable_sendreply()->set_approve(false);
        DFile::instance()->replyServiceSocket(wrappedDFileMessage);
        return;
    }

    wrappedDFileMessage->mutable_sendreply()->set_approve(true);
    wrappedDFileMessage->mutable_sendreply()->set_filename(request->filename());
    wrappedDFileMessage->mutable_sendreply()->set_chunksize(request->chunksize());
    wrappedDFileMessage->mutable_sendreply()->set_port(request->port());

//    std::cout << __PRETTY_FUNCTION__ << ": has filepath" << std::endl;
    DFile::instance()->replyServiceSocket(wrappedDFileMessage);

    if (request->has_filepath())
    {
        DFile::instance()->startFetchClientThread(request->senderip(), request->port(),
                request->filename(), request->filemd5(), request->filesize(),
                request->chunksize(), request->filepath());
    }
    else
        DFile::instance()->startFetchClientThread(request->senderip(), request->port(),
                                                  request->filename(), request->filemd5(), request->filesize(),
                                                  request->chunksize());

}


void DFileMessageHandler::process(const SendReplyPointer &reply) {
//    LOG(DEBUG) << "SendReply message received";
    if (!reply->approve())
    {
//        LOG(ERROR) << "Send request denied. File already exist at target.";
        return;
    }
    DFile::instance()->startFetchServerThread(reply->port(), reply->filename(), reply->chunksize());
}

void DFileMessageHandler::process(const FetchRequestPointer &request) {
//    LOG(DEBUG) << "FetchRequest message received";
    DFile::instance()->sendNextChunk(request->port(), request->chunkid());
}

void DFileMessageHandler::process(const FetchReplyPointer &reply) {
    DFile::instance()->recvNextChunk(reply->filename(), reply->chunkid(), reply->data(), reply->chunksize());
}

void DFileMessageHandler::process(const ServiceRequestPointer &request) {

//    LOG(DEBUG) << "ServiceRequest message received";
    WrappedDFileMessagePointer wrappedDFileMessage = std::make_shared<dfile::WrappedDFileMessage>();
    wrappedDFileMessage->set_type(dfile::SERVICE_REP);

    switch (request->type())
    {
        case dfile::ServiceType::PUT:
        {
//            LOG(DEBUG) << "PUT request received";
            wrappedDFileMessage->mutable_servicereply()->set_type(dfile::ServiceType::PUT);
            PutServicePointer putServ(request->release_putrequest());
            int count = DFileMaster::instance()->countReplicates(putServ->filename());
            wrappedDFileMessage->mutable_servicereply()->mutable_putreply()->set_approve(!count);
            if(!count) wrappedDFileMessage->mutable_servicereply()->mutable_putreply()->set_filename(putServ->filename());

            DFileMaster::instance()->replyMasterSocket(wrappedDFileMessage);

            if (count == 0)
            {
                FileInfo *fi = new FileInfo(putServ->filename(), putServ->filemd5(), putServ->filesize());
                DFileMaster::instance()->addToFileMap(request->id(), fi);
                DFileMaster::instance()->replicateFile(putServ->filename());
            }

            break;
        }
        case dfile::ServiceType::GET:
        {
            wrappedDFileMessage->mutable_servicereply()->set_type(dfile::ServiceType::GET);
            GetServicePointer getServ(request->release_getrequest());
            int count = DFileMaster::instance()->countReplicates(getServ->filename());
            wrappedDFileMessage->mutable_servicereply()->mutable_getreply()->set_available(count);
            DFileMaster::instance()->replyMasterSocket(wrappedDFileMessage);

            if (count)
            {
                std::vector<std::string> target;
                target.push_back(request->id());
                for (auto member : DMember::instance()->membershipList())
                {
                    if (DFileMaster::instance()->hasFile(member, getServ->filename()))
                    {
                        DFileMaster::instance()->sendReplicateCommand(member, target, getServ->filename(), getServ->filepath());
                        break;
                    }
                }
            }
            break;
        }

        case dfile::ServiceType::DEL:
        {
            wrappedDFileMessage->mutable_servicereply()->set_type(dfile::ServiceType::DEL);
            DeleteServicePointer delServ(request->release_deleterequest());
            int count = DFileMaster::instance()->countReplicates(delServ->filename());
            wrappedDFileMessage->mutable_servicereply()->mutable_deletereply()->set_available(count);
            wrappedDFileMessage->mutable_servicereply()->mutable_deletereply()->set_filename(delServ->filename());
            DFileMaster::instance()->replyMasterSocket(wrappedDFileMessage);

            if(count)
                DFileMaster::instance()->deleteFile(delServ->filename());



            break;
        }


        case dfile::ServiceType::UPDATE:
        {
            wrappedDFileMessage->mutable_servicereply()->set_type(dfile::ServiceType::UPDATE);
            UpdateServicePointer updateServ(request->release_updaterequest());

            FileInfo *fi = new FileInfo(updateServ->filename(), updateServ->filemd5(), updateServ->filesize());
            DFileMaster::instance()->addToFileMap(request->id(), fi);
            DFileMaster::instance()->replyMasterSocket(wrappedDFileMessage);
            break;
        }

        case dfile::ServiceType::LST:
        {

            wrappedDFileMessage->mutable_servicereply()->set_type(dfile::ServiceType::LST);
            ListServicePointer listServ(request->release_listrequest());
            for (auto member : DMember::instance()->membershipList())
            {
                if(DFileMaster::instance()->hasFile(member, listServ->filename()))
                {
                    wrappedDFileMessage->mutable_servicereply()->mutable_listreply()->add_id(member);
                }
            }
            DFileMaster::instance()->replyMasterSocket(wrappedDFileMessage);
            break;
        }


        default:
            LOG(ERROR) << "Invalid service request";
    }


}



void DFileMessageHandler::process(const CommandRequestPointer &request)
{
    if (Candidate::instance()->leader() != request->senderip())
    {
        LOG(ERROR) << "command comes from non-leader node: " << request->senderip();
        return;
    }

    WrappedDFileMessagePointer wrappedDFileMessage = std::make_shared<dfile::WrappedDFileMessage>();
    wrappedDFileMessage->set_type(dfile::COMMAND_REP);
    wrappedDFileMessage->mutable_commandreply()->set_senderip(DMember::instance()->id());


    switch (request->type())
    {
        case dfile::CommandType::REPLICATE:
        {
            wrappedDFileMessage->mutable_commandreply()->set_type(dfile::CommandType::REPLICATE);
            ReplicateCommandPointer replicateCommand(request->release_replicatecommand());
            if (replicateCommand->has_filepath())
            {

                for (auto target : replicateCommand->targets())
                {
                    DFile::instance()->sendFileRequest(target, replicateCommand->filename(), replicateCommand->filepath());
                }
            }
            else {

                for (auto target : replicateCommand->targets())
                {
                    DFile::instance()->sendFileRequest(target, replicateCommand->filename());
                }
            }
            break;
        }
        case dfile::CommandType::DELETE:
        {

            wrappedDFileMessage->mutable_commandreply()->set_type(dfile::CommandType::DELETE);
            DeleteCommandPointer deleteCommand(request->release_deletecommand());
            DFile::instance()->deleteLocalFile(deleteCommand->filename());
            break;
        }
        case dfile::CommandType::LIST:
        {
            wrappedDFileMessage->mutable_commandreply()->set_type(dfile::CommandType::LIST);
            for (auto fi : *(DFile::instance()->localFileInfos()))
            {
                auto fileinfo = wrappedDFileMessage->mutable_commandreply()->mutable_listreply()->add_fileinfo();
                fileinfo->set_filename(fi->name);
                fileinfo->set_filemd5(fi->md5);
                fileinfo->set_size(fi->size);
            }
            break;
        }
        default:
        {

            LOG(ERROR) << "Invalid command";
            return;
        }
    }
    DFile::instance()->replyServiceSocket(wrappedDFileMessage);
}

void DFileMessageHandler::process(const CommandReplyPointer &reply)
{
    switch (reply->type())
    {
        case dfile::CommandType::REPLICATE:
        {
            break;
        }
        case dfile::CommandType::DELETE:
        {
            break;
        }
        case dfile::CommandType::LIST:
        {
            if (reply->has_listreply())
            {
                ListReplyPointer listRep(reply->release_listreply());
                std::vector<FileInfo*> fileInfos;
                for (auto fileinfo : listRep->fileinfo())
                {
                    FileInfo *fi = new FileInfo(fileinfo.filename(), fileinfo.filemd5(), fileinfo.size());
                    fileInfos.push_back(fi);
                }
                DFileMaster::instance()->updateFileMap(reply->senderip(), fileInfos);
            }

            break;
        }
        default:
            LOG(ERROR) << "Invalid command reply";
    }
}

void DFileMessageHandler::process(const ServiceReplyPointer &reply)
{
    switch (reply->type())
    {
        case dfile::ServiceType::PUT:
        {
            PutServiceReplyPointer putRep(reply->release_putreply());
            if (putRep->approve())
            {
                DFile::instance()->copyBufferedFile(putRep->filename());
                // TODO: need to wait if file is too large
            }
            break;
        }
        case dfile::ServiceType::GET:
        {
            GetServiceReplyPointer getRep(reply->release_getreply());
            if (!getRep->available())
            {
                LOG(ERROR) << "requested file: " << getRep->filename() << " not available";
            }
            break;
        }

        case dfile::ServiceType::DEL:
        {

            DeleteServiceReplyPointer deleteRep(reply->release_deletereply());
            if (!deleteRep->available())
            {
                LOG(ERROR) << "requested file: " << deleteRep->filename() << " not available";
            }
            break;
        }

        case dfile::ServiceType::UPDATE:
            break;

        case dfile::ServiceType::LST:
        {
            ListServiceReplyPointer listRep(reply->release_listreply());
            if (listRep)
            {
                for (auto id : listRep->id())
                {
                    std::cout << id << std::endl;
                }
            }
            break;
        }

        default:
            LOG(ERROR) << "Invalid service reply";
    }
}
