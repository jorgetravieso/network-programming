/*
Instant Messaging Application
*** Server ***
Receive messages from clients of a maximum size of 2048 bytes
Forward those messages to their corresponding destinations
*/

#include <stdio.h>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>

#define BUFFER_SIZE 2048
#define ACCEPTED "ACCEPTED"
#define DENIED "DENIED"


using namespace std;



string connected_users( map<int, string>, bool);
void send_broadcast(int sckt_to_exclude, string message, map<int, string>);
void process_client_msg(string, string, map<string, int>, map<int, string>);
void send_message_simple(int, string);
void send_message(int,string, int, string);
vector<string> &split(const string &s, char delim, vector<string> &elems);
vector<string> split(string &s, char delim);
void clean_up();
void syserr(string msg) { perror(msg.c_str()); clean_up(); exit(-1); }
void signal_handler(int param);



int sockfd, newsockfd, portno, n;                                   
struct sockaddr_in serv_addr, clt_addr;
socklen_t addrlen;                                          
char recv_buffer[BUFFER_SIZE];                                //used for receiving thru sockets
map<int, string> scks_map;                                    //maps sockets fds to usernames
map<string, int> users_map;                                   //maps usernames to sockets fds

int main(int argc, char *argv[])
{

  signal(SIGTERM, signal_handler);                            //in case of termination

  if(argc != 2) {                                             //check args
    fprintf(stderr,"Usage: %s <port>\n", argv[0]);
    return 1;
  } 
  portno = atoi(argv[1]);
                                                              //creating socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0); 
  if(sockfd < 0) syserr("can't open socket"); 
  printf("create socket...\n");

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  if(bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) 
    syserr("can't bind");
  printf("bind socket to port %d...\n", portno);

  listen(sockfd, 5);                                          //listening socket
  fd_set readset;                                             //for the select



  for(;;) {
  
    FD_ZERO(&readset);                                           //clear the socket set
    FD_SET(sockfd, &readset);                                    //add the listen socket to the set
    int max_fd = sockfd;

    map<int, string>::iterator itr;                                                      //iterate thru the good sockets and add them to the readset for select
    for (itr = scks_map.begin(); itr != scks_map.end(); itr++) {
     if(itr->first > 0 && !itr->second.empty()) FD_SET(itr->first, &readset);
     if(itr->first > max_fd) max_fd = itr->first;
    }

   n = select( max_fd + 1 , &readset , NULL , NULL , NULL);
   if(n < 0) syserr("error on select()"); 

    if (FD_ISSET(sockfd, &readset)){                                                     //new incomming connection, try to add it to the user maps
      printf("waiting on port %d...\n", portno);
      addrlen = sizeof(clt_addr);
      newsockfd = accept(sockfd, (struct sockaddr*)&clt_addr, &addrlen);
      if(newsockfd < 0)  syserr("new incomming request couldnt be processed");
      n = recv(newsockfd, recv_buffer, BUFFER_SIZE, 0);                                  //get the username
      if(n < 0) syserr("can't receive from client");
      recv_buffer[n] = 0;
      string username(recv_buffer);
      cout << "new user trying to connect: " << username << endl;
      if(users_map[username] == 0 &&  scks_map[newsockfd] == ""){                        //if slot is available, add it to the map
        users_map[username] = newsockfd;  
        scks_map[newsockfd] = username;
        cout << username << " was accepted" << endl;
        n = send(newsockfd, ACCEPTED, strlen(ACCEPTED), 0);                              //and accept the connection
        if(n < 0) syserr("can't send to  client");
        send_message_simple(newsockfd, connected_users(scks_map, true));                 //send the current list of connected users to the new client
        send_broadcast(newsockfd, "join "  + username + "\n" + connected_users(scks_map, false), scks_map);                                        
      }else{                                
        n = send(newsockfd, DENIED, strlen(DENIED), 0);                                  //else deny connection and close fd  
        if(n < 0) syserr("can't send to client"); 
        cout << username << " was denied" << endl;
        close(newsockfd);
      }
    }

    for (itr = scks_map.begin(); itr != scks_map.end(); itr++) {
      if(itr->first > 0 && FD_ISSET(itr->first, &readset)){                          
            
            n = recv(itr->first, recv_buffer, BUFFER_SIZE, 0);                          //recieve from the socket
            if(n < 0) syserr("can't receive from client");                              //error
            if(n == 0){                                                                 //user disconnected
              cout << "some user has disconnected ... \n removing from user map..." << endl;                                               
              close(itr->first);
              string username = itr->second;                                            //close disconnected user socket, clear the map, and and send broadcast
              users_map[username] = 0;  
              itr->second  = "";
              send_broadcast(itr->first, "leave " + username + "\n" + connected_users(scks_map,false), scks_map);    
            }
            else{
                recv_buffer[n]= 0;                                                      //we've got a new message, process it
                string message(recv_buffer);
                process_client_msg(scks_map[itr->first], message, users_map, scks_map);
            }
        }
    }
  }

  close(sockfd); 
  return 0;
}

