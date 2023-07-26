#include "rtspclient.hh"

void onFrameArrival(unsigned char* clientData, const char *codecName, unsigned frameSize, unsigned numTruncatedBytes,struct timeval presentationTime)
{
    //printf("onFrameArrival\n");
    std::cout<<"codec name:"<<codecName<<" frame size:"<<frameSize<<" presentationTime:"<<presentationTime.tv_sec<<std::endl;
}

int main(int argc,char *argv[])
{
    rtspPlayer *player = new rtspPlayer();
    player->onFrameData = onFrameArrival;
    if (player->startRTSP((const char *)"rtsp://10.170.0.2:8554/slamtv60.264", "username1", "password1") == OK)
    {
        sleep(3);
        player->stopRTSP();
    }
    delete(player);
}