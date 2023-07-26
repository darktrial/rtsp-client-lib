#include "rtspclient.hh"


int main(int argc,char *argv[])
{
    rtspPlayer *player = new rtspPlayer();
    if (player->startRTSP((const char *)"rtsp://10.170.0.2:8554/surfing.265", "username1", "password1") == OK)
    {
        sleep(3);
        player->stopRTSP();
    }
    delete(player);
}