void send_broadcast(int sckt_to_exclude, string message, map<int, string> scks_map){
  
  cout << "sending broadcast " << message << endl;
  map<int, string>::iterator itr;
  for (itr = scks_map.begin(); itr != scks_map.end(); itr++) {
    if(itr->first != sckt_to_exclude && itr->first != 0){
      send_message_simple(itr->first,message);
    }
  }
}

string connected_users( map<int, string> scks_map, bool send_number){

  string message = "ONLINE: ";
  map<int, string>::iterator itr;
  int count = 0;
  for (itr = scks_map.begin(); itr != scks_map.end(); itr++) {                     //go thru the map and collect the online users
    if(itr->first > 0 && !itr->second.empty()){                                                            //only users only
      message += itr->second;
      message += ", ";
      count++;
    }
  }
  message = message.substr(0, message.length());  
  if(send_number){
    return "Number of connected users is " + to_string(count) + "\n" + message;
  }
  return message.substr(0, message.length());                                     //send the message without the last comma
}

//we assume that the message is a valid command with the right format
void process_client_msg(string sender_username, string message, map<string, int> users_map, map<int, string> scks_map){

    cout << "processing message: " <<message << endl;


    vector<string> lines = split(message, '\n');
    if(lines.size() != 2){
        cout << "invalid message format: " << message << endl; 
        return;
    }
    string users_str = lines.at(0).substr(6);
    vector<string> users = split(users_str, ',');
    for(int i = 0; i < users.size(); i++){
          string message = "msg @ " + sender_username + "\n" + lines.at(1);
          if(users.at(i) == "all"){
            send_broadcast(users_map[sender_username], message, scks_map);
            break;
          }
          send_message(users_map[sender_username], users.at(i), users_map[users.at(i)], message);
    }

}

void send_message_simple(int tosockfd, string message)
{
    if(message.length() > BUFFER_SIZE){
      message = message.substr(0, BUFFER_SIZE);
    }

    if(send(tosockfd, message.c_str(), message.length(), 0)  < 0)           //if we could not send
      cout << "user disconnected " << endl;
}

void send_message(int fromsockfd, string touser,int tosockfd, string message)
{
    if(message.length() > BUFFER_SIZE){
      message = message.substr(0, BUFFER_SIZE);
    }

    if(send(tosockfd, message.c_str(), message.length(), 0)  < 0) {           //if we could not send
      message = "message to " + touser  + " could not be forwarded...";
      if(send(fromsockfd, message.c_str(), message.length(), 0)  < 0){        //send error back to sender
        cout << "the sender and the destination are disconnected..." << endl;
      }   
    }
    
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


vector<string> split(string &s, char delim) 
{
    vector<string> elems;
    split(s, delim, elems);
    return elems;
}

vector<string> &split(const string &s, char delim, vector<string> &elems) 
{
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}


