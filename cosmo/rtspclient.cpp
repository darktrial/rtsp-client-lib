#include "rtspclient.hh"

void continueAfterDESCRIBE(RTSPClient *rtspClient, int resultCode, char *resultString);
void continueAfterSETUP(RTSPClient *rtspClient, int resultCode, char *resultString);
void continueAfterPLAY(RTSPClient *rtspClient, int resultCode, char *resultString);
void subsessionAfterPlaying(void *clientData);
void subsessionByeHandler(void *clientData, char const *reason);
void streamTimerHandler(void *clientData);
void openURL(UsageEnvironment &env, char const *progName, RTSPClient *rtspClient, char const *rtspURL);
void setupNextSubsession(RTSPClient *rtspClient);
void shutdownStream(RTSPClient *rtspClient, int exitCode = 1);

UsageEnvironment &operator<<(UsageEnvironment &env, const RTSPClient &rtspClient)
{
  return env << "[URL:\"" << rtspClient.url() << "\"]: ";
}

UsageEnvironment &operator<<(UsageEnvironment &env, const MediaSubsession &subsession)
{
  return env << subsession.mediumName() << "/" << subsession.codecName();
}

void usage(UsageEnvironment &env, char const *progName)
{
  env << "Usage: " << progName << " <rtsp-url-1> ... <rtsp-url-N>\n";
  env << "\t(where each <rtsp-url-i> is a \"rtsp://\" URL)\n";
}

/*int main(int argc, char **argv)
{
  TaskScheduler *scheduler = BasicTaskScheduler::createNew();
  UsageEnvironment *env = BasicUsageEnvironment::createNew(*scheduler);
  if (argc < 2)
  {
    usage(*env, argv[0]);
    return 1;
  }
  for (int i = 1; i <= argc - 1; ++i)
  {
    openURL(*env, argv[0], argv[i]);
  }
  env->taskScheduler().doEventLoop(&eventLoopWatchVariable);
  return 0;

}*/

void rtspPlayer::playRTSP(char *url)
{
  TaskScheduler *scheduler = BasicTaskScheduler::createNew();
  UsageEnvironment *env = BasicUsageEnvironment::createNew(*scheduler);
  // av_register_all();
  // avcodec_register_all();
  RTSPClient *rtspClient = ourRTSPClient::createNew(*env, url, RTSP_CLIENT_VERBOSITY_LEVEL, "RTSP Client");
  if (rtspClient == NULL)
  {
    // env << "Failed to create a RTSP client for URL \"" << rtspURL << "\": " << env.getResultMsg() << "\n";
    std::cout << "RTSP Client: Failed to create a RTSP client for URL \"" << url << "\": " << env->getResultMsg() << "\n";
    return;
  }
  openURL(*env, "RTSP Client", rtspClient, url);
  env->taskScheduler().doEventLoop(&watchVariable);
  shutdownStream(rtspClient);
  env->reclaim();
  env = NULL;
  delete scheduler;
  scheduler = NULL;

  std::cout << "RTSP Client: End of stream\n";
  return;
}

void rtspPlayer::startRTSP(char *url)
{
  watchVariable = 0;
  std::thread t1(&rtspPlayer::playRTSP, this, url);
  t1.detach();
}

void rtspPlayer::stopRTSP()
{
  watchVariable = 1;
}

static unsigned rtspClientCount = 0;

void openURL(UsageEnvironment &env, char const *progName, RTSPClient *rtspClient, char const *rtspURL)
{
  ++rtspClientCount;
  rtspClient->sendDescribeCommand(continueAfterDESCRIBE);
}

void continueAfterDESCRIBE(RTSPClient *rtspClient, int resultCode, char *resultString)
{
  do
  {
    UsageEnvironment &env = rtspClient->envir();
    StreamClientState &scs = ((ourRTSPClient *)rtspClient)->scs;

    if (resultCode != 0)
    {
      env << *rtspClient << "Failed to get a SDP description: " << resultString << "\n";
      delete[] resultString;
      break;
    }

    char *const sdpDescription = resultString;
    env << *rtspClient << "Got a SDP description:\n"
        << sdpDescription << "\n";

    scs.session = MediaSession::createNew(env, sdpDescription);
    delete[] sdpDescription;
    if (scs.session == NULL)
    {
      env << *rtspClient << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
      break;
    }
    else if (!scs.session->hasSubsessions())
    {
      env << *rtspClient << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
      break;
    }

    scs.iter = new MediaSubsessionIterator(*scs.session);
    setupNextSubsession(rtspClient);
    return;
  } while (0);
  shutdownStream(rtspClient);
}

