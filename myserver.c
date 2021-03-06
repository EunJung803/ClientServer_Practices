#include <stdio.h>
#include <sys/wait.h>     
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <sys/stat.h>
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>     
#include <time.h>   

#define BUF_SIZE 256
#define MAX_BUF_SIZE 1024
#define MAX_CLIENTS 100

static char *HTTP_400 = "HTTP/1.1 400 Bad Request";
static char *HTTP_403 = "HTTP/1.1 403 Forbidden";
static char *HTTP_404 = "HTTP/1.1 404 Not Found";
static char *HTTP_500 = "HTTP/1.1 500 Internal Server Error";
static char *HTTP_200 = "HTTP/1.1 200 OK";

char cur_path[BUF_SIZE];

void request_handler(int);
void print_client_info(struct sockaddr_in* cli_pt);
char* content_type(char *file_path);
void send_error_response(int new_sockfd, int error_code);
void send_content_response(int new_sockfd, char *request_url);


void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
   int sockfd, new_sockfd; //descriptors rturn from socket and accept system calls
   int option = 1;
   int portno;  // port number
   socklen_t clilen;

    /*sockaddr_in: Structure Containing an Internet Address*/
   struct sockaddr_in serv_addr, cli_addr;
   
   if (argc < 2) {
      fprintf(stderr,"ERROR, no port provided\n");
      exit(1);
   }

   /*Check port number*/
   portno = atoi(argv[1]); //atoi converts from String to Integer
   if (portno >= 0 && portno <= 1024) {
      fprintf(stderr,"ERROR, not to use port number 0 ~ 1024\n");
      exit(1);
   }
   
   if(getcwd(cur_path, sizeof(cur_path)) == NULL) {
      fprintf(stderr,"ERROR, Can't get the current folder\n");
      exit(1);
   }

   /*Create a new socket
      AF_INET: Address Domain is Internet 
      SOCK_STREAM: Socket Type is STREAM Socket */
   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   if (sockfd < 0) 
      error("ERROR opening socket");

   bzero((char *) &serv_addr, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY; //for the server the IP address is always the address that the server is running on
   serv_addr.sin_port = htons(portno); //convert from host to network byte order
   
   if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) //Bind the socket to the server address
      error("ERROR on binding");

   listen(sockfd, MAX_CLIENTS); // Listen for socket connections. Backlog queue (connections to wait) is 5
   
   clilen = sizeof(cli_addr);

   while(1) {   //????????? ????????? ??? ????????? while??? ??????
      /*accept function: 
         1) Block until a new connection is established
         2) the new socket descriptor will be used for subsequent communication with the newly connected client.
      */
      new_sockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);  //client ???????????? ???????????? accept?????? ????????? socket descripter??? ????????????
      if (new_sockfd < 0) 
         error("ERROR on accept");

	  //fork child process
      switch(fork()) {
         case -1: //Fork error (0?????? ?????????)
            error("ERROR on fork of child process");
            break;
         case 0:  //Child  (0?????? child process?????? ??????)
            close(sockfd);
            request_handler(new_sockfd);
            exit(0);
         default: //Parent process (parent process)
            signal(SIGCHLD,SIG_IGN);
            close(new_sockfd);
      }
   }
   close(sockfd); 
   return 0; 
}

//Receive & handle the request from a client
void request_handler(int new_sockfd) {
   int recv_bytes;
   char msg_buffer[MAX_BUF_SIZE];
   bzero(msg_buffer, MAX_BUF_SIZE);

   char method[BUF_SIZE];
   bzero(method, BUF_SIZE);
   char req_url[BUF_SIZE];
   bzero(req_url, BUF_SIZE);

   recv_bytes = read(new_sockfd, msg_buffer, MAX_BUF_SIZE); //socket??? ????????? msg buffer??? ????????????

   if (recv_bytes < 0) 
      error("ERROR reading from socket");

   //request msg print
   printf("------------ Requested Message ------------\n"); 
   printf("%s\n",msg_buffer); 

   //request msg ??????
   sscanf(msg_buffer, "%s%s", method, req_url); //sscanf????????? ?????? request msg?????? /??? ??????????????? ????????? ????????? get??? method??? ??????, / ???????????? url??? ?????????
   
   if (strncmp(method, "GET\0",4) == 0) { //method??? get?????? ??????
      printf("Process '%s' request for '%s'\n", method, req_url); //??? ????????? ?????????
      send_content_response(new_sockfd, req_url); //url??? ???????????? ?????? ????????? ????????? ??? ?????? ??????
   }
   else if (req_url == NULL){
      printf("URL is null\n");  //url??? ??? ???????????????
      send_error_response(new_sockfd, 403);  //Send "Forbidden" error to a client -> 403 ?????? ?????????
   }
   else {
      printf("%s: Not supported request method\n", method); //method??? get??? ????????? ????????????
      send_error_response(new_sockfd, 400); //Send "Bad request" error to a client -> 400 ?????? ?????????
   }

   close(new_sockfd);   // ??????
}


