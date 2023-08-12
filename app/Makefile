INCLUDES = -I ../ffmpeg-4.4.4/install/include -I../live555/BasicUsageEnvironment/include -I../live555/UsageEnvironment/include -I../live555/groupsock/include -I../live555/liveMedia/include -I../cosmo/
LIBS = -L../ffmpeg/install/lib/ ../live555/liveMedia/libliveMedia.a ../live555/groupsock/libgroupsock.a ../live555/BasicUsageEnvironment/libBasicUsageEnvironment.a ../live555/UsageEnvironment/libUsageEnvironment.a -lssl -lcrypto -lpthread ../ffmpeg-4.4.4/install/lib/libavformat.a ../ffmpeg-4.4.4/install/lib/libavcodec.a ../ffmpeg-4.4.4/install/lib/libavutil.a
All:
	$(CXX) ${INCLUDES} -o rtspplayer rtspplayer.cpp ../cosmo/rtspClientHelper.cpp ${LIBS}
clean:
	rm -rf rtspplayer