void setupNextSubsession(RTSPClient *rtspClient)
{
  UsageEnvironment &env = rtspClient->envir();
  StreamClientState &scs = ((ourRTSPClient *)rtspClient)->scs;

  scs.subsession = scs.iter->next();
  if (scs.subsession != NULL)
  {
    if (!scs.subsession->initiate())
    {
      env << *rtspClient << "Failed to initiate the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
      setupNextSubsession(rtspClient);
    }
    else
    {
      env << *rtspClient << "Initiated the \"" << *scs.subsession << "\" subsession (";
      if (scs.subsession->rtcpIsMuxed())
      {
        env << "client port " << scs.subsession->clientPortNum();
      }
      else
      {
        env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum() + 1;
      }
      env << ")\n";
      rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP, False, REQUEST_STREAMING_OVER_TCP);
    }
    return;
  }
  if (scs.session->absStartTime() != NULL)
  {
    rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY, scs.session->absStartTime(), scs.session->absEndTime());
  }
  else
  {
    scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
    rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY);
  }
}

void continueAfterSETUP(RTSPClient *rtspClient, int resultCode, char *resultString)
{
  do
  {
    UsageEnvironment &env = rtspClient->envir();
    StreamClientState &scs = ((ourRTSPClient *)rtspClient)->scs;

    if (resultCode != 0)
    {
      env << *rtspClient << "Failed to set up the \"" << *scs.subsession << "\" subsession: " << resultString << "\n";
      break;
    }

    env << *rtspClient << "Set up the \"" << *scs.subsession << "\" subsession (";
    if (scs.subsession->rtcpIsMuxed())
    {
      env << "client port " << scs.subsession->clientPortNum();
    }
    else
    {
      env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum() + 1;
    }
    env << ")\n";

    scs.subsession->sink = DummySink::createNew(env, *scs.subsession, rtspClient->url());
    if (scs.subsession->sink == NULL)
    {
      env << *rtspClient << "Failed to create a data sink for the \"" << *scs.subsession
          << "\" subsession: " << env.getResultMsg() << "\n";
      break;
    }

    env << *rtspClient << "Created a data sink for the \"" << *scs.subsession << "\" subsession\n";
    scs.subsession->miscPtr = rtspClient;
    scs.subsession->sink->startPlaying(*(scs.subsession->readSource()), subsessionAfterPlaying, scs.subsession);
    if (scs.subsession->rtcpInstance() != NULL)
    {
      scs.subsession->rtcpInstance()->setByeWithReasonHandler(subsessionByeHandler, scs.subsession);
    }
  } while (0);
  delete[] resultString;
  setupNextSubsession(rtspClient);
}

void continueAfterPLAY(RTSPClient *rtspClient, int resultCode, char *resultString)
{
  Boolean success = False;

  do
  {
    UsageEnvironment &env = rtspClient->envir();
    StreamClientState &scs = ((ourRTSPClient *)rtspClient)->scs;

    if (resultCode != 0)
    {
      env << *rtspClient << "Failed to start playing session: " << resultString << "\n";
      break;
    }
    if (scs.duration > 0)
    {
      unsigned const delaySlop = 2;
      scs.duration += delaySlop;
      unsigned uSecsToDelay = (unsigned)(scs.duration * 1000000);
      scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc *)streamTimerHandler, rtspClient);
    }

    env << *rtspClient << "Started playing session";
    if (scs.duration > 0)
    {
      env << " (for up to " << scs.duration << " seconds)";
    }
    env << "...\n";

    success = True;
  } while (0);
  delete[] resultString;

  if (!success)
  {
    shutdownStream(rtspClient);
  }
}

void subsessionAfterPlaying(void *clientData)
{
  MediaSubsession *subsession = (MediaSubsession *)clientData;
  RTSPClient *rtspClient = (RTSPClient *)(subsession->miscPtr);

  Medium::close(subsession->sink);
  subsession->sink = NULL;

  MediaSession &session = subsession->parentSession();
  MediaSubsessionIterator iter(session);
  while ((subsession = iter.next()) != NULL)
  {
    if (subsession->sink != NULL)
      return;
  }
  shutdownStream(rtspClient);
}

