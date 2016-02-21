//
// Created by e-al on 28.08.15.
//

#include <Types.h>
#include <crane.pb.h>
#include <src/CraneClient.h>
#include "MessageHandler.h"

void MessageHandler::process(const CraneClientSubmitReplyPointer &reply) {
    if(!reply->approval())
    {
        std::cout << "Request denied" << std::endl;
        return;
    }

    Client::instance()->send(reply->toponame());
}

void MessageHandler::process(const CraneClientStartReplyPointer &reply) {

}

void MessageHandler::process(const CraneClientStopReplyPointer &reply) {

}

void MessageHandler::process(const CraneClientKillReplyPointer &reply) {

}

void MessageHandler::process(const CraneClientSendReplyPointer &reply) {
    if (!reply->md5match())
    {
        std::cout << "Topology: " << reply->toponame() << " code submission failed. Please try again." << std::endl;
    }
    else
    {
        std::cout << "Topology: " << reply->toponame() << " code submission succeeded." << std::endl;
    }
}

void MessageHandler::process(const CraneClientStatusReplyPointer &reply) {
    if (reply->topostatus_size() != 0)
    {
        for (auto ts : reply->topostatus())
        {
            std::cout << "Topology ID: " << ts.toponame() << std::endl;
            std::cout << "status: " << ts.status() << std::endl;

            if (ts.workers_size() != 0)
            {
                std::cout << "workers: " << std::endl;
                for (auto worker : ts.workers())
                {
                    std::cout << "\t" << worker.runname() << ": " << worker.address() << std::endl;
                }
            }
            std::cout << std::endl;
        }
    }
}
