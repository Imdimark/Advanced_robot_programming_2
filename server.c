#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>	
#include <time.h>

////////////////////////////
struct Feedback { 
    int position;	// position along z axis
    char mess [25];  // feedback message
};
int flag = 0; 
////////////////////////////

void error(char *msg)
{ //function to check errors
    perror(msg);
    exit(1);
}

void handle_sigusr1 (int sig){ // this signal adverts the simulator that it must read from pipe a command
    if (sig == SIGUSR1) 
        printf("received SIGUSR1\n");
    flag = 1;
}


int main(int argc, char *argv[]){  //Super process for connecting socket and forking
    int fd1[2];// file descriptor to connect simulator and server to receive over pipe
    int fd2[2];// file descriptor to connect simulator and server to send over pipe
    int fd3[2];// file descriptor to connect super-process and server to receive over pipe
    pid_t pid0,pid1,pid2; 
    int pipe1 = pipe(fd1); // pipe to connect simulator and server to receive
    int pipe2 = pipe(fd2); // pipe to connect simulator and server to send
    int pipe3 = pipe(fd3); // pipe to connect super-process and server to receive
    int r, w, n, cmd;
    struct Feedback fb;    

    ///// operation to connect servet to client over socket
    int sockfd, newsockfd, portno, clilen;
    struct sockaddr_in serv_addr, cli_addr;
    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        fflush (stdout);
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd < 0) 
        error ("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        error("ERROR on binding");
    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0) 
        error ("ERROR on accept");	
    /////

    pid0= fork ();// Creation of the process server to receive
    if (pid0<0)
        error ("ERROR on creating server"); 
    if (pid0>0)  {
        pid1=fork();// Creation of the simulator (H)
        if (pid1<0)
            error ("ERROR on creating the simulator"); 
        if(pid1!=0) { 
            w = write (fd3[1], &pid1, sizeof (pid1));
            if (w < 0) 
                error ("ERROR on writing to pipe");	
            pid2 = fork();// Creation of the process server to send
            if (pid2<0)
                error ("ERROR on creating the return server"); 
            if (pid2==0) { //Server process to send the feedback
                while(1){
                    struct Feedback fb;
                    n = read(fd2[0], &fb, sizeof (fb)); //// The server reads from the simulator
                    if (n < 0) 
                        error ("ERROR on reading from pipe");
                    n = write(newsockfd,&fb,sizeof(fb)); //////The server sends to client the feedback
                    if (n < 0) 
                        error ("ERROR on writing on socket");
                    }    
            }
        }
        else{
            //Simulator H 
            int cmd = 0;
            int z = 0;
            int x;
            if (signal(SIGUSR1, handle_sigusr1) == SIG_ERR) // signal
            printf("\n can't catch\n");   
            while(1) {  
                if (flag==1){
                    n = read(fd1[0], &cmd, sizeof (cmd)); // The simulator reads the command from sever
                    if (n < 0) 
                    error ("ERROR on reading from pipe");
                    flag = 0;				
                }
                if(cmd == 0) { //// start point of the hook without any commando from client
                    strcpy(fb.mess, "Ready");
                    fb.position = z;
                }
                else {    		
                    switch (cmd) {
                            case (1): { // operations to go up the hook
                                if(z==0){ 
                                    strcpy(fb.mess, "End of stroke");
                                    fb.position = z;
                                    break;
                                }
                                else {
                                    z = z - 5;
                                    strcpy(fb.mess, "Done");
                                    fb.position = z;
                                    break;
                                }
                            }
                            case (2): { // operations to stop the hook
                                strcpy(fb.mess, "Stop");
                                fb.position = z;
                                break;
                            }
                            case (3): {// operation to go down the hook
                                if(z==200){ 
                                    strcpy(fb.mess, "End of stroke");
                                    fb.position = z;
                                    break;
                                }
                                else {
                                    z = z + 5;
                                    strcpy(fb.mess, "Done");
                                    fb.position = z;
                                    break;	
                                }		
                            }		
                    }
                }
            w = write (fd2[1], &fb , sizeof (fb)); //The simulator writes the command to the server to send over pipe
            if (n < 0) 
                error ("ERROR on writing to pipe");
            sleep (1);
            }
        }
    }
    else   { 
        // Process Server to receive
        pid_t pidsignal; //PID of process H
        n = read(fd3[0], &pidsignal, sizeof (pidsignal)); //read the PID of H from the super-process
        if (n < 0) 
            error ("ERROR on reading from pipe");	
        while(1){	
            n = read(newsockfd,&cmd,sizeof(cmd)); //Reading  the command from socket
            if (n < 0) 
            error("ERROR reading from socket");
            printf("Here is the comand: %d\n",cmd);
            fflush (stdout);
            w = write (fd1[1], &cmd, sizeof (cmd)); //////Writing from server to the simulator over a pipe
            if (w<0)
            error("error on writing to pipe");
            kill (pidsignal,SIGUSR1); //sending a signal to process H
        }
    }
return 0;
}

