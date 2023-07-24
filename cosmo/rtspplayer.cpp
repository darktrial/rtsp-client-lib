#include "rtspclient.hh"


int main(int argc,char *argv[])
{
    rtspPlayer *player = new rtspPlayer();
    player->startRTSP((char *)"rtsp://10.170.0.2:8554/surfing.265");
    sleep(3);
    player->stopRTSP();
    sleep(1);
}