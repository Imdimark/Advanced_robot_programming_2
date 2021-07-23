#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

struct Feedback { //struct used to store every datum of a single message
int position;	// position on z axis
char mess [25];  // feedback status of the hook
};

void error(char *msg)
{ //function to check errors
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
//////// operations to connect the client to the server over socket
	int sockfd, portno, n, cmd;
	pid_t pid0; //process child 
	struct sockaddr_in serv_addr;
	struct hostent *server;
	struct Feedback fb;
	if (argc < 3) {
		fprintf(stderr,"usage %s hostname port\n", argv[0]);
		exit(0);
	} 
	portno = atoi(argv[2]);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
		error("ERROR opening socket");
	server = gethostbyname(argv[1]);
	if (server == NULL) {
		fprintf(stderr,"ERROR, no such host\n");
		exit(0);
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,server->h_length);
	serv_addr.sin_port = htons(portno);
	if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
		error("ERROR connecting"); 
/////////
	pid0=fork(); //creatiion of child process
	if (pid0<0)
    	error ("errore in creazione server"); 	
	if(pid0!=0) { // father process
		while (1){
			printf("inserisci il comando: ");       
			scanf("%d", &cmd); //catching of the commando from shell
			n = write(sockfd, &cmd, sizeof (cmd)); //sending of the command to the server over socket
			if (n < 0) 
			error("ERROR writing to socket");
		}
	}
	if(pid0 == 0){ // child process
		while(1){
			n = read(sockfd,&fb,sizeof(fb)); //reiceiving of the feedback from the server over socket
			if (n < 0) 
			error("ERROR reading from socket");
			printf("%d %s\n",fb.position, fb.mess);
			sleep (1);
		}
	}
	return 0;
}

