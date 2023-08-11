#include "rtspClientHelper.hh"

#define FFMPEG_HELPER 1

#ifdef FFMPEG_HELPER
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
extern int ffpg_get_minqp();
extern int ffpg_get_maxqp();
extern double ffpg_get_avgqp();
}
AVCodecContext *pCodecCtx = NULL;
typedef struct _timeRecords
{
    long long starttime;
    long long endtime;
    int numberOfFrames;
    int sizeOfFrames;
} timeRecords;
timeRecords tr;
void getFrameStatics(const char *codecName, AVPacket *packet, bool &isIframe, int &min_qp, int &max_qp, double &avg_qp)
{
    int got_frame;
    int ret = 0;
    int qp_sum = 0;
    AVBufferRef *qp_table_buf = NULL;
    unsigned char *qscale_table = NULL;

    if (!pCodecCtx)
        return;

    AVFrame *frame = av_frame_alloc();
    int x, y;
    //printf("mb_width:%d mb_height:%d mb_stride:%d\n", mb_width, mb_height, mb_stride);
    avcodec_send_packet(pCodecCtx, packet);
    got_frame = avcodec_receive_frame(pCodecCtx, frame);

    if (got_frame == 0)
    {
        isIframe = frame->pict_type == AV_PICTURE_TYPE_I;
        if (strcmp(codecName,"H264") == 0)
        {
            int mb_width = (pCodecCtx->width + 15) / 16;
            int mb_height = (pCodecCtx->height + 15) / 16;
            int mb_stride = mb_width + 1;
            qp_table_buf = av_buffer_ref(frame->qp_table_buf);
            qscale_table = qp_table_buf->data;
            if (qscale_table!= NULL)
            {
                for (y = 0; y < mb_height; y++)
                {
                    for (x = 0; x < mb_width; x++)
                    {
                        qp_sum += qscale_table[x + y * mb_stride];
                        if (qscale_table[x + y * mb_stride] > max_qp)
                        {
                            max_qp = qscale_table[x + y * mb_stride];
                        }
                        if (qscale_table[x + y * mb_stride] < min_qp || min_qp == 0)
                        {
                            min_qp = qscale_table[x + y * mb_stride];
                        }
                    }
                }
                avg_qp = (double)qp_sum / (mb_height * mb_width);
            }
        }
        else
        {
            min_qp=ffpg_get_minqp();
            max_qp=ffpg_get_maxqp();
            avg_qp=ffpg_get_avgqp();
        }
    }
    av_buffer_unref(&qp_table_buf);
    av_frame_free(&frame);
}

long long getCurrentTimeMicroseconds()
{
#ifdef _WIN32
    FILETIME currentTime;
    GetSystemTimePreciseAsFileTime(&currentTime);

    ULARGE_INTEGER uli;
    uli.LowPart = currentTime.dwLowDateTime;
    uli.HighPart = currentTime.dwHighDateTime;

    return uli.QuadPart / 10LL;
#else
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);

    return (long long)currentTime.tv_sec * 1000000LL + currentTime.tv_usec;
#endif
}

#endif

void onFrameArrival(unsigned char *videoData, const char *codecName, unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, void *privateData)
{
#ifdef FFMPEG_HELPER
    char uSecsStr[7];
    int min_qp = 0;
    int max_qp = 0;
    int qp_sum = 0;
    double avg_qp = 0;
    bool isIframe = false;
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
    packet.data = frameData; 
    packet.size = frameSize + 4;
    // bool isIframe = isIFrame(&packet);
    getFrameStatics(codecName,&packet, isIframe, min_qp, max_qp, avg_qp);
    snprintf(uSecsStr, 7, "%06u", (unsigned)presentationTime.tv_usec);
    if (isIframe)
    {
        std::cout << " codec:" << codecName << " I-Frame "
                  << " size:" << frameSize << " bytes "
                  << " min_qp:" << min_qp << " max_qp:" << max_qp << " avg_qp:" << avg_qp
                  << "presentation time:" << (int)presentationTime.tv_sec << "." << uSecsStr << "\n";
        if (tr.starttime == 0)
        {
            tr.starttime = getCurrentTimeMicroseconds();
            tr.numberOfFrames++;
            tr.sizeOfFrames = frameSize;
        }
        else
        {
            tr.endtime = getCurrentTimeMicroseconds();
            std::cout << "FPS:" << (tr.numberOfFrames * 1000000.0) / (tr.endtime - tr.starttime) << "\n";
            std::cout << "bit rate:" << (tr.sizeOfFrames) / ((tr.endtime - tr.starttime) / 1000000.0) / 1024 << " KB/s \n";
            tr.starttime = tr.endtime;
            tr.numberOfFrames = 1;
            tr.sizeOfFrames = frameSize;
        }
    }
    else
    {
        std::cout << " codec:" << codecName << " P-frame "
                  << " size:" << frameSize << " bytes "
                  << " min_qp:" << min_qp << " max_qp:" << max_qp << " avg_qp:" << avg_qp
                  << " presentation time:" << (int)presentationTime.tv_sec << "." << uSecsStr << "\n";
        if (tr.starttime != 0)
        {
            tr.numberOfFrames++;
            tr.sizeOfFrames += frameSize;
        }
    }
    free(frameData);
    av_packet_unref(&packet);
#else
    char uSecsStr[7];
    snprintf(uSecsStr, 7, "%06u", (unsigned)presentationTime.tv_usec);
    std::cout << "codec name:" << codecName << " frame size:" << frameSize << " presentationTime:" << presentationTime.tv_sec << "." << uSecsStr << std::endl;
#endif
}

void onConnectionSetup(char *codecName, void *privateData)
{
    std::cout << "codec:" << codecName << std::endl;
    if (privateData != NULL)
        printf("%s\n", (const char *)privateData);
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
    const char *s = "12345";
    rtspPlayer *player = new rtspPlayer((void *)s);
    player->onFrameData = onFrameArrival;
    player->onConnectionSetup = onConnectionSetup;
    if (player->startRTSP((const char *)"rtsp://10.170.0.2:8554/slamtv60.264", false, "username1", "password1") == OK)
    {
        sleep(3);
        player->stopRTSP();
    }
    delete (player);
#ifdef FFMPEG_HELPER
    /*if (pCodecCtx->extradata != NULL)
        free(pCodecCtx->extradata);*/
    if (pCodecCtx != NULL)
    {
        pCodecCtx->extradata = NULL;
        avcodec_free_context(&pCodecCtx);
    }
    tr.starttime = 0;
    tr.endtime = 0;
    tr.numberOfFrames = 0;
    tr.sizeOfFrames = 0;
#endif
}