//
// Created by yura on 11/16/15.
//

#include <zmq.hpp>
#include <SystemHelper.h>
#include <src/election/Candidate.h>
#include <easylogging++.h>
#include <src/message/DFileMessageParser.h>
#include <src/message/DFileMessageHandler.h>
#include <src/DMember.h>
#include <thread>
#include <dfile.pb.h>
#include <mutex>
#include "DFileMaster.h"

struct DFileMaster::Impl
{
    Impl(DFileMaster *newparent)
            : parent(newparent)
            , context(1)
            , masterServiceSocket(context, ZMQ_REP)
            , id(DMember::instance()->id())
    {
        masterServiceSocket.bind(SystemHelper::makeAddr("*", masterServicePort).c_str());
        Candidate::instance()->subscribe<event::LeaderElectedEvent>(parent);
        DMember::instance()->subscribe<event::NodeFailEvent>(parent);
    }

    void masterServiceThread()
    {
        while (Candidate::instance()->leader() == id)
        {
            try {
                zmq::message_t msg;
                //LOG(DEBUG) << "Trying to receive master request";
                masterServiceSocket.recv(&msg);
                //LOG(DEBUG) << "Master request received, trying to parse";
                DFileMessageParser::parse(msg.data(), msg.size(), &messageHandler);
                //LOG(DEBUG) << "Master request parsed";
            }
            catch (...) {
                LOG(ERROR) << "Bad file service request in Master";
            }
        }
    }

    void replyServiceRequest(WrappedDFileMessagePointer wrappedDFileMessage) {
        try {
            zmq::message_t msg(wrappedDFileMessage->ByteSize());
            wrappedDFileMessage->SerializeToArray(msg.data(), wrappedDFileMessage->ByteSize());

            masterServiceSocket.send(msg);
        }
        catch (const std::exception &e)
        {
            LOG(ERROR) << e.what();
        }

    }

    void start()
    {

        std::thread masterThread(&Impl::masterServiceThread, this);

        for (auto member : DMember::instance()->membershipList())
        {
            sendListCommand(member);
        }
        masterThread.detach();
        reReplicate();

    }

    int countReplicates(const std::string &filename)
    {
        int count = 0;
        for (auto it = fileMap.begin(); it != fileMap.end(); ++it)
        {
            for (auto fi : it->second)
            {
                if (fi->name == filename)
                {
                    ++count;
                    break;
                }
            }
        }
        return count;
    }

    void updateFileMap(const std::string &id, const std::vector<FileInfo*> &fileinfos)
    {
        fileMap[id].clear();
        fileMap[id] = fileinfos;
        //LOG(DEBUG) << "Filemap updated";
    }

    bool hasFile(const std::string &id, const std::string &filename)
    {
        for (auto it = fileMap[id].begin(); it != fileMap[id].end(); ++it)
        {
            if ((*it)->name == filename) return true;
        }
        return false;
    }

    void addToFileMap(const std::string &id, FileInfo *fi)
    {
        // warning: did not check if there is duplication
        if(hasFile(id, fi->name)) return;
        fileMap[id].push_back(fi);
    }

    void deleteFromFileMap(const std::string &id, const std::string &filename)
    {
        std::vector<FileInfo*> newVector;
        for (auto it = fileMap[id].begin(); it != fileMap[id].end(); ++it)
        {
            if ((*it)->name != filename)
            {
                newVector.push_back(*it);
            }
        }
        fileMap[id] = std::move(newVector);
    }

