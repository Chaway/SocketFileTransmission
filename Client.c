#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <termios.h>

#define MAXLINE 1024
#define TRUE    1
#define REALSPEED_UPDATE_TIME 1000000  //1000000us = 1s
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

FILE *fp;

void processbar(int rece, int total, float realSpeed, int leftTimeMin, int leftTimeSec, char flag) {
    int i = 0;
    int progress = (int) (rece / (float) total * 100);
    printf("\b\b\b\b\b\b\b\b             \r[" ANSI_COLOR_MAGENTA);
    for (i = 0; i < 0.8 * progress; i++) {
        printf("#");
    }
    for (i += 2; i < 82; i++) {
        printf(" ");
    }
    printf(ANSI_COLOR_RESET "][");
    printf(ANSI_COLOR_MAGENTA" %d%% " ANSI_COLOR_RESET, progress);
    printf("][" ANSI_COLOR_CYAN " %d KB/%d KB " ANSI_COLOR_RESET, rece / 1024, total / 1024);
    if (realSpeed < 1000.0) {
        printf("][Speed: " ANSI_COLOR_YELLOW "%d kB/s" ANSI_COLOR_RESET "]", (int) realSpeed);
    } else if (realSpeed < 10240.0) {
        printf("][Speed: " ANSI_COLOR_YELLOW "%3.2f MB/s" ANSI_COLOR_RESET "]", realSpeed / 1024);
    } else {
        printf("][Speed: " ANSI_COLOR_YELLOW "%3.1f MB/s" ANSI_COLOR_RESET "]", realSpeed / 1024);
    }
    if (flag == 0) {
        printf("[Time Left: " ANSI_COLOR_BLUE "%d min %d s" ANSI_COLOR_RESET "]", leftTimeMin, leftTimeSec);
    } else if (flag == 1) {
        printf("[ " ANSI_COLOR_RED "Pasued by Server" ANSI_COLOR_RESET " ]");
    } else if (flag == 2) {
        printf("[ " ANSI_COLOR_RED "Pasued by Client" ANSI_COLOR_RESET " ]");
    }
    if (rece == total) {
        printf("\n");
    }
    fflush(stdout);
}


