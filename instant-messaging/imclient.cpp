/*
Instant Messaging Application
*** Client ***
Sends messages to clients connected to the server of a maximum size of 2048 bytes
Receive messages from users
*/


#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <regex>

#define BUFFER_SIZE 2048
#define ACCEPTED "ACCEPTED"
#define DENIED "DENIED"
#define INPUT_REGEX_FORMAT "@[0-9a-zA-Z]+(,[0-9a-zA-Z]+)*:.+"

using namespace std;


void process_server_msg(string message);
void send_message(int sockfd, string message);
vector<string> &split(const string &s, char delim, vector<string> &elems);
vector<string> split(string &s, char delim);
void clean_up();
void syserr(string msg) { perror(msg.c_str()); clean_up(); exit(-1); }
void signal_handler(int param);


int sockfd, portno, n;
struct hostent* server;
struct sockaddr_in serv_addr;
char recv_buffer[BUFFER_SIZE];    
regex rx(INPUT_REGEX_FORMAT);

int main(int argc, char* argv[])
{
	signal(SIGTERM, signal_handler);                                                            
	//read args
	if(argc != 4) {
		fprintf(stderr, "Usage: %s <server-ip> <server-port> <client-username>\n", argv[0]);
		return 1;
	}
	server = gethostbyname(argv[1]);
	if(!server) {
		fprintf(stderr, "ERROR: no such host: %s\n", argv[1]);
		return 2;
	}
	portno = atoi(argv[2]);
	string username = argv[3];

	if (username ==  "all"){
		cout << "invalid username \'all\', which is a reserved keyword. exiting...\n";
		return 1;
	}
	sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sockfd < 0) syserr("can't open socket");
	
	//creating tcp connection                                 
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr = *((struct in_addr*)server->h_addr);
	serv_addr.sin_port = htons(portno);
	
	if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
		syserr("can't connect to server. exiting...");
	cout << "connection established...\n";

	//send username to see if this name is not taken in the server
	n = send(sockfd, username.c_str(), username.length(), 0);
	if(n < 0) syserr("can't send to server");


	//receive server's confirmation whether the username is valid and the user can join
	n = recv(sockfd, recv_buffer, BUFFER_SIZE, 0);
	if(n < 0) syserr("can't receive from server");
	recv_buffer[n] = 0;


	//not accepted
	if(strncmp(DENIED, recv_buffer, 6) == 0){
		cout << "invalid username " << username << " is a already taken in the chatroom. exiting...\n";
		return 1;
	}

	n = recv(sockfd, recv_buffer, BUFFER_SIZE, 0);
	if(n < 0) syserr("can't receive from server");
	recv_buffer[n] = 0;
	cout << recv_buffer << endl;									//print users online

	//prompt
	cout << "im> ";
	fflush(stdout); 
	fd_set readset;
	
	for(;;){                                              
		FD_ZERO(&readset);                                  		//do select without timeout
		FD_SET(sockfd, &readset);
		FD_SET(STDIN_FILENO, &readset);
		n = select(sockfd+1, &readset, NULL, NULL, NULL);
		if(n < 0) syserr("error on select()");
		if (FD_ISSET(sockfd, &readset)){                   			//incoming server message    
			n = recv(sockfd, recv_buffer, BUFFER_SIZE, 0);
			if(n == 0) {
				cout << "the server died... exiting" << endl;
				break;
			}
			if(n < 0) syserr("can't receive from server");
			recv_buffer[n] = 0;
			string message(recv_buffer);
			
			if(message.substr(0,3) == "msg"){
				process_server_msg(message);
			}
			else{
				cout << message << endl;                      		 //server broadcast
			}
			cout << "im> ";
			fflush(stdout);

		}
		if(FD_ISSET(STDIN_FILENO, &readset)){            			 //user message
			string input;
			getline(cin, input);         


			if(input == "quit"){									//user typed quit
				break;
			}
		
			if(!regex_match(input.begin(), input.end(), rx)){		//check user input	
				cout << "wrong message format, try: @<users>: <message>" << endl;
				cout << "im> ";
				fflush(stdout); 
				continue;
			}
			vector<string> lines = split(input, ':');				//transform the stdin input into the server receiving format
			if(lines.size() != 2) {
				cout << "invalid input "<< endl;
				break;
			}
			string users = lines.at(0).substr(1);
			string message = "msg @ " + users + "\n" + lines.at(1); 

			send_message(sockfd, message);
			cout << "im> ";
			fflush(stdout); 
		}

	}

	close(sockfd);
	return 0;
}


//sends a message to a file descritor
void send_message(int sockfd, string message)
{
    if(message.length() > BUFFER_SIZE){
      message = message.substr(0, BUFFER_SIZE);
    }
    if(send(sockfd, message.c_str(), message.length(), 0)  < 0) 
        syserr("can't send to client");
}

//parses the server msg
void process_server_msg(string message)
{
	vector<string> lines = split(message, '\n');
	if(lines.size() != 2){
		cout << "invalid message format: " << message << endl; 
		return;
	}
	cout << "@" << lines.at(0).substr(6) << ":" << lines.at(1) << endl;
}



/*
GENERAL METHODS
*/

//split a string to a vector from a char delimiter 
vector<string> split(string &s, char delim) {
	vector<string> elems;
	split(s, delim, elems);
	return elems;
}
vector<string> &split(const string &s, char delim, vector<string> &elems) {
	stringstream ss(s);
	string item;
	while (getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}
void clean_up(){
	cout << "closing the socket" << endl;
	close(sockfd);
}
void signal_handler(int param){
	if(param == SIGTERM){
		clean_up();
	}
	exit(0);
}