    void sendReplicateCommand(const std::string &source, std::vector<std::string> &targets,
                              const std::string &filename, const std::string &filepath="")
    {
        zmq::socket_t socket(context, ZMQ_REQ);
        socket.connect(SystemHelper::makeAddr(source, servicePort).c_str());

        WrappedDFileMessagePointer wrappedDFileMessage = std::make_shared<dfile::WrappedDFileMessage>();
        wrappedDFileMessage->set_type(dfile::COMMAND_REQ);
        wrappedDFileMessage->mutable_commandrequest()->set_type(dfile::CommandType::REPLICATE);
        wrappedDFileMessage->mutable_commandrequest()->set_senderip(id);
        wrappedDFileMessage->mutable_commandrequest()->mutable_replicatecommand()->set_filename(filename);
        if (!filepath.empty())
            wrappedDFileMessage->mutable_commandrequest()->mutable_replicatecommand()->set_filepath(filepath);

        for (auto target : targets)
        {
            wrappedDFileMessage->mutable_commandrequest()->mutable_replicatecommand()->add_targets(target);
        }

        try {
            zmq::message_t msg(wrappedDFileMessage->ByteSize());
            wrappedDFileMessage->SerializeToArray(msg.data(), wrappedDFileMessage->ByteSize());
            socket.send(msg);
            msg.rebuild();

            socket.recv(&msg);
            DFileMessageParser::parse(msg.data(), msg.size(), &messageHandler);
        }
        catch (const std::exception &e)
        {
            LOG(ERROR) << e.what();
        }
    }

    void sendDeleteCommand(const std::string &target, const std::string &filename)
    {
        zmq::socket_t socket(context, ZMQ_REQ);
        socket.connect(SystemHelper::makeAddr(target, servicePort).c_str());

        WrappedDFileMessagePointer wrappedDFileMessage = std::make_shared<dfile::WrappedDFileMessage>();
        wrappedDFileMessage->set_type(dfile::COMMAND_REQ);
        wrappedDFileMessage->mutable_commandrequest()->set_type(dfile::CommandType::DELETE);
        wrappedDFileMessage->mutable_commandrequest()->set_senderip(id);
        wrappedDFileMessage->mutable_commandrequest()->mutable_deletecommand()->set_filename(filename);

        try {
            zmq::message_t msg(wrappedDFileMessage->ByteSize());
            wrappedDFileMessage->SerializeToArray(msg.data(), wrappedDFileMessage->ByteSize());
            socket.send(msg);
            msg.rebuild();

            socket.recv(&msg);
            DFileMessageParser::parse(msg.data(), msg.size(), &messageHandler);
        }
        catch (const std::exception &e)
        {
            LOG(ERROR) << e.what();
        }
    }

    void sendListCommand(const std::string &target)
    {
        zmq::socket_t socket(context, ZMQ_REQ);
        socket.connect(SystemHelper::makeAddr(target, servicePort).c_str());

        WrappedDFileMessagePointer wrappedDFileMessage = std::make_shared<dfile::WrappedDFileMessage>();
        wrappedDFileMessage->set_type(dfile::COMMAND_REQ);
        wrappedDFileMessage->mutable_commandrequest()->set_type(dfile::CommandType::LIST);
        wrappedDFileMessage->mutable_commandrequest()->set_senderip(id);

//        LOG(DEBUG) << "Trying to send request list to " << target;
        try {
            zmq::message_t msg(wrappedDFileMessage->ByteSize());
            wrappedDFileMessage->SerializeToArray(msg.data(), wrappedDFileMessage->ByteSize());
            socket.send(msg);
//            LOG(DEBUG) << "Successfully send request list, about to process" << target;
            msg.rebuild();

            socket.recv(&msg);
            DFileMessageParser::parse(msg.data(), msg.size(), &messageHandler);
        }
        catch (const std::exception &e)
        {
            LOG(ERROR) << e.what();
        }
    }


    void replicateFile(const std::string &filename)
    {
        const int MAX_REPLICATE = 3;
        int numMembers = static_cast<int>(DMember::instance()->membershipList().size());
        int count = countReplicates(filename);

        int numTargets = std::min(numMembers, MAX_REPLICATE) - count;

        if (!numTargets) return;

        std::vector<std::string> members;
        for (auto member : DMember::instance()->membershipList())
        {
            members.push_back(member);
        }

        // pick random targets
        std::set<std::string> targets;
        while (static_cast<unsigned int>(numTargets) > targets.size())
        {
            std::string target = members[rand()%numMembers];
            while(hasFile(target, filename))
                target = members[rand()%numMembers];
            targets.insert(target);
        }

        // pick a random source to balance traffic
        std::string source = members[rand()%numMembers];
        while(!hasFile(source, filename))
            source = members[rand()%numMembers];

        std::vector<std::string> vec_targets(targets.begin(), targets.end());
        sendReplicateCommand(source, vec_targets, filename);
    }



