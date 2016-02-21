//
// Created by Tianyuan Liu on 10/22/15.
//

#include <DMember.h>
#include <election/Candidate.h>
#include "DFile.h"
#include "DFileMaster.h"


#include <SystemHelper.h>
#include <FileHelper.h>
#include <dfile.pb.h>
#include <easylogging++.h>
#include <message/DFileMessageParser.h>
#include <message/DFileMessageHandler.h>

#include <zmq.hpp>
#include <zmq_utils.h>

#include <map>
#include <iostream>
#include <thread>
#include <libgen.h>
#include <mutex>


struct DFile::Impl
{
    Impl(DFile *newparent)
        : parent(newparent)
        , context(1)
        , serviceSocket(context, ZMQ_REP)
        , fileRoot("files/")
        , dynamicPort(dynamicPortBase)
    {

        setNodeId();
        cleanLocalFileDir();
        serviceSocket.bind(SystemHelper::makeAddr("*", servicePort).c_str());
        master = DFileMaster::instance();

    }



    /* local file operations:
     * addToLocalFileMap():     add a file to local fileMap
     * buildLocalFIleInfos():   read from ./files directory to recover local files and write to local fileMap
     * listLocalFiles():             list all the files and their sizes in ./files directory
     * deleteLocalFile():            delete file from ./files directory and remove it from local fileMap
     * checkAvailable():        check if allowed to write a file to this machine
     */

    bool addToLocalFileMap(const std::string &filename)
    {
        if (!checkFileAvailable(filename)) return false;
        FileHelper fh(fileRoot+filename, chunkSize);
        FileInfo *fi = new FileInfo(filename, SystemHelper::getmd5(fileRoot+filename), fh.getSize());
        fileMap[id].push_back(fi);
        return true;
    }


    void cleanLocalFileDir()
    {
        fileMap[id];
        SystemHelper::exec("rm -f files/*");
    }

    void listLocalFiles()
    {
        LOG(INFO) << "List local files:";
        for (auto fi : fileMap[id])
        {
            LOG(INFO) << "File name: " << fi->name << " size: " << fi->size << " md5:" << fi->md5;
        }
    }

    void deleteLocalFile(const std::string &filename)
    {
        for (auto it = fileMap[id].begin(); it != fileMap[id].end(); ++it)
        {
            if ((*it)->name == filename) {
                fileMap[id].erase(it);
                std::remove((fileRoot+filename).c_str());
                return;
            }
        }
        LOG(ERROR) << "File name: " << filename << " not in file directory";
    }

    bool checkFileAvailable(const std::string &filename)
    {
        for (auto fi : fileMap[id])
        {
            if (fi->name == filename) return false;
        }
        return true;
    }

    void copyBufferedFile(const std::string &filename)
    {
        std::string filepath = filePathBuf[filename];
        SystemHelper::copyFile(filepath, fileRoot+filename);
        addToLocalFileMap(filename);
    }


    /* system operations:
     * setNodeId():     set node id to fa15-cs425-gxx-0x.cs.illinois.edu
     * nextPort():      get next dynamic port (mainly for data connection)
     * setMaster():     set this node as master, and then gather the fileMap from all members.
     */

    void setNodeId()
    {
        const std::string IPPrefix = "fa15-cs425-g10-0";
        const std::string IPSuffix = ".cs.illinois.edu";
        int nodeId = (SystemHelper::exec("uname -n")[static_cast<int>(IPPrefix.length())] - '0');

        std::stringstream ss;
        ss << IPPrefix << nodeId << IPSuffix;
        id = ss.str();
    }
    int nextPort()
    {
        if (dynamicPort == dynamicPortMax)
        {
            int tmp = dynamicPort;
            dynamicPort = dynamicPortBase;
            return tmp;
        }
        return dynamicPort++;
    }

    /* protocol operations: (for detail refer to design page)
     * sendFileRequest():     send a SendRequest to target
     *
     *
     */

