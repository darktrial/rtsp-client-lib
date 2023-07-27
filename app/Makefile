INCLUDES = -I../live555/BasicUsageEnvironment/include -I../live555/UsageEnvironment/include -I../live555/groupsock/include -I../live555/liveMedia/include -I../cosmo/
LIBS = ../live555/liveMedia/libliveMedia.a ../live555/groupsock/libgroupsock.a ../live555/BasicUsageEnvironment/libBasicUsageEnvironment.a ../live555/UsageEnvironment/libUsageEnvironment.a -lssl -lcrypto -lpthread -lavformat -lavcodec -lavutil
All:
	$(CXX) ${INCLUDES} -o rtspplayer rtspplayer.cpp ../cosmo/rtspClientHelper.cpp ${LIBS}
clean:
	rm -rf rtspplayer