//????????? ????????? ??????
void send_error_response(int new_sockfd, int error_code) {
	char error_msg[BUF_SIZE];
   bzero(error_msg, BUF_SIZE);
   int send_bytes;

	switch(error_code)
    //?????? ????????? ????????? ?????? ???????????? ???????????? ????????? ????????? ??????
	{
		case 400: 
         snprintf(error_msg, sizeof(error_msg), "%s\r\nContent-type:text/html\r\n\r\n<html><head><title>400 Bad Request</title></head><body><h1>Bad Request</h1></html>", HTTP_400); 
			break;
		case 403: 
         snprintf(error_msg, sizeof(error_msg), "%s\r\nContent-type:text/html\r\n\r\n<html><head><title>403 Forbidden</title></head><body><h1>Forbidden</h1></html>", HTTP_403); 
			break;
		case 404: 
         snprintf(error_msg, sizeof(error_msg), "%s\r\nContent-type:text/html\r\n\r\n<html><head><title>404 Not Found</title></head><body><h1>Not Found</h1></html>", HTTP_404); 
			break;
      default: 
         snprintf(error_msg, sizeof(error_msg), "%s\r\nContent-type:text/html\r\n\r\n<html><head><title>500 Server Error</title></head><body><h1>Server Error</h1></html>", HTTP_500); 
	}

   send_bytes = write(new_sockfd, error_msg, strlen(error_msg));
   if (send_bytes < 0) 
      perror("ERROR writing to socket");
   else
      printf("Sent %d error to a client, %d bytes\n", error_code, send_bytes);
}

//req_url??? ????????? ????????? ????????? ???????????????
void send_content_response(int new_sockfd, char *req_url)
{
   int send_bytes;
   char content_path[BUF_SIZE];
   bzero(content_path, BUF_SIZE);
   char content_buf[MAX_BUF_SIZE];

   struct stat st_buf;
   int file_size = 0;									
   int total_bytes = 0;
  
   if(strcmp(req_url, "/") == 0) {
      strcpy(content_path, cur_path);   //?????? ??????????????? content_path??? ?????????
		strcat(content_path, "/eunjung.html");  //html??? ??????
   }
   else {
      //???????????? ?????? ????????? ????????? ?????????
      strcpy(content_path, cur_path);
		strcat(content_path, req_url);  
   }
   
   FILE *content_fp = fopen(content_path, "rb");    //????????? rb??? ??????
   if(content_fp == NULL) {
      send_error_response(new_sockfd, 404);
      return;
   }
   //?????? ????????? ??????
	if (stat(content_path, &st_buf) == 0)
	   file_size = st_buf.st_size;									

   /*Send HTTP header to a client*/
   send_bytes = send_content_header(new_sockfd, content_type(content_path), file_size); //content header??? ????????????
   if (send_bytes <= 0) return;

   /*Send file contents to a client*/
   int read_bytes = 0;
   while ((read_bytes = fread(content_buf, sizeof(char), MAX_BUF_SIZE, content_fp)) > 0)
   {
      send_bytes = write(new_sockfd, content_buf, read_bytes);
      if (send_bytes < 0) 
         perror("ERROR writing to socket");
      total_bytes += send_bytes;
   }  
   fclose(content_fp);

   printf("Sent %d bytes contents to a client\n", total_bytes);
}

/*Send HTTP header*/
int send_content_header(int new_sockfd, char *content_type, int content_size)
{
   char content_length[BUF_SIZE];		
	snprintf(content_length, sizeof(content_length), "\r\nContent-Length: %d",content_size);		//Content Length ??????

    //?????? ????????? ?????????
	char cur_time[50];
	time_t now = time(NULL);
	struct tm time_data = *gmtime(&now);
	strftime(cur_time,sizeof(cur_time),"%a, %d %b %Y %H:%M:%S %Z", &time_data);	//current time ??????

   char header[MAX_BUF_SIZE];	//Header
   bzero(header, MAX_BUF_SIZE);
   //header ??????
	strcpy(header, HTTP_200);
    strcat(header, "\r\nContent-Type: ");   //content_type ???????????? ????????????
	strcat(header, content_type);	
	strcat(header, content_length);
	strcat(header, "\r\nConnection: keep-alive");
	strcat(header, "\r\nDate: ");
	strcat(header, cur_time);
	strcat(header, "\r\nServer: myserver");
	strcat(header, "\r\n\r\n");
	/*Send header*/
   int send_bytes = write(new_sockfd, header, strlen(header));  //header ?????????
   if (send_bytes < 0) 
      perror("ERROR writing to socket");
   else {
      printf("------------ Responded Header ------------\n"); 
      printf("%s", header);
   }
   return send_bytes;
}

/*Get Content type*/
char* content_type(char *file_path)
{
	char *dot = strrchr(file_path, '.'); // return the address of last '.' found in string
	char *extension; //file extension

	if(!dot || dot == file_path)
		extension = "";
	else
		extension = dot + 1; 

	//content type return
	if(strncmp(extension, "html", 4) == 0 || strncmp(extension, "htm", 3) == 0)
		return "text/html";
	else if(strncmp(extension, "txt", 3) == 0)
		return "text/plain";
	else if(strncmp(extension, "gif", 3) == 0)
		return "image/gif";
	else if(strncmp(extension, "jpeg", 4) == 0 || strncmp(extension, "jpg", 3) == 0)
		return "image/jpeg";
	else if(strncmp(extension, "pdf", 3) == 0)
		return "application/pdf";
	else if(strncmp(extension, "mp3", 3) == 0)
		return "audio/mpeg";
	else
		return "application/octet-stream";
} 
