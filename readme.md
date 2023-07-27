# rtsp-client-lib
## A live555 wrapper library for making RTSP client easier.

### To use
#### 1. Include rtspClientHelper.hh 
#### 2. RTSP client APIs
#####    2.1 Create rtspPlayer object
   `rtspPlayer *player = new rtspPlayer();`
#####    2.2 Call startRTSP with URL, overTCP(true or false), username(optional), password(optional)
   `player->startRTSP((const char *)"rtsp://10.170.0.2:8554/slamtv60.264", false, "username1", "password1")`
#####    2.3 register call back to get video frames 
   `player->onFrameData = onFrameArrival;`

Please refer to app folder about how to use this wrapper.