//
// Created by Tianyuan Liu on 9/16/15.
//

#include <boost/program_options.hpp>

#include <Types.h>
#include <easylogging++.h>
#include <CraneClient.h>
#include <topology/Topology.h>

// initializing logging
#define ELPP_THREAD_SAFE
INITIALIZE_EASYLOGGINGPP

namespace po = boost::program_options;

using namespace topology;


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

    std::string config;

    po::options_description opts("Options");
    opts.add_options()
            ("help,h", "Show help")
            ("config,c", po::value<std::string>(&config)->default_value("crane.conf"), "config file")
            ("introducer,i", "Run dfile as introducer");
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(opts).run(), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << opts << std::endl;
        return -1;
    }

    //logInit();


    Client *client = Client::instance();
    client->config("fa15-cs425-g10-01.cs.illinois.edu", 4444);
//    std::string path = "/home/e-al/Projects/C++/UIUC/CS425/cs425-mp4/user-code/SimpleTopology";

    TopologyBuilder builder;

    // WordCountTopology
    std::string path = "/home/e-al/Projects/C++/UIUC/CS425/cs425-mp4/user-code/WordCountTopology";
    builder.name = "wordcounttopo";
    builder.setSpout("input", "InputSpout");
    builder.setBolt("split", "SplitSentenceBolt", 5)->setParent("input", SHUFFLE);
    builder.setBolt("wordcount", "WordCountBolt", 5)->setParent("split", FIELD);
    builder.setBolt("print", "PrinterBolt", 1)->setParent("wordcount", FIELD);

    // JoinTopology
//    std::string path = "/home/e-al/Projects/C++/UIUC/CS425/cs425-mp4/user-code/JoinTopology";
//    builder.name = "jointopo";
//    builder.setSpout("input", "InputSpout");
//    builder.setBolt("join", "JoinBolt", 5)->setParent("input", SHUFFLE);
//    builder.setBolt("print", "PrinterBolt", 1)->setParent("join", FIELD);

    // FilterTopology
//    std::string path = "/home/e-al/Projects/C++/UIUC/CS425/cs425-mp4/user-code/FilterTopology";
//    builder.name = "filtertopo";
//    builder.setSpout("input", "InputSpout");
//    builder.setBolt("filter", "FilterBolt", 5)->setParent("input", SHUFFLE);
//    builder.setBolt("print", "PrinterBolt", 1)->setParent("filter", FIELD);


    while(true)
    {
        std::string command;
        std::cin >> command;
        if (command == "show")
        {
            builder.showTopology();
        }
        if (command == "submit")
        {
            client->submit(builder.buildTopology(), path);
        }
        if (command == "start")
        {
            client->start(builder.name);
        }
        if (command == "stop")
        {
            client->stop(builder.name);
        }
        if (command == "kill")
        {
            client->kill(builder.name);
        }
        if (command == "status")
        {
            client->status();
        }
    }

}