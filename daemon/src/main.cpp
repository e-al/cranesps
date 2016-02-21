//
// Created by Tianyuan Liu on 9/16/15.
//

#include <boost/program_options.hpp>

#include <filesys/DFile.h>
#include <DMember.h>
#include <election/Candidate.h>
#include <Types.h>
#include <unistd.h>
#include <easylogging++.h>
#include <src/filesys/DFileMaster.h>
#include <src/nimbus/Nimbus.h>
#include <supervisor/Supervisor.h>
#include <SystemHelper.h>
#include <worker/Worker.h>

// initializing logging
#define ELPP_THREAD_SAFE
INITIALIZE_EASYLOGGINGPP

namespace po = boost::program_options;

const char* getPid(void) {
    return std::to_string(::getpid()).c_str();
}

const char* getPname(void) {
    std::string path = ::getenv("_");
    std::size_t pos = path.rfind('/');
    if (pos != path.length() - 1)
        return path.substr(pos+1).c_str();
    return "";
}

void logInit() {
    el::Helpers::installCustomFormatSpecifier(el::CustomFormatSpecifier("%pid", getPid));
    el::Helpers::installCustomFormatSpecifier(el::CustomFormatSpecifier("%pname", getPname));

    el::Configurations conf(std::string(getenv("HOME")) + "/dmember/log.conf");

    el::Loggers::reconfigureAllLoggers(conf);
    return;
}

int main(int argc, char *argv[])
{
    /* uncomment the following block to run in background
    pid_t pid = fork();
    if (pid < 0) std::cerr << "Failed to create dfile" << std::endl;
    if (pid > 0) exit(0);
    */

    int worker_port;
    std::string worker_libname;
    std::string worker_id;
    po::options_description opts("Options");
    opts.add_options()
            ("help,h", "Show help")
            ("introducer,i", "Run daemon as introducer")
            ("worker,w", "Run daemon as worker")
            ("port,p", po::value<int>(&worker_port)->default_value(10000), "worker port")
            ("lib,l", po::value<std::string>(&worker_libname)->default_value("libbolt.so"), "lib name to run in worker")
            ("id,a", po::value<std::string>(&worker_id), "IPC address to which the worker should bind")
            ("bolt,b", "Run worker as bolt");

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(opts).run(), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << opts << std::endl;
        return -1;
    }

    logInit();
    LOG(INFO) << "Test Log";
    auto dmember = DMember::instance();

    bool asIntroducer = vm.count("introducer");
    bool asWorker = vm.count("worker");
    if (asIntroducer && asWorker)
    {
        std::cerr << "Cannot start both as worker and as an introducer!" << std::endl;
        return -1;
    }

    if (asWorker)
    {
        std::cout << "Start as worker" << std::endl;
        std::cout << "Trying to start worker with id: " << worker_id << std::endl;
        SystemHelper::exec("echo \"started as worker\" > /tmp/worker.out");
        Worker worker(worker_id, worker_port, worker_libname, vm.count("bolt") > 0, false);
        worker.start();
        return 0;
    }

    if (asIntroducer)
    {
        std::cout << "Start introducer" << std::endl;
    }
    else { std::cout << "Start member node" << std::endl; }

    dmember->start(asIntroducer);

    std::cout << "Enter \"leave\" to gracefully exit the group" << std::endl;
    std::cout << "Enter \"list\" to print a membership list" << std::endl;
    std::cout << "Enter \"listf\" to print file list" << std::endl;
    std::cout << "Enter \"id\" to print a node's id" << std::endl;

    // Start leader election
    auto candidate = Candidate::instance();
    candidate->start();
    
    DFile *dfile = DFile::instance();
    dfile->start();


    // Start crane
    auto nimbus = Nimbus::instance();
    if (asIntroducer) nimbus->start();

    auto supervisor = Supervisor::instance();
    supervisor->start();


    while (true)
    {
        std::cout << ">>> ";
        std::string command;
        std::cin >> command;
        // help infos
        if (command == "help")
        {
            std::cout << "DMember operations:" << std::endl;
            std::cout << "leave: " << "exit the group" << std::endl;
            std::cout << "list: " << "print membership list" << std::endl;
            std::cout << "id: " << "print node's id" << std::endl;
            std::cout << "leader: " << "show current leader id" << std::endl;
            std::cout << "DFile operations:" << std::endl;
            std::cout << "put path/to/file: " << "put file in the file system" << std::endl;
            std::cout << "get file local/path: " << "get file and store at local path" << std::endl;
            std::cout << "delete file: " << "delete file from the file system" << std::endl;
            std::cout << "store: " << "list files stored at this machine" << std::endl;
            std::cout << "listf file: " << "list where file is replicated" << std::endl;
        }

            // local file operations
        else if (command == "send")
        {
            std::string target, filename;
            std::cin >> target >>filename;
            dfile->sendFileRequest(target, filename);
        }

        else if (command == "listfiles")
        {
            if (Candidate::instance()->leader() == DMember::instance()->id())
            {
                DFileMaster::instance()->listFiles();
            }
            else std::cout << "Not leader. Aborted." << std::endl;
        }

        else if (command == "deletelocal")
        {
            std::string filename;
            std::cin >> filename;
            dfile->deleteLocalFile(filename);
        }

            // dfile operations
        else if (command == "put")
        {
            std::string filepath;
            std::cin >> filepath;
            dfile->sendPutRequest(filepath);
        }

        else if (command == "get")
        {
            std::string filename, filepath;
            std::cin >> filename >> filepath;
            dfile->sendGetRequest(filename, filepath);
        }

        else if (command == "delete")
        {
            std::string filename;
            std::cin >> filename;
            dfile->sendDeleteRequest(filename);
        }

        else if (command == "store")
        {
            dfile->listFiles();
        }

        else if (command == "listf")
        {
            std::string filename;
            std::cin >> filename;
            dfile->sendListRequest(filename);
        }

            // dmember operations
        else if (command == "leave")
        {
            dmember->stop();
            break;
        }
        else if (command == "list")
        {
            std::cout << "Membership list for node with id = " << dmember->id() << std::endl;
            std::cout << "Id" << '\t' << "Counter" << '\t' << "Timestamp" << '\t'
            << "Leaving" << '\t' << "Suspicious" << '\t' << "Failed" << std::endl;
            for (const auto& member : dmember->membershipList())
            {
                std::cout << member << std::endl;
            }
        }
        else if (command == "id")
        {
            std::cout << "Current node id = " << dmember->id() << std::endl;
        }
        else if (command == "leader")
        {
            std::cout << "Current leader = " << candidate->leader() << std::endl;
        }
        else
        {
            std::cout << "Incorrect command, please try again" << std::endl;
        }

    }
}