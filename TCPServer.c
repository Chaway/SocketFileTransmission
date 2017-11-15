#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define MAXLINE 1024
#define MAXNAMELENGTH 100
#define TRUE    1

FILE *fp;

int main(int argc, char **argv)
{
    int sockfd, fd, n, m;
    unsigned char line[MAXLINE];
    struct sockaddr_in6 servaddr, cliaddr;
    time_t t0 = time(NULL);
    printf("time #: %ld\n", t0);
    fputs(ctime(&t0), stdout);
    
    if((sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
        perror("socket error");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin6_family = AF_INET6;
    servaddr.sin6_port = htons(20001);
    servaddr.sin6_addr = in6addr_any;

    if(bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1)  
        perror("bind error");  
  
    if(listen(sockfd, 5) == -1)  
        perror("listen error");  

    while(TRUE) {

        printf("> Waiting clients ...\r\n");

        socklen_t clilen = sizeof(struct sockaddr);  
        fd = accept(sockfd, (struct sockaddr*)&cliaddr, &clilen);  
	struct in6_addr ipAddr = cliaddr.sin6_addr;
	int clientPort = cliaddr.sin6_port;
	char strClientIP[INET6_ADDRSTRLEN];
	inet_ntop(AF_INET6, &ipAddr, strClientIP, INET6_ADDRSTRLEN);
        if(fd == -1)  {  
            perror("accept error");  
        }   

        printf("> Accepted.\r\n");
	printf("Client is From IP: %s Port: %d \n", strClientIP, clientPort);
        char filename[MAXNAMELENGTH];
        int fileNameLen = 0;
	int fileSize = 0;
	n = recv(fd, &fileNameLen, sizeof(int), 0);
	//printf("n = %d\n", n);
	if(n == sizeof(int)){
		//printf("Filename Length = %d\n", fileNameLen);
		if(fileNameLen > MAXNAMELENGTH){
		    perror("Filename too long");
		    exit(0); 
		}
		if((n = recv(fd, filename, fileNameLen, 0)) <= 0){
		    perror("Filename Error");
		    exit(0);
		}	
		n = recv(fd, &fileSize, sizeof(int), 0);
		if(fileSize <= 0){
		    perror("File Size Must Larger Than 0");
		    exit(0);
		}
	}
	else{
	    perror("Filename Length Transmission Error");
	    exit(0);
	}
	char ch;
	printf("Accept File '%s' from ipv6_addr ([N]/Y) ", filename);
        char isAccept = 'N';
	char notAccept = 'N';
//        fflush(stdin);
        int chi = 0;
	while((ch=fgetc(stdin)) != '\n' && ch != EOF){
		if(chi == 0)
			isAccept = ch;
        	chi ++;
	}
         
	if(chi >= 2){
	    printf("Wrong Input,And Refuse to Accept File\n");
	    send(fd, &notAccept, sizeof(char), 0);
	    continue;
        }
	else if(isAccept != 'Y'){
	    printf("Refuse to Accept File\n");
	    send(fd, &notAccept, sizeof(char), 0);
	    continue;
	}

	send(fd, &isAccept, sizeof(char), 0);

	printf("Start to Transmit File: ");
	// printf("%s\n", filename);
	if(fputs(filename, stdout) == EOF){
	    perror("fputs error");
	}
	else{
	    fp = fopen(filename, "wb");
	    if(fp == NULL){
		 perror("Open file error");
		 exit(0);
	    }
	}
	
        printf(" (%d Bytes)\n", fileSize);
        int i = 0;
        while((n = recv(fd, line, MAXLINE, 0)) > 0) {
            //if(fputs(line, fp) == EOF)
            fwrite(line, sizeof(char), n, fp);
            //printf("%d\n", n);
            // printf("\n>>>>>>>%d\n", ++i);
            // for(int c = 0; c < MAXLINE; c++ ){
                // printf("%x", line[c]);
            // }
            //printf("%s\n", line);
        }
        printf("Done!\n");
        close(fd);
        fclose(fp);
    }      
    exit(0);
}
