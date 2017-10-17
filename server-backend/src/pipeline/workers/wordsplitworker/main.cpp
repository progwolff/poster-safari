#include "wordsplitworker.h"

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
    std::cout << "read content successfully\n";
    Postr::WordSplitWorker worker;
    
    int status = worker.processBlocking(data);
    std::cout << "before serializing\n";
    std::cout << data.serialize(false);
    
    return status;
}
