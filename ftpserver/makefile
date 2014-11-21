all:	ftpclient ftpserver

ftpclient: ftpclient.cpp
	g++ -Wall $< -o $@

ftpserver: ftpserver.cpp
	g++ -Wall $< -o $@

clean:
	rm -f ftpclient ftpserver *.o *~ core

