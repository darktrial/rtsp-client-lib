All:
	$(CXX) -o testclient testclient.cpp rtspclientlib.cpp -I../BasicUsageEnvironment/include -I../UsageEnvironment/include -I../groupsock/include -I../liveMedia/include ../liveMedia/libliveMedia.a ../groupsock/libgroupsock.a ../BasicUsageEnvironment/libBasicUsageEnvironment.a ../UsageEnvironment/libUsageEnvironment.a -lssl -lcrypto -lpthread

clean:
	rm -rf testclient
