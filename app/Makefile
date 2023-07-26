INCLUDES = -I${PWD}/../live555/BasicUsageEnvironment/include -I${PWD}/../live555/UsageEnvironment/include -I${PWD}/../live555/groupsock/include -I${PWD}/../live555/liveMedia/include -I${PWD}/../cosmo/
LIBS = -L${PWD}/../live555/liveMedia/libliveMedia.a -L${PWD}/../live555/groupsock/libgroupsock.a -L${PWD}/../live555/BasicUsageEnvironment/libBasicUsageEnvironment.a -L${PWD}/../live555/UsageEnvironment/libUsageEnvironment.a -lssl -lcrypto -lpthread -lavformat -lavcodec -lavutil
All:
	$(CXX) ${INCLUDES} -o rtspclient rtspplayer.cpp ../cosmo/rtspclient.cpp ${LIBS}
