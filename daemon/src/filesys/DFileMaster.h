//
// Created by yura on 11/16/15.
//

#ifndef DMEMBER_DFILEMASTER_H
#define DMEMBER_DFILEMASTER_H


#include <event/EventHandler.h>
#include <Types.h>

class DFileMaster : public EventHandler {
public:
    virtual void handle(const event::NodeFailEvent &event);
    virtual void handle(const event::LeaderElectedEvent &event);

    static DFileMaster* instance();
    virtual ~DFileMaster() = default;

    bool hasFile(const std::string &id, const std::string &filename);

    void start();


    int countReplicates(const std::string &filename);
    void updateFileMap(const std::string &id, const std::vector<FileInfo*> &filemap);
    void addToFileMap(const std::string &id, FileInfo *fi);
    void replicateFile(const std::string &filename);
    void deleteFile(const std::string &filename);

    void sendReplicateCommand(const std::string &id, std::vector<std::string> &targets,
                              const std::string &filename, const std::string &filepath="");

    void replyMasterSocket(WrappedDFileMessagePointer wrappedDFileMessage);

    void listFiles();

private:
    DFileMaster();
    struct Impl;
    UniquePointer<Impl> _pImpl;
};


#endif //DMEMBER_DFILEMASTER_H