void subsessionByeHandler(void *clientData, char const *reason)
{
  MediaSubsession *subsession = (MediaSubsession *)clientData;
  RTSPClient *rtspClient = (RTSPClient *)subsession->miscPtr;
  UsageEnvironment &env = rtspClient->envir(); // alias

  env << *rtspClient << "Received RTCP \"BYE\"";
  if (reason != NULL)
  {
    env << " (reason:\"" << reason << "\")";
    delete[] (char *)reason;
  }
  env << " on \"" << *subsession << "\" subsession\n";
  subsessionAfterPlaying(subsession);
}

void streamTimerHandler(void *clientData)
{
  ourRTSPClient *rtspClient = (ourRTSPClient *)clientData;
  StreamClientState &scs = rtspClient->scs;

  scs.streamTimerTask = NULL;
  shutdownStream(rtspClient);
}

void shutdownStream(RTSPClient *rtspClient, int exitCode)
{
  UsageEnvironment &env = rtspClient->envir();
  StreamClientState &scs = ((ourRTSPClient *)rtspClient)->scs;

  if (scs.session != NULL)
  {
    Boolean someSubsessionsWereActive = False;
    MediaSubsessionIterator iter(*scs.session);
    MediaSubsession *subsession;

    while ((subsession = iter.next()) != NULL)
    {
      if (subsession->sink != NULL)
      {
        Medium::close(subsession->sink);
        subsession->sink = NULL;

        if (subsession->rtcpInstance() != NULL)
        {
          subsession->rtcpInstance()->setByeHandler(NULL, NULL);
        }

        someSubsessionsWereActive = True;
      }
    }

    if (someSubsessionsWereActive)
    {
      rtspClient->sendTeardownCommand(*scs.session, NULL);
    }
  }

  env << *rtspClient << "Closing the stream.\n";
  Medium::close(rtspClient);
  if (--rtspClientCount == 0)
  {
    exit(exitCode);
  }
}

ourRTSPClient *ourRTSPClient::createNew(UsageEnvironment &env, char const *rtspURL, int verbosityLevel, char const *applicationName, portNumBits tunnelOverHTTPPortNum)
{
  return new ourRTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
}

ourRTSPClient::ourRTSPClient(UsageEnvironment &env, char const *rtspURL,
                             int verbosityLevel, char const *applicationName, portNumBits tunnelOverHTTPPortNum)
    : RTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, -1)
{
}

ourRTSPClient::~ourRTSPClient()
{
}

StreamClientState::StreamClientState() : iter(NULL), session(NULL), subsession(NULL), streamTimerTask(NULL), duration(0.0)
{
}

StreamClientState::~StreamClientState()
{
  delete iter;
  if (session != NULL)
  {
    UsageEnvironment &env = session->envir(); // alias

    env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
    Medium::close(session);
  }
}

DummySink *DummySink::createNew(UsageEnvironment &env, MediaSubsession &subsession, char const *streamId)
{
  return new DummySink(env, subsession, streamId);
}

DummySink::DummySink(UsageEnvironment &env, MediaSubsession &subsession, char const *streamId)
    : MediaSink(env),
      fSubsession(subsession)
{
  AVCodec *codec;
  if (strcmp(fSubsession.codecName(), "H264") == 0)
    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
  else if (strcmp(fSubsession.codecName(), "H265") == 0)
    codec = avcodec_find_decoder(AV_CODEC_ID_H265);
  else
  {
    env << "Failed to find the suitable decoder"
        << "\n";
    return;
  }
  if (!codec)
  {
    env << "Failed to find the suitable decoder"
        << "\n";
    return;
  }
  pCodecCtx = avcodec_alloc_context3(codec);
  if (!pCodecCtx)
  {
    env << "Failed to find the suitable context"
        << "\n";
    return;
  }
  // Open the codec
  if (avcodec_open2(pCodecCtx, codec, NULL) < 0)
  {
    env << "Failed to open the codec"
        << "\n";
    avcodec_free_context(&pCodecCtx);
    return;
  }
  fStreamId = strDup(streamId);
  fReceiveBuffer = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];
}

DummySink::~DummySink()
{
  delete[] fReceiveBuffer;
  delete[] fStreamId;
  avcodec_free_context(&pCodecCtx);
}

