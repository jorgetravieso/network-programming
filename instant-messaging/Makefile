all:	imclient imserver

imclient: imclient.cpp
	g++ -Wall $< -o $@

imserver: imserver.cpp
	g++ -Wall $< -o $@

clean:
	rm -f imclient imserver *.o *~ core

