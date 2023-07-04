#include <unistd.h>
#include <string.h>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <queue>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <utility>
#include <memory>
#include <thread>
#include <tuple>
#include <functional>
#include <atomic>
#include <mutex>
#include <memory>
#include <chrono>
#include <random>
#include <condition_variable>
#include "common.h"
extern void startRTSPThread(const char *url, const char *usrname, const char *passwd,int tid);


threadinfo ti[NUMBER_OF_THREADS];

void initThreadInfo()
{
    for(int i=0;i<NUMBER_OF_THREADS;i++)
    {
        ti[i].inUse=0;
        ti[i].stopflag=0;
        ti[i].eventLoopWatchVariable=0;
    }
}

int findEmptyThread()
{
    for(int i=0;i<NUMBER_OF_THREADS;i++)
    {
        if(ti[i].inUse==0)
        {
            ti[i].inUse=1;
            return i;
        }
    }
    return -1;
}

int main(int argc,char *argv[])
{
    int tid=0;

    initThreadInfo();
    tid=findEmptyThread();
    std::thread t1(startRTSPThread,"rtsp://10.135.245.194/live1s1.sdp","","",tid);
    t1.detach();
    sleep(3);
    
    ti[tid].stopflag=1;
    printf("set tid %d stopflag to 1\n",tid);
    while (ti[tid].stopflag == 1)
        usleep(100);
    //t1.join();
    /*ti[tid].stopflag=0;
    ti[tid].inUse=0;

    tid=findEmptyThread();
    t1=std::thread(startRTSPThread,"rtsp://10.135.245.194/live1s1.sdp","","",tid);
    sleep(3);

    ti[tid].stopflag=1;
    t1.join();
    ti[tid].stopflag=0;
    ti[tid].inUse=0;*/
    //sleep(5);
    //t1.terminate();
}