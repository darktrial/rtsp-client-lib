#include "rtspClientHelper.hh"

void onFrameArrival(unsigned char* clientData, const char *codecName, unsigned frameSize, unsigned numTruncatedBytes,struct timeval presentationTime)
{
    std::cout<<"codec name:"<<codecName<<" frame size:"<<frameSize<<" presentationTime:"<<presentationTime.tv_sec<<std::endl;
}

void onConnectionSetup(char *codecName)
{
    std::cout<<"xxxxxxxxxxxxxxxxxxxxxxcodec:"<<codecName<<std::endl;
}

int main(int argc,char *argv[])
{
    rtspPlayer *player = new rtspPlayer();
    player->onFrameData = onFrameArrival;
    player->onConnectionSetup = onConnectionSetup;
    if (player->startRTSP((const char *)"rtsp://10.170.0.2:8554/slamtv60.264", "username1", "password1") == OK)
    {
        sleep(3);
        player->stopRTSP();
    }
    delete(player);
}