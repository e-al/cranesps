//
// Created by Tianyuan Liu on 9/23/15.
//

#ifndef DFILE_SYSTEMHELPER_H
#define DFILE_SYSTEMHELPER_H

#include <Types.h>

#include <string>
#include <algorithm>
#include <list>
#include <unistd.h>
#include <sstream>
#include <sys/types.h>
#include <dirent.h>
#include <fstream>
#include <typeinfo>

class SystemHelper {
public:


    static std::string exec(const std::string& cmd) {
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return "ERROR";
        char buffer[128];
        std::string result = "";
        while (!feof(pipe)) {
            if (fgets(buffer, 128, pipe) != NULL)
                result += buffer;
        }
        pclose(pipe);
        return result;
    }

    static std::string makeAddr(const std::string &ip, int port) {
        std::stringstream ss;
        std::string prefix = "tcp://";
        ss << prefix << ip << ":" << std::to_string(port);
        return ss.str();

    }

    static std::string timeToString(TimeStamp ts)
    {
        std::stringstream ss;
        auto time = static_cast<time_t>(std::chrono::duration_cast<std::chrono::seconds>(ts.time_since_epoch()).count());
        ss << std::ctime(&time);
        return ss.str();
    }
    

    static bool exist(const std::string &filepath)
    {
        std::ifstream ifs(filepath.c_str());
        if (ifs) {
            ifs.close();
            return true;
        } else {
            ifs.close();
            return false;
        }
    }

    static bool copyFile(const std::string &src, const std::string &dest)
    {
        std::ifstream source(src, std::ios::binary);
        std::ofstream destination(dest, std::ios::binary);
        destination << source.rdbuf();
        return source && destination;
    }

    static std::string getmd5(const std::string &filepath)
    {
        if (!exist(filepath)) return "";
        std::string command = "md5sum " + filepath;
        std::string output = exec(command);
        return output.substr(0, 32);
    }

    // Used for field grouping
    static int hashField(const std::string& str)
    {
        int hash = 0;
        for (const char& c : str) { hash += c; }
        return hash;
//        std::string command = "printf '%s' " + str + " | md5sum";
//        std::string output = exec(command);
//
//        std::string truncMd5 = output.substr(0, 4);
//        std::size_t processedChars;
//        int md5Int = std::stol(truncMd5, &processedChars, 16);

//        return md5Int;
    }

    static int getdir (std::string dir, std::vector<std::string> &files)
    {
        DIR *dp;
        struct dirent *dirp;
        if((dp  = opendir(dir.c_str())) == NULL) {
            std::cout << "Error(" << errno << ") opening " << dir << std::endl;
            return errno;
        }

        while ((dirp = readdir(dp)) != NULL) {
            files.push_back(std::string(dirp->d_name));
        }
        closedir(dp);
        return 0;
    }
    
    static std::list<std::string> lowerIds(const std::list<std::string>& ids, const std::string& id)
    {
        std::list<std::string> result;
        auto isLower = [&id](const std::string& i) { return i < id;  };
        auto it = std::find_if(std::begin(ids), std::end(ids), isLower);
        while (it != std::end(ids))
        {
            result.emplace_back(*it);
            it = std::find_if(std::next(it), std::end(ids), isLower);
        }

        return result;
    }

    template <class ObjectT>
    static std::string getClassName(const ObjectT &object)
    {
        std::string ret = typeid(object).name();
        while (ret[0] >= '0' && ret[0] <= '9') ret = ret.substr(1);
        return ret;
    }

    // for now this one simply adds ".so" to the worker id
    // but more sophisticated name mangling is possible
    static std::string getWorkerSharedObject(const std::string& workerId)
    {
        std::string result(workerId);
        result.append(".so");

        return result;
    }

    static std::string getLibraryName(const std::string& topoId, const std::string& runName)
    {
        std::string libname{"lib"};
        libname.append(topoId);
        libname.append("_");
        libname.append(runName);
        libname.append(".so");
        std::transform(libname.begin(), libname.end(), libname.begin(), ::tolower);

        return libname;
    }

    static std::string getLibraryLocalPath(const std::string& libname)
    {
        std::string path{"./"};
        path.append(libname);

        return path;
    }

    static bool isBolt(const std::string& runName)
    {
        return runName.find("Bolt") != std::string::npos || runName.find("bolt") != std::string::npos;
    }

    template<typename IterType, typename RandomGenerator>
    static IterType selectRandom(IterType start, IterType end, RandomGenerator& g)
    {
        std::uniform_int_distribution<> distance(0, std::distance(start, end) - 1);
        std::advance(start, distance(g));
        return start;
    };

    template<typename IterType>
    static IterType selectRandom(IterType start, IterType end)
    {
        static std::random_device randomDevice;
        static std::mt19937 generator(randomDevice());
        return selectRandom(start, end, generator);
    }
};

#endif //DMEMBER_SYSTEMHELPER_H
