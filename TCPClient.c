#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#define MAXLINE 1024
#define TRUE    1


FILE *fp;

int display_progress(int progress, int last_char_count)     
{
    int i = 0;

    /*把上次显示的进度条信息全部清空*/
    for (i = 0; i < last_char_count; i++)
    {
        printf("\b"); 
    }

    /*此处输出‘=’，也可以是其他字符，仅个人喜好*/
    for (i = 0; i < progress; i++)
    {
            printf("=");  
    }
    printf(">>");
    /*输出空格截止到第104的位置，仅个人审美*/
    for (i += 2; i < 104; i++) 
    {
            printf(" ");
    }
    /*输出进度条百分比*/
    i = i + printf("[%d%%]", progress);  
    /*此处不能少，需要刷新输出缓冲区才能显示，
    这是系统的输出策略*/
    fflush(stdout);

    /*返回本次显示进度条时所输出的字符个数*/ 
    return i; 
}

int main(int argc, char **argv)
{
    int sockfd, n, m;
    unsigned char line[MAXLINE];
    struct sockaddr_in6 servaddr, clieaddr;
    time_t t0 = time(NULL);
    printf("time #: %ld\n", t0);
    fputs(ctime(&t0), stdout);
    
    if(argc != 3)
        perror("usage: ./Client <IPaddress> <Filename>");
    
    if((sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
        perror("socket error");
       
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin6_family = AF_INET6;
    servaddr.sin6_port = htons(20001);
    
    //bind(sockfd,(struct sockaddr*)&clieaddr, sizeof(struct sockaddr));    
    
    if(inet_pton (AF_INET6, argv[1], &servaddr.sin6_addr) <= 0) 
        perror("inet_pton error");
       
    if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
        perror("connect error");
    
    int fileNameLen = strlen(argv[2]);
    printf("Filename Length: %d\n", fileNameLen); 

    struct stat filestat; 
    int fileSize;
    if(stat(argv[2], &filestat) == 0){
       fileSize= filestat.st_size;
       printf("File Size: %d Bytes\n", fileSize); 
    }
    else{
        exit(0);
    }

    send(sockfd, &fileNameLen, sizeof(int), 0);
    send(sockfd, argv[2], strlen(argv[2]), 0);
    send(sockfd, &fileSize, sizeof(int), 0);
    char isSend = 'U';
    while(!(isSend == 'Y' || isSend == 'N')){
        n = recv(sockfd, &isSend, sizeof(char), 0);
//	printf("n = %d\n", n);
//	printf("%c\n", isSend);
    }
    if(isSend == 'N'){
	printf("Sending Request Canceled by Server\n");
        exit(0);
    }
    else
    {
	printf("Transferring ... \n");
    }

/*
    if(bind(sockfd, (struct sockaddr*)&cliaddr, sizeof(cliaddr)) == -1)
	perror("bind error");
    if(listen(sockfd, 5) == -1)
	perror("listen error"); 
*/
    fp = fopen(argv[2], "r");
    if( fp == NULL){
        perror("Open file error");
        exit(0);
    }

    printf("%s Upload Done!\n", argv[2]);
    int i = 0;
    while((n = fread(line, sizeof(char), MAXLINE, fp)) > 0 ) {
//         printf("\n>>>>>>>%d\n", ++i);
        // for(int c = 0; c < MAXLINE; c++ ){
            //printf("%d\n", c);
            // printf("%x", line[c]);
        // }
        send(sockfd, line, n, 0);
        //if(i > 1)
            //break;
    }
    
    close(sockfd);
    fclose(fp);
    exit(0);
}