    void sendFileRequest(const std::string &target, const std::string &fileName, const std::string &filepath="")
    {
        zmq::socket_t requestSocket(context, ZMQ_REQ);
        requestSocket.connect(SystemHelper::makeAddr(target, servicePort).c_str());

        zmq::socket_t testSocket(context, ZMQ_REP);
        int availablePort;
        while (true)
        {
            availablePort = nextPort();
            try {
                testSocket.bind(SystemHelper::makeAddr("*", availablePort).c_str());
                testSocket.close();
                break;
            }
            catch (const zmq::error_t &e)
            {
                LOG(ERROR) << "cannot bind to socket: " << e.what();
            }
        }

        FileHelper fh(fileRoot+fileName, chunkSize);
        std::string md5 = SystemHelper::getmd5(fileRoot+fileName);

        WrappedDFileMessagePointer wrappedDFileMessage = std::make_shared<dfile::WrappedDFileMessage>();
        wrappedDFileMessage->set_type(dfile::SEND_REQ);
        wrappedDFileMessage->mutable_sendrequest()->set_senderip(id);
        wrappedDFileMessage->mutable_sendrequest()->set_filename(fileName);
        wrappedDFileMessage->mutable_sendrequest()->set_filesize(fh.getSize());
        wrappedDFileMessage->mutable_sendrequest()->set_filemd5(md5);
        wrappedDFileMessage->mutable_sendrequest()->set_chunksize(chunkSize);
        wrappedDFileMessage->mutable_sendrequest()->set_port(availablePort);
        if (!filepath.empty())
            wrappedDFileMessage->mutable_sendrequest()->set_filepath(filepath);

        try {
            zmq::message_t msg(wrappedDFileMessage->ByteSize());
            wrappedDFileMessage->SerializeToArray(msg.data(), wrappedDFileMessage->ByteSize());

            requestSocket.send(msg);
            msg.rebuild();

            requestSocket.recv(&msg);
            DFileMessageParser::parse(msg.data(), msg.size(), &messageHandler);
        }
        catch (const std::exception &e) {
            LOG(ERROR) << e.what();
        }
    }

    void replySendRequest(WrappedDFileMessagePointer wrappedDFileMessage) {
        try {
            zmq::message_t msg(wrappedDFileMessage->ByteSize());
            wrappedDFileMessage->SerializeToArray(msg.data(), wrappedDFileMessage->ByteSize());

            serviceSocket.send(msg);

        }
        catch (const std::exception &e)
        {
            LOG(ERROR) << e.what();
        }

    }



    void fetchClientThread(const std::string &senderip, int port, const std::string &filename, const std::string &filemd5,
                           long filesize, int chunksize, const std::string &filepath="")
    {

        int totalChunks = filesize/chunksize;
        if(filesize % chunksize != 0) ++totalChunks;

        clientChunkCountMap[filename] = 0;

        zmq::socket_t fetchSocket(context, ZMQ_REQ);
        fetchSocket.connect(SystemHelper::makeAddr(senderip, port).c_str());

        while(clientChunkCountMap[filename] < totalChunks)
        {
            WrappedDFileMessagePointer wrappedDFileMessage = std::make_shared<dfile::WrappedDFileMessage>();
            wrappedDFileMessage->set_type(dfile::FETCH_REQ);
            wrappedDFileMessage->mutable_fetchrequest()->set_filename(filename);
            wrappedDFileMessage->mutable_fetchrequest()->set_port(port);
            wrappedDFileMessage->mutable_fetchrequest()->set_chunkid(clientChunkCountMap[filename]);

            zmq::message_t msg(wrappedDFileMessage->ByteSize());
            wrappedDFileMessage->SerializeToArray(msg.data(), wrappedDFileMessage->ByteSize());

            try {
                fetchSocket.send(msg);
                msg.rebuild();

                fetchSocket.recv(&msg);
                DFileMessageParser::parse(msg.data(), msg.size(), &messageHandler);
            }
            catch (const std::exception &e)
            {
                LOG(ERROR) << e.what();
            }
        }

        LOG(INFO) << "Done fetching file: " << filename;


        if (filepath.empty())
        {
            FileInfo *fi = new FileInfo(filename, filemd5, filesize);
            addToLocalFileMap(filename);
            sendUpdateRequest(fi);
        }
        else
        {
            SystemHelper::exec("mv " + fileRoot+filename + " " + filepath);
            //LOG(DEBUG) << "file moved: " << "mv " + fileRoot+filename + " " + filepath;
        }
    }