int main(int argc, char **argv) {
    int sockfd, n, m;
    unsigned char line[MAXLINE];
    struct sockaddr_in6 servaddr, clieaddr;
    time_t t0 = time(NULL);
    printf("time #: %ld\n", t0);
    fputs(ctime(&t0), stdout);

    if (argc != 3)
        perror("usage: ./client <IPaddress> <Filename>");

    if ((sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
        perror("socket error");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin6_family = AF_INET6;
    servaddr.sin6_port = htons(20001);

    //bind(sockfd,(struct sockaddr*)&clieaddr, sizeof(struct sockaddr));    

    if (inet_pton(AF_INET6, argv[1], &servaddr.sin6_addr) <= 0)
        perror("inet_pton error");

    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("Connect Error, Please Try Again");
        exit(0);
    }

    int fileNameLen = strlen(argv[2]);
    //printf("Filename Length: %d\n", fileNameLen); 

    struct stat filestat;
    int fileSize;
    if (stat(argv[2], &filestat) == 0) {
        fileSize = filestat.st_size;
        //TODO
        //process file size too large
        printf("File Size: %d Bytes\n", fileSize);
    } else {
        exit(0);
    }

    send(sockfd, &fileNameLen, sizeof(int), 0);
    send(sockfd, argv[2], strlen(argv[2]), 0);
    send(sockfd, &fileSize, sizeof(int), 0);
    char isSend = 'U';
    // while(!(isSend == 'Y' || isSend == 'N')){
    n = recv(sockfd, &isSend, sizeof(char), 0);
//	printf("%x,%c\n",isSend,isSend);
//	printf("n = %d\n", n);
//	printf("%c\n", isSend);
    //}
    if (isSend == 'N' || isSend == 'U') {
        printf("Sending Request" ANSI_COLOR_RED " Cancelled" ANSI_COLOR_RESET " by Server\n");
        exit(0);
    } else {
        printf("Request" ANSI_COLOR_GREEN " Accepted" ANSI_COLOR_RESET " by Server, Default Speed Is 2048 kB/s\nIf You Want To Customize Speed, Please Input Transmission Speed (KB/s), And Then Press Enter.  ");
        fflush(stdout);
    }

/*
    if(bind(sockfd, (struct sockaddr*)&cliaddr, sizeof(cliaddr)) == -1)
	perror("bind error");
    if(listen(sockfd, 5) == -1)
	perror("listen error"); 
*/
    fp = fopen(argv[2], "r");
    if (fp == NULL) {
        perror("Open file error");
        exit(0);
    }

    int i = 0;
    double speed = 2048.0;
    /*
    for(int i = 0;i < 10;i++){
        sleep(1);
	printf("\r");
        printf("If You Want To Customize Speed, Please Input Transmission Speed (Bytes/s), And Then Press Enter (%d s)", 10 - 1- i);
	fflush(stdout);
    }*/
    //fcntl(0, F_SETFL, O_NONBLOCK);
    char speedstr[10];
    if ((n = read(0, speedstr, 100)) > 1) {
        speed = atof(speedstr);
        if (speed == 0) {
            speed = 2048;
        }
    }

    //fscanf(stdin,"%lf",&speed);
    //scanf("%lf", &speed);
    int time_per_kB = (int) (1000000.0 / speed);
    int now = 0;
    struct timeval t_val, t_val_end, t_result, t_min;
    printf("Transmitting... (" ANSI_COLOR_GREEN "Press Space to Pause/Resume, Press 'q' to Quit)\n" ANSI_COLOR_RESET);
    int dur = 0;
    int sentByte = 0;
    int sentByteMin = 0;
    float realSpeed = 0;
    int LeftTimeMin = 0;
    int LeftTimeSec = 0;
    char ch;
    char serverPauseFlag = 0;
    fcntl(0, F_SETFL, O_NONBLOCK);
    struct termios info;
    tcgetattr(0, &info);          /* get current terminal attirbutes; 0 is the file descriptor for stdin */
    info.c_lflag &= ~ICANON;      /* disable canonical mode */
    info.c_cc[VMIN] = 1;          /* wait until at least one keystroke available */
    info.c_cc[VTIME] = 0;         /* no timeout */
    tcsetattr(0, TCSANOW, &info); /* set immediately */
    int spbyte = 0;
    gettimeofday(&t_val, NULL);
    gettimeofday(&t_min, NULL);
    while ((n = fread(line, sizeof(char), MAXLINE, fp)) > 0) {
        ch = getchar();
        if (ch == ' ') {
            processbar(sentByte, fileSize, 0, 0, 0, 2);
            while (1) {
                fcntl(0, F_SETFL, fcntl(0, F_GETFL) & ~O_NONBLOCK);
                ch = getchar();
                if (ch == ' ') {
                    fcntl(0, F_SETFL, O_NONBLOCK);
                    break;
                }
                if (ch == 'q') {
                    exit(0);
                }
            }
        }
        if (ch == 'q') {
            exit(0);
        }
        //whether server sent pause signal
        fcntl(sockfd, F_SETFL, O_NONBLOCK);
        if ((spbyte = recv(sockfd, &serverPauseFlag, 1, 0)) == 1) {
            if (serverPauseFlag == 1) {
                processbar(sentByte, fileSize, 0, 0, 0, 1);
                fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) & ~O_NONBLOCK);
                spbyte = recv(sockfd, &serverPauseFlag, 1, 0);
            }
            if (serverPauseFlag == 2) {
                printf("\nTransmission Stopped by Server\n");
                exit(0);
            } else if (serverPauseFlag != 0) {
                printf("\nControl Signal Error\n");
                exit(0);
            }
        }
        fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) & ~O_NONBLOCK);
//         printf("\n>>>>>>>%d\n", ++i);
        // for(int c = 0; c < MAXLINE; c++ ){
        //printf("%d\n", c);
        // printf("%x", line[c]);
        // }
        n = send(sockfd, line, n, 0);
        sentByte += n;
        sentByteMin += n;
        processbar(sentByte, fileSize, realSpeed, LeftTimeMin, LeftTimeSec, 0);
        while (1) {
            gettimeofday(&t_val_end, NULL);
            timersub(&t_val_end, &t_min, &t_result);
            dur = t_result.tv_sec * 1000000 + t_result.tv_usec;
            //printf("dur = %d\n", dur);
            //sumTime += dur;
            //printf("dur = %d\n", dur);
            if (dur >= REALSPEED_UPDATE_TIME) {
                gettimeofday(&t_min, NULL);
                //int realtime = t_result.tv_sec*1000000 + t_result.tv_usec;
//		    printf("%d,%d us\n",sentTimes, dur);
                // realSpeed = sentTimes;
                realSpeed = sentByteMin / 1024.0 / (dur / 1e6);
                LeftTimeMin = (int) ((fileSize - sentByte) / 1024.0 / realSpeed / 60.0);
                LeftTimeSec = (int) ((fileSize - sentByte - LeftTimeMin * 60 * realSpeed * 1024.0) /
                                     (realSpeed * 1024.0));
                sentByteMin = 0;
            }
            gettimeofday(&t_val_end, NULL);
            timersub(&t_val_end, &t_val, &t_result);
            dur = t_result.tv_sec * 1000000 + t_result.tv_usec;
            if (dur >= time_per_kB -
                       6) {  // adjust time loss to guarantee data sent per second as close as possilble to theoretical value
                //sentTimes ++;
                gettimeofday(&t_val, NULL);
                break;
            }
        }
    }

    printf("%s Upload Done!\n", argv[2]);
    close(sockfd);
    fclose(fp);
    exit(0);
}


