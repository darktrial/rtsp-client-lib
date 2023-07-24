All:
	$(CXX) -o rtspclient rtspplayer.cpp rtspclient.cpp -I../BasicUsageEnvironment/include -I../UsageEnvironment/include -I../groupsock/include -I../liveMedia/include ../liveMedia/libliveMedia.a ../groupsock/libgroupsock.a ../BasicUsageEnvironment/libBasicUsageEnvironment.a ../UsageEnvironment/libUsageEnvironment.a -lssl -lcrypto -lpthread -lavformat -lavcodec -lavutil

clean:
	rm -rf rtspclient
