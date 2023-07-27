#include "rtspClientHelper.hh"

void onFrameArrival(unsigned char *clientData, const char *codecName, unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime)
{
    char uSecsStr[7];
    snprintf(uSecsStr, 7, "%06u", (unsigned)presentationTime.tv_usec);
    std::cout << "codec name:" << codecName << " frame size:" << frameSize << " presentationTime:" << presentationTime.tv_sec << "." << uSecsStr << std::endl;
}

void onConnectionSetup(char *codecName)
{
    std::cout << "codec:" << codecName << std::endl;
}

int main(int argc, char *argv[])
{
    rtspPlayer *player = new rtspPlayer();
    player->onFrameData = onFrameArrival;
    player->onConnectionSetup = onConnectionSetup;
    if (player->startRTSP((const char *)"rtsp://10.170.0.2:8554/slamtv60.264", "username1", "password1") == OK)
    {
        sleep(3);
        player->stopRTSP();
    }
    delete (player);
}