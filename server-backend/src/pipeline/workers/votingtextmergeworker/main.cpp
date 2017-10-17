#include "votingtextmergeworker.h"

#include <iostream>

int main(int argc, char** argv)
{
    Postr::VotingTextMergeWorker worker;
    
    Postr::Data data;
    for(int i=0; i<argc/2; ++i)
    {
        data.meta["text"][i]["text"] = argv[i+1];
        data.meta["text"][i]["confidence"] = 0.5;
        data.meta["text"][i]["replaces"][0]["text"] = argv[i+2];
        data.meta["text"][i]["replaces"][0]["confidence"] = 0.5;
    }
    worker.processBlocking(data);
    
    return 0;
}
