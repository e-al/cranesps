//
// Created by Tianyuan Liu on 10/22/15.
//

#ifndef DFILE_DFILE_H
#define DFILE_DFILE_H

#include <Types.h>

class DFile{
public :

    static DFile* instance();
    virtual ~DFile();


    bool checkFileAvailable(const std::string &);

    void start();

    void listFiles();
    void deleteLocalFile(const std::string &);




    void sendPutRequest(const std::string &filepath);
    void sendGetRequest(const std::string &filename, const std::string &filepath);
    void sendDeleteRequest(const std::string &filename);
    void sendListRequest(const std::string &filename);

    void copyBufferedFile(const std::string &);

    void sendFileRequest(const std::string &, const std::string &, const std::string &filepath="");
    void replyServiceSocket(WrappedDFileMessagePointer);

    void startFetchClientThread(const std::string &, int,
                                const std::string &, const std::string &, long, int,
                                const std::string &filepath="");
    void startFetchServerThread(int, const std::string &, int);

    void recvNextChunk(const std::string &, int, const std::string &, long);
    void sendNextChunk(int, int);

    std::vector<FileInfo*> *localFileInfos();

private:
    DFile();
    struct Impl;
    UniquePointer<Impl> _pImpl;

};


#endif //DFILE_DFILE_H
