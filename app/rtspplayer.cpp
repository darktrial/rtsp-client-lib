#include "rtspClientHelper.hh"

#define FFMPEG_HELPER 1

#ifdef FFMPEG_HELPER
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}
AVCodecContext *pCodecCtx = NULL;
bool isIFrame(AVPacket *packet)
{
    int got_frame;
    if (!pCodecCtx)
        return false;
    AVFrame *frame = av_frame_alloc();

    avcodec_send_packet(pCodecCtx, packet);
    got_frame = avcodec_receive_frame(pCodecCtx, frame);

    bool isIframe = false;
    if (got_frame != AVERROR(EAGAIN) && got_frame != AVERROR_EOF)
    {
        isIframe = frame->pict_type == AV_PICTURE_TYPE_I;
    }
    av_frame_free(&frame);
    return isIframe;
}
#endif

void onFrameArrival(unsigned char *videoData, const char *codecName, unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime)
{
#ifdef FFMPEG_HELPER
    char uSecsStr[7];
    u_int8_t start_code[4] = {0x00, 0x00, 0x00, 0x01};
    u_int8_t *frameData;
    AVPacket packet;

    if (strcmp(codecName, "JPEG") == 0)
    {
        snprintf(uSecsStr, 7, "%06u", (unsigned)presentationTime.tv_usec);
        std::cout << "codec:" << codecName << " size:" << frameSize << " presentation time:" << (int)presentationTime.tv_sec << "." << uSecsStr << "\n";
        return;
    }
    frameData = (u_int8_t *)malloc(frameSize + 4);
    memcpy(frameData, start_code, 4);
    memcpy(frameData + 4, videoData, frameSize);
    av_new_packet(&packet, frameSize + 4);
    packet.data = frameData; //(uint8_t*)fReceiveBuffer;
    packet.size = frameSize + 4;
    bool isIframe = isIFrame(&packet);
    snprintf(uSecsStr, 7, "%06u", (unsigned)presentationTime.tv_usec);
    if (isIframe)
    {
        std::cout << " codec:" << codecName << " I-Frame "
                  << " size:" << frameSize << " bytes "
                  << "presentation time:" << (int)presentationTime.tv_sec << "." << uSecsStr << "\n";
    }
    else
    {
        std::cout << " codec:" << codecName << " P-frame "
                  << " size:" << frameSize << " bytes "
                  << " presentation time:" << (int)presentationTime.tv_sec << "." << uSecsStr << "\n";
    }
    free(frameData);
    av_packet_unref(&packet);
#else
    char uSecsStr[7];
    snprintf(uSecsStr, 7, "%06u", (unsigned)presentationTime.tv_usec);
    std::cout << "codec name:" << codecName << " frame size:" << frameSize << " presentationTime:" << presentationTime.tv_sec << "." << uSecsStr << std::endl;
#endif
}

void onConnectionSetup(char *codecName)
{
    std::cout << "codec:" << codecName << std::endl;
#ifdef FFMPEG_HELPER
    AVCodec *codec;
    if (strcmp(codecName, "H264") == 0)
        codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    else if (strcmp(codecName, "H265") == 0)
        codec = avcodec_find_decoder(AV_CODEC_ID_H265);
    else
    {
        std::cout << "Failed to find the suitable decoder"
                  << "\n";
        return;
    }
    if (!codec)
    {
        std::cout << "Failed to find the suitable decoder"
                  << "\n";
        return;
    }
    pCodecCtx = avcodec_alloc_context3(codec);
    if (!pCodecCtx)
    {
        std::cout << "Failed to find the suitable context"
                  << "\n";
        return;
    }
    // Open the codec
    if (avcodec_open2(pCodecCtx, codec, NULL) < 0)
    {
        std::cout << "Failed to open the codec"
                  << "\n";
        avcodec_free_context(&pCodecCtx);
        return;
    }
#endif
}

int main(int argc, char *argv[])
{
    rtspPlayer *player = new rtspPlayer();
    player->onFrameData = onFrameArrival;
    player->onConnectionSetup = onConnectionSetup;
    if (player->startRTSP((const char *)"rtsp://10.170.0.2:8554/slamtv60.264", false, "username1", "password1") == OK)
    {
        sleep(3);
        player->stopRTSP();
    }
    delete (player);
#ifdef FFMPEG_HELPER
    avcodec_free_context(&pCodecCtx);
#endif
}