void DummySink::afterGettingFrame(void *clientData, unsigned frameSize, unsigned numTruncatedBytes,
                                  struct timeval presentationTime, unsigned durationInMicroseconds)
{
  DummySink *sink = (DummySink *)clientData;

  sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}
/*bool DummySink::isIFrame(unsigned char *frameData, unsigned frameSize)
{
  // H.264 NAL unit starts with 0x00 0x00 0x00 0x01 (start code)
  // The first byte after start code is the NAL unit type (5 bits)
  // 5th bit is 0 for I-frame, and 1 for P-frame
  return (frameData[0] & 0x1F) == 0x05; // I-frame NAL unit type
}*/
bool DummySink::isIFrame(AVPacket *packet)
{
  // Decode the packet to get the frame type
  int got_frame;
  AVFrame *frame = av_frame_alloc();
  // avcodec_decode_video2(pCodecCtx, frame, &got_frame, packet);
  avcodec_send_packet(pCodecCtx, packet);
  got_frame = avcodec_receive_frame(pCodecCtx, frame);

  // Get the frame type from the decoded frame
  bool isIframe = false;
  if (got_frame != AVERROR(EAGAIN) && got_frame != AVERROR_EOF)
  {
    isIframe = frame->pict_type == AV_PICTURE_TYPE_I;
  }
  av_frame_free(&frame);
  return isIframe;
}
void DummySink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
                                  struct timeval presentationTime, unsigned /*durationInMicroseconds*/)
{
#ifdef DEBUG_PRINT_EACH_RECEIVED_FRAME
  if (fStreamId != NULL)
    envir() << "Stream \"" << fStreamId << "\"; ";
  envir() << fSubsession.mediumName() << "/" << fSubsession.codecName() << "\tReceived " << frameSize << " bytes";
  if (numTruncatedBytes > 0)
    envir() << " (with " << numTruncatedBytes << " bytes truncated)";
  char uSecsStr[6 + 1]; // used to output the 'microseconds' part of the presentation time
  sprintf(uSecsStr, "%06u", (unsigned)presentationTime.tv_usec);
  envir() << ".\tPresentation time: " << (int)presentationTime.tv_sec << "." << uSecsStr;
  if (fSubsession.rtpSource() != NULL && !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP())
  {
    envir() << "!"; // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
  }
#ifdef DEBUG_PRINT_NPT
  envir() << "\tNPT: " << fSubsession.getNormalPlayTime(presentationTime);
#endif
  envir() << "\n";
#endif
  char uSecsStr[7];
  // bool isIframe = isIFrame(fReceiveBuffer, frameSize);
  u_int8_t start_code[4] = {0x00, 0x00, 0x00, 0x01};
  u_int8_t *frameData = (u_int8_t *)malloc(frameSize + 4);
  AVPacket packet;

  memcpy(frameData, start_code, 4);
  memcpy(frameData + 4, fReceiveBuffer, frameSize);
  // av_init_packet(&packet);
  av_new_packet(&packet, frameSize + 4);
  packet.data = frameData; //(uint8_t*)fReceiveBuffer;
  packet.size = frameSize + 4;
  bool isIframe = isIFrame(&packet);
  snprintf(uSecsStr, 7, "%06u", (unsigned)presentationTime.tv_usec);

  // Print the result
  if (isIframe)
  {
    envir() << " codec:" << fSubsession.codecName() << " I-Frame "
            << " size:" << frameSize << " bytes "
            << "presentation time:" << (int)presentationTime.tv_sec << "." << uSecsStr << "\n";
  }
  else
  {
    envir() << " codec:" << fSubsession.codecName() << " P-frame "
            << " size:" << frameSize << " bytes "
            << " presentation time:" << (int)presentationTime.tv_sec << "." << uSecsStr << "\n";
  }
  // av_packet_free(&packet);
  // av_free_packet(&packet);
  av_packet_unref(&packet);
  free(frameData);
  continuePlaying();
}

Boolean DummySink::continuePlaying()
{
  if (fSource == NULL)
    return False;
  fSource->getNextFrame(fReceiveBuffer, DUMMY_SINK_RECEIVE_BUFFER_SIZE,
                        afterGettingFrame, this,
                        onSourceClosure, this);
  return True;
}
