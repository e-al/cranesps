//
// Created by Tianyuan Liu on 10/22/15.
//

#ifndef DFILE_FILEHELPER_H
#define DFILE_FILEHELPER_H

#include <fstream>
#include <string>

class FileHelper {
public:
    FileHelper(const std::string &fileName, int chunkSize)
    {
        ifs.open(fileName, std::ifstream::in | std::ios::binary);
        ch_size = chunkSize;
    }

    ~FileHelper()
    {
        ifs.close();
    }

    bool isEOF()
    {
        return ifs.eof();
    }

    std::string readAll()
    {
        buffer.clear();
        buffer.assign((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
        return buffer;
    }

    std::string getNextChunk()
    {
        int count = 0;
        buffer.clear();
        while (!ifs.eof() && count < ch_size)
        {
            ++count;
            buffer.append(1, ifs.get());
        }
        if (ifs.eof()) { buffer.pop_back(); }
        return buffer;
    }


    std::string getPrevChunk()
    {
        return buffer;
    }


    long getSize()
    {
        ifs.seekg(0, std::ios::end);
        long length = ifs.tellg();
        ifs.seekg(0, std::ios::beg);
        return length;
    }

private:
    std::ifstream ifs;
//    char *buffer;
    std::string buffer;
    int ch_size;

};


#endif //DFILE_FILEHELPER_H