    void deleteFile(const std::string &filename)
    {
        std::vector<std::string> targets;
        for (auto member : DMember::instance()->membershipList())
        {
            if(hasFile(member, filename)) targets.push_back(member);
        }
        for (auto target : targets)
        {
            sendDeleteCommand(target, filename);
            deleteFromFileMap(target, filename);
        }
    }

    void reReplicate()
    {
        std::set<std::string> files;
        for (auto member : DMember::instance()->membershipList())
        {
            for (auto it = fileMap[member].begin(); it != fileMap[member].end(); ++it)
            {
                files.insert((*it)->name);
            }
        }

        for (auto file : files)
        {
            replicateFile(file);
        }
    }

    void listFiles()
    {
        LOG(INFO) << "List all files:";
        for (auto id : DMember::instance()->membershipList())
        {
            LOG(INFO) << id;
            for (auto fi : fileMap[id])
            {
                LOG(INFO) << "File name: " << fi->name << " size: " << fi->size << " md5:" << fi->md5;
            }
        }
    }

    DFileMaster* parent;
    zmq::context_t context;
    zmq::socket_t masterServiceSocket;
    DFileMessageHandler messageHandler;


    std::string id;

    const int masterServicePort = 9599;
    const int servicePort = 9600;
    std::map<std::string, std::vector<FileInfo*> > fileMap;

    std::mutex rereplicationMutex;
};




DFileMaster::DFileMaster()
    : _pImpl(new DFileMaster::Impl(this))
{
}

DFileMaster *DFileMaster::instance() {
    static DFileMaster* app = nullptr;
    if (app == nullptr) app = new DFileMaster();
    return app;
}

void DFileMaster::start() {
    _pImpl->start();

}

int DFileMaster::countReplicates(const std::string &filename) {
    return _pImpl->countReplicates(filename);
}

void DFileMaster::updateFileMap(const std::string &id, const std::vector<FileInfo *> &filemap) {
    _pImpl->updateFileMap(id, filemap);
}

void DFileMaster::addToFileMap(const std::string &id, FileInfo *fi) {
    _pImpl->addToFileMap(id, fi);
}

void DFileMaster::replicateFile(const std::string &filename) {
    _pImpl->replicateFile(filename);
}

void DFileMaster::deleteFile(const std::string &filename) {
    _pImpl->deleteFile(filename);
}

void DFileMaster::sendReplicateCommand(const std::string &id, std::vector<std::string> &targets,
                                       const std::string &filename, const std::string &filepath) {
    _pImpl->sendReplicateCommand(id, targets, filename, filepath);
}

bool DFileMaster::hasFile(const std::string &id, const std::string &filename) {
    return _pImpl->hasFile(id, filename);
}

void DFileMaster::handle(const event::NodeFailEvent &event) {
    std::thread rereplicateThread([this, event]()
    {
        std::unique_lock<std::mutex> lk(_pImpl->rereplicationMutex);

        _pImpl->fileMap[event.id].clear();
        _pImpl->reReplicate();
    });

    rereplicateThread.detach();
}

void DFileMaster::handle(const event::LeaderElectedEvent &event) {
    if (event.id == _pImpl->id) {
        _pImpl->start(); // this will also start a new master thread
    }
}

void DFileMaster::replyMasterSocket(WrappedDFileMessagePointer wrappedDFileMessage) {
    _pImpl->replyServiceRequest(wrappedDFileMessage);
}

void DFileMaster::listFiles() {
    _pImpl->listFiles();
}
