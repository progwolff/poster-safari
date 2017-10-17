#include "textgroupcollateworker.h"

#include <iostream>

int main(int /*argc*/, char** argv)
{
    std::ifstream ifs(argv[1], std::ifstream::in);
    
    std::string str;
    std::string file_contents;
    while (std::getline(ifs, str))
    {
        file_contents += str;
        file_contents.push_back('\n');
    } 
    
    Postr::Data data(file_contents);
    
    Postr::TextGroupCollateWorker worker;
    
    Postr::Worker::interactive = false;
    int status = worker.processBlocking(data);
    
    std::cout << data;
    
    return status;
}