    void fetchServerThread(int port, const std::string &filename, int chunksize)
    {

        int timeout = 3000;
        zmq::socket_t fetchServerSocket(context, ZMQ_REP);
        fetchServerSocket.setsockopt(ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
        fetchServerSocket.bind(SystemHelper::makeAddr("*", port).c_str());


        serverChunkCountMap[port] = -1;
        std::string filePath = fileRoot + filename;
        FileHelper fp(filePath, chunksize);
        int chunkCount = -1;

        while(!fp.isEOF())
        {
            try
            {
                zmq::message_t msg;

                bool recv = fetchServerSocket.recv(&msg);
                if (!recv) return;

                DFileMessageParser::parse(msg.data(), msg.size(), &messageHandler);

                WrappedDFileMessagePointer wrappedDFileMessage = std::make_shared<dfile::WrappedDFileMessage>();
                wrappedDFileMessage->set_type(dfile::FETCH_REP);
                wrappedDFileMessage->mutable_fetchreply()->set_filename(filename);
                if (serverChunkCountMap[port] >= 0 && serverChunkCountMap[port] == chunkCount)
                {
                    std::string tmp(fp.getPrevChunk());
                    wrappedDFileMessage->mutable_fetchreply()->set_data(tmp);
                    wrappedDFileMessage->mutable_fetchreply()->set_chunkid(chunkCount);
                    wrappedDFileMessage->mutable_fetchreply()->set_chunksize(tmp.length());

                }
                else if(serverChunkCountMap[port] == chunkCount+1)
                {
                    std::string tmp(fp.getNextChunk());
                    wrappedDFileMessage->mutable_fetchreply()->set_data(tmp);
                    wrappedDFileMessage->mutable_fetchreply()->set_chunkid(++chunkCount);
                    wrappedDFileMessage->mutable_fetchreply()->set_chunksize(tmp.length());
                }
                else {
                    throw "Bad request chunk ID";
                }

                msg.rebuild(wrappedDFileMessage->ByteSize());
                wrappedDFileMessage->SerializeToArray(msg.data(), wrappedDFileMessage->ByteSize());
                fetchServerSocket.send(msg);
            }
            catch (std::string s) {
                LOG(ERROR) << s;
            }
            catch (const std::exception &e) {
                LOG(ERROR) << e.what();
            }
        }

        // wait for re-fetching the last piece
        try
        {
            zmq::message_t msg;
            bool recv = fetchServerSocket.recv(&msg);
            if (recv)
            {
                DFileMessageParser::parse(msg.data(), msg.size(), &messageHandler);

                WrappedDFileMessagePointer wrappedDFileMessage = std::make_shared<dfile::WrappedDFileMessage>();
                wrappedDFileMessage->set_type(dfile::FETCH_REP);
                if (serverChunkCountMap[port] >= 0 && serverChunkCountMap[port] == chunkCount)
                {
                    wrappedDFileMessage->mutable_fetchreply()->set_data(fp.getPrevChunk());
                }
                else {
                    throw "Bad request chunk ID";
                }

                msg.rebuild(wrappedDFileMessage->ByteSize());
                wrappedDFileMessage->SerializeToArray(msg.data(), wrappedDFileMessage->ByteSize());
                fetchServerSocket.send(msg);
            }

        }
        catch (std::string s) {
            LOG(ERROR) << s;
        }
        catch (const std::exception &e) {
            LOG(ERROR) << e.what();
        }

        LOG(INFO) << "Done sending file: " << filename;
    }

    void recvNextChunk(const std::string &filename, int chunkid, const std::string &data, long chunksize)
    {
        if (chunkid != clientChunkCountMap[filename]) return;
        if (static_cast<unsigned int>(data.length()) != chunksize)
        {
            //LOG(DEBUG) << "Bad chunksize for " << filename << "at id " << chunkid;
            return;
        }


        clientChunkCountMap[filename] += 1;
        std::string filePath = fileRoot + filename;
        std::ofstream of;
        of.open(filePath, std::ios_base::app);
        of << data;
        of.flush();

    }

    void sendNextChunk(int port, int chunkid)
    {
        serverChunkCountMap[port] = chunkid;
    }

    void startFetchClientThread(const std::string &senderip, int port, const std::string &filename,
                                const std::string &filemd5, long filesize, int chunksize,
                                const std::string &filepath="")
    {
        zmq_sleep(1); // wait for server socket to be ready
        std::thread fetchClientThread(&Impl::fetchClientThread, this, senderip, port, filename, filemd5,
                                      filesize, chunksize, filepath);
        fetchClientThread.detach();
    }

    void startFetchServerThread(int port, const std::string &filename, int chunksize)
    {
        std::thread fetchServerThread(&Impl::fetchServerThread, this, port, filename, chunksize);
        fetchServerThread.detach();
    }

    void serviceThread()
    {
        while (true)
        {
            try {
                zmq::message_t msg;
                serviceSocket.recv(&msg);
                DFileMessageParser::parse(msg.data(), msg.size(), &messageHandler);
            }
            catch (...) {
                LOG(ERROR) << "Bad file service request in node";
                break;
            }
        }
    }


    /* requests sent to master
     * sendPutRequest()
     * sendGetRequest()
     * sendDeleteRequest()
     * sendUpdateRequest()
     */
    void sendPutRequest(const std::string &filepath)
    {
        if(Candidate::instance()->leader().empty())
        {
            LOG(ERROR) << "No known leader. Sending put request aborted.";
            return;
        }

        std::string filename = basename(const_cast<char*>(filepath.c_str()));
        std::string md5 = SystemHelper::getmd5(filepath);
        FileHelper fh(filepath, chunkSize);
        long filesize = fh.getSize();
        WrappedDFileMessagePointer wrappedDFileMessage = std::make_shared<dfile::WrappedDFileMessage>();
        wrappedDFileMessage->set_type(dfile::SERVICE_REQ);
        wrappedDFileMessage->mutable_servicerequest()->set_type(dfile::ServiceType::PUT);
        wrappedDFileMessage->mutable_servicerequest()->set_id(id);
        wrappedDFileMessage->mutable_servicerequest()->mutable_putrequest()->set_filename(filename);
        wrappedDFileMessage->mutable_servicerequest()->mutable_putrequest()->set_filemd5(md5);
        wrappedDFileMessage->mutable_servicerequest()->mutable_putrequest()->set_filesize(filesize);;

        zmq::socket_t socket(context, ZMQ_REQ);
        socket.setsockopt(ZMQ_RCVTIMEO, &requestTimeout, sizeof(requestTimeout));
        socket.connect(SystemHelper::makeAddr(Candidate::instance()->leader(), masterServicePort).c_str());

        // buffer filepath
        filePathBuf[filename] = filepath;

        try {
            zmq::message_t msg(wrappedDFileMessage->ByteSize());
            wrappedDFileMessage->SerializeToArray(msg.data(), wrappedDFileMessage->ByteSize());

            socket.send(msg);
            msg.rebuild();

            bool recv = socket.recv(&msg);
            if (recv)
                DFileMessageParser::parse(msg.data(), msg.size(), &messageHandler);
        }
        catch (const std::exception &e)
        {
            LOG(ERROR) << e.what();
        }
    }

    void sendGetRequest(const std::string &filename, const std::string &filepath)
    {
        if(SystemHelper::exist(filepath))
        {
            LOG(WARNING) << "filepath: " << filepath << " exist. Aborted fetching.";
            return;
        }
        SystemHelper::exec("touch " + filepath);

        // if file replicated at this machine
        if(hasFile(id, filename))
        {
            SystemHelper::copyFile(fileRoot+filename, filepath);
            LOG(INFO) << "Done getting file: " << filename;
            return;
        }

        // send a get request to leader

        if(Candidate::instance()->leader().empty())
        {
            LOG(ERROR) << "No known leader. Sending get request aborted.";
            return;
        }

        WrappedDFileMessagePointer wrappedDFileMessage = std::make_shared<dfile::WrappedDFileMessage>();
        wrappedDFileMessage->set_type(dfile::SERVICE_REQ);
        wrappedDFileMessage->mutable_servicerequest()->set_type(dfile::ServiceType::GET);
        wrappedDFileMessage->mutable_servicerequest()->set_id(id);
        wrappedDFileMessage->mutable_servicerequest()->mutable_getrequest()->set_filename(filename);
        wrappedDFileMessage->mutable_servicerequest()->mutable_getrequest()->set_filepath(filepath);

        zmq::socket_t socket(context, ZMQ_REQ);
        socket.setsockopt(ZMQ_RCVTIMEO, &requestTimeout, sizeof(requestTimeout));
        socket.connect(SystemHelper::makeAddr(Candidate::instance()->leader(), masterServicePort).c_str());

        try {
            zmq::message_t msg(wrappedDFileMessage->ByteSize());
            wrappedDFileMessage->SerializeToArray(msg.data(), wrappedDFileMessage->ByteSize());

            socket.send(msg);
            msg.rebuild();

            bool recv = socket.recv(&msg);
            if (recv)
                DFileMessageParser::parse(msg.data(), msg.size(), &messageHandler);
        }
        catch (const std::exception &e)
        {
            LOG(ERROR) << e.what();
        }
    }

    void sendDeleteRequest(const std::string &filename)
    {
        if(Candidate::instance()->leader().empty())
        {
            LOG(ERROR) << "No known leader. Sending delete request aborted.";
            return;
        }

        WrappedDFileMessagePointer wrappedDFileMessage = std::make_shared<dfile::WrappedDFileMessage>();
        wrappedDFileMessage->set_type(dfile::SERVICE_REQ);
        wrappedDFileMessage->mutable_servicerequest()->set_type(dfile::ServiceType::DEL);
        wrappedDFileMessage->mutable_servicerequest()->set_id(id);
        wrappedDFileMessage->mutable_servicerequest()->mutable_deleterequest()->set_filename(filename);

        zmq::socket_t socket(context, ZMQ_REQ);
        socket.setsockopt(ZMQ_RCVTIMEO, &requestTimeout, sizeof(requestTimeout));
        socket.connect(SystemHelper::makeAddr(Candidate::instance()->leader(), masterServicePort).c_str());

        try {
            zmq::message_t msg(wrappedDFileMessage->ByteSize());
            wrappedDFileMessage->SerializeToArray(msg.data(), wrappedDFileMessage->ByteSize());

            socket.send(msg);
            msg.rebuild();

            bool recv = socket.recv(&msg);
            if (recv)
                DFileMessageParser::parse(msg.data(), msg.size(), &messageHandler);
        }
        catch (const std::exception &e)
        {
            LOG(ERROR) << e.what();
        }
    }

    void sendUpdateRequest(FileInfo *fi)
    {
         if(Candidate::instance()->leader().empty())
        {
            LOG(ERROR) << "No known leader. Sending update request aborted.";
            return;
        }


        WrappedDFileMessagePointer wrappedDFileMessage = std::make_shared<dfile::WrappedDFileMessage>();
        wrappedDFileMessage->set_type(dfile::SERVICE_REQ);
        wrappedDFileMessage->mutable_servicerequest()->set_type(dfile::ServiceType::UPDATE);
        wrappedDFileMessage->mutable_servicerequest()->set_id(id);
        wrappedDFileMessage->mutable_servicerequest()->mutable_updaterequest()->set_filename(fi->name);
        wrappedDFileMessage->mutable_servicerequest()->mutable_updaterequest()->set_filemd5(fi->md5);
        wrappedDFileMessage->mutable_servicerequest()->mutable_updaterequest()->set_filesize(fi->size);

        zmq::socket_t socket(context, ZMQ_REQ);
        socket.setsockopt(ZMQ_RCVTIMEO, &requestTimeout, sizeof(requestTimeout));
        socket.connect(SystemHelper::makeAddr(Candidate::instance()->leader(), masterServicePort).c_str());

        try {
            zmq::message_t msg(wrappedDFileMessage->ByteSize());
            wrappedDFileMessage->SerializeToArray(msg.data(), wrappedDFileMessage->ByteSize());

            socket.send(msg);
            msg.rebuild();

            bool recv = socket.recv(&msg);
            if (recv)
                DFileMessageParser::parse(msg.data(), msg.size(), &messageHandler);
        }
        catch (const std::exception &e)
        {
            LOG(ERROR) << e.what();
        }
    }


    void sendListRequest(const std::string &filename)
    {
         if(Candidate::instance()->leader().empty())
        {
            LOG(ERROR) << "No known leader. Sending update request aborted.";
            return;
        }


        WrappedDFileMessagePointer wrappedDFileMessage = std::make_shared<dfile::WrappedDFileMessage>();
        wrappedDFileMessage->set_type(dfile::SERVICE_REQ);
        wrappedDFileMessage->mutable_servicerequest()->set_type(dfile::ServiceType::LST);
        wrappedDFileMessage->mutable_servicerequest()->set_id(id);
        wrappedDFileMessage->mutable_servicerequest()->mutable_listrequest()->set_filename(filename);

        zmq::socket_t socket(context, ZMQ_REQ);
        socket.setsockopt(ZMQ_RCVTIMEO, &requestTimeout, sizeof(requestTimeout));
        socket.connect(SystemHelper::makeAddr(Candidate::instance()->leader(), masterServicePort).c_str());

        try {
            zmq::message_t msg(wrappedDFileMessage->ByteSize());
            wrappedDFileMessage->SerializeToArray(msg.data(), wrappedDFileMessage->ByteSize());

            socket.send(msg);
            msg.rebuild();

            bool recv = socket.recv(&msg);
            if (recv)
                DFileMessageParser::parse(msg.data(), msg.size(), &messageHandler);
        }
        catch (const std::exception &e)
        {
            LOG(ERROR) << e.what();
        }
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

    bool hasFile(const std::string &id, const std::string &filename)
    {
        for (auto it = fileMap[id].begin(); it != fileMap[id].end(); ++it)
        {
            if ((*it)->name == filename) return true;
        }
        return false;
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


    void start()
    {
        std::thread serviceThread(&Impl::serviceThread, this);
        serviceThread.detach();
    }

    DFile *parent;
    zmq::context_t context;
    zmq::socket_t serviceSocket;
    DFileMessageHandler messageHandler;
    DFileMaster *master;


    std::string id;
    std::string fileRoot;
    const int masterServicePort = 9599;
    const int servicePort = 9600;
    const int dynamicPortBase = 9601;
    const int dynamicPortMax = 9800;

    const int chunkSize = 1024*250;
    const int requestTimeout = 1000;

    int dynamicPort;

    std::map<std::string, std::vector<FileInfo*> > fileMap;

    std::map<std::string, int> clientChunkCountMap; // map filename to chunk count
    std::map<int, int> serverChunkCountMap; // map server port to chunk count

    std::map<std::string, std::string> filePathBuf;

};

DFile::DFile()
    : _pImpl(new Impl(this))
{ }

DFile::~DFile() = default;

DFile * DFile::instance()
{
    static DFile *app = nullptr;
    if ( app == nullptr ) app = new DFile();
    return app;
}

bool DFile::checkFileAvailable(const std::string &filename)
{
    return _pImpl->checkFileAvailable(filename);

}

void DFile::replyServiceSocket(WrappedDFileMessagePointer wrappedDFileMessage)
{
    _pImpl->replySendRequest(wrappedDFileMessage);
}

void DFile::startFetchClientThread(const std::string &senderip, int port,
                                    const std::string &filename, const std::string &filemd5,
                                   long filesize, int chunksize, const std::string &filepath)
{
    _pImpl->startFetchClientThread(senderip, port, filename, filemd5, filesize, chunksize, filepath);
//    LOG(DEBUG) << "start fetching client for " << filename;
}

void DFile::startFetchServerThread(int port, const std::string &filename, int chunksize)
{
    _pImpl->startFetchServerThread(port, filename, chunksize);
//    LOG(DEBUG) << "start fetching server for " << filename << " at port: " << port;
}

void DFile::sendFileRequest(const std::string &target, const std::string &filename, const std::string &filepath)
{
    _pImpl->sendFileRequest(target, filename, filepath);
}

void DFile::start()
{
    _pImpl->start();
}


void DFile::recvNextChunk(const std::string &filename, int chunkid, const std::string &data, long chunksize)
{
    _pImpl->recvNextChunk(filename, chunkid, data, chunksize);
}

void DFile::sendNextChunk(int port, int chunkid) {
    _pImpl->sendNextChunk(port, chunkid);

}

void DFile::listFiles() {
    _pImpl->listLocalFiles();
}

void DFile::deleteLocalFile(const std::string &filename) {
    _pImpl->deleteLocalFile(filename);

}


void DFile::sendPutRequest(const std::string &filepath)
{
    if(!SystemHelper::exist(filepath))
    {
        LOG(ERROR) << "File path not existed";
        return;
    }
   _pImpl->sendPutRequest(filepath);
}

void DFile::copyBufferedFile(const std::string &filename) {
    _pImpl->copyBufferedFile(filename);
}

void DFile::sendGetRequest(const std::string &filename, const std::string &filepath) {
    _pImpl->sendGetRequest(filename, filepath);
}

void DFile::sendDeleteRequest(const std::string &filename) {
    _pImpl->sendDeleteRequest(filename);
}

std::vector<FileInfo *> *DFile::localFileInfos() {
    return &(_pImpl->fileMap[_pImpl->id]);
}

void DFile::sendListRequest(const std::string &filename)
{
    _pImpl->sendListRequest(filename);
}

