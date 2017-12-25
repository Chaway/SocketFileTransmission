#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <errno.h>
#include <termios.h>

#define MAXLINE 1024
#define MAXNAMELENGTH 100
#define REALSPEED_UPDATE_TIME 1000000
#define TRUE    1
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
    printf("\b\b\b\b\b\b        \r[" ANSI_COLOR_MAGENTA);
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
    fflush(stdout);
}


int main(int argc, char **argv) {
    int sockfd, fd, n, m;
    unsigned char line[MAXLINE];
    struct sockaddr_in6 servaddr, cliaddr;
    time_t t0 = time(NULL);
    printf("time #: %ld\n", t0);
    fputs(ctime(&t0), stdout);

    if ((sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
        perror("socket error");
    printf("Created a Socket, File Discriptor sockfd = %d \n", sockfd);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin6_family = AF_INET6;
    servaddr.sin6_port = htons(20001);
    servaddr.sin6_addr = in6addr_any;

    if (bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1) {
        perror("bind error");
        exit(0);
    }
    if (listen(sockfd, 5) == -1)
        perror("listen error");

    while (TRUE) {

        printf(ANSI_COLOR_GREEN "> Waiting clients ...\n" ANSI_COLOR_RESET);

        socklen_t clilen = sizeof(struct sockaddr);
        fd = accept(sockfd, (struct sockaddr *) &cliaddr, &clilen);
        struct in6_addr ipAddr = cliaddr.sin6_addr;
        int clientPort = cliaddr.sin6_port;
        char strClientIP[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &ipAddr, strClientIP, INET6_ADDRSTRLEN);
        if (fd == -1) {
            perror("accept error");
        }

        printf(ANSI_COLOR_GREEN "> Accepted.\n" ANSI_COLOR_RESET);
        printf("Created a New Socket for Client, And File Discriptor is fd = %d \n", fd);
        printf(ANSI_COLOR_CYAN "Client is From IP: %s Port: %d \n" ANSI_COLOR_RESET, strClientIP, clientPort);
        char filename[MAXNAMELENGTH];
        int fileNameLen = 0;
        int fileSize = 0;
        n = recv(fd, &fileNameLen, sizeof(int), 0);
        //printf("n = %d\n", n);
        if (n == sizeof(int)) {
            //	printf("Filename Length = %d\n", fileNameLen);
            if (fileNameLen > MAXNAMELENGTH) {
                perror("Filename too long");
                exit(0);
            }
            if ((n = recv(fd, filename, fileNameLen, 0)) != fileNameLen) {
                perror("Filename Error");
                exit(0);
            }
            filename[fileNameLen] = 0;
            n = recv(fd, &fileSize, sizeof(int), 0);
            if (fileSize <= 0) {
                perror("File Size Must Larger Than 0");
                exit(0);
            }
        } else {
            perror("Filename Length Transmission Error");
            exit(0);
        }
        char ch;
        printf("Accept File" ANSI_COLOR_CYAN " '%s' " ANSI_COLOR_RESET "(%d Bytes)? ([N]/Y) ", filename, fileSize);
//        printf(" (%d Bytes)\n", fileSize);
        char isAccept = 'N';
        char notAccept = 'N';
//        fflush(stdin);
        int chi = 0;
        while ((ch = fgetc(stdin)) != '\n' && ch != EOF) {
            if (chi == 0)
                isAccept = ch;
            chi++;
        }

        if (chi >= 2) {
            printf("Wrong Input, And Refused to Accept File\n");
            send(fd, &notAccept, sizeof(char), 0);
            continue;
        } else if (isAccept != 'Y') {
            printf("Refused to Accept File\n");
            send(fd, &notAccept, sizeof(char), 0);
            continue;
        }

        send(fd, &isAccept, sizeof(char), 0);

        printf("Prepared to Transmit File: '%s'\n", filename);
        // printf("%s\n", filename);
        fp = fopen(filename, "wb");
        if (fp == NULL) {
            perror("Open File Error");
            exit(0);
        }

        int receviedByte = 0;
        int receviedByteMin = 0;
        struct timeval t_val, t_val_end, t_result, t_min;
        int dur = 0;
        float realSpeed = 0;
        int LeftTimeMin = 0;
        int LeftTimeSec = 0;
        int byteinBuffer = 1024;
        struct timeval tvTimeout;
        tvTimeout.tv_sec = 0;
        tvTimeout.tv_usec = 500000;
        fd_set set;
        FD_ZERO(&set);
        FD_SET(fd, &set);
        int selRe = 1;
        struct termios info;
        tcgetattr(0, &info);          /* get current terminal attirbutes; 0 is the file descriptor for stdin */
        info.c_lflag &= ~ICANON;      /* disable canonical mode */
        info.c_cc[VMIN] = 1;          /* wait until at least one keystroke available */
        info.c_cc[VTIME] = 0;         /* no timeout */
        tcsetattr(0, TCSANOW, &info); /* set immediately */
        fcntl(0, F_SETFL, O_NONBLOCK);
        char pauseFlag = 0;
        gettimeofday(&t_val, NULL);
        gettimeofday(&t_min, NULL);
        //fcntl(fd, F_SETFL, O_NONBLOCK);
        while ((n = recv(fd, line, MAXLINE, 0)) > 0) {
            //printf("%d", n);
            fwrite(line, sizeof(char), n, fp);
            if (receviedByte == 0) {
                printf("Transmiting... (" ANSI_COLOR_GREEN "Press Space to Pause/Resume, Press 'q' to Quit)\n" ANSI_COLOR_RESET);
            }
            receviedByte += n;
            receviedByteMin += n;
            ch = getchar();
            if (ch == ' ') {
                processbar(receviedByte, fileSize, 0, 0, 0, 1);
                pauseFlag = 1;
                send(fd, &pauseFlag, 1, 0);
                while (1) {
                    fcntl(0, F_SETFL, fcntl(0, F_GETFL) & ~O_NONBLOCK);
                    ch = getchar();
                    if (ch == ' ') {
                        fcntl(0, F_SETFL, O_NONBLOCK);
                        pauseFlag = 0;
                        send(fd, &pauseFlag, 1, 0);
                        break;
                    }
                    if (ch == 'q') {
                        break;
                    }
                }
            }
            if (ch == 'q') {
                pauseFlag = 2;
                send(fd, &pauseFlag, 1, 0);
                break;
            }
            //if(fputs(line, fp) == EOF)
            //if(receviedByte == 0){
            //fcntl(fd, F_SETFL, O_NONBLOCK); //set to not blocking
// 
            FD_ZERO(&set);
            FD_SET(fd, &set);
            tvTimeout.tv_sec = 0;
            tvTimeout.tv_usec = 500000;
            if ((selRe = select(fd + 1, &set, NULL, NULL, &tvTimeout)) == 0) {
                //printf("%d\n", errno);
                //printf("%d, %d\n",n ,receviedByteMin);
                //fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_NONBLOCK); //set to blocking
                processbar(receviedByte, fileSize, 0, 0, 0, 2);
                //printf("%d,pause\n", receviedByte);
                //fflush(stdout);
                if ((n = recv(fd, line, MAXLINE, 0)) == 0) { //waiting clint to resume
                    break;
                }
                fwrite(line, sizeof(char), n, fp);
                receviedByte += n;
                receviedByteMin += n;
                byteinBuffer = 1024;
                //fcntl(fd, F_SETFL, O_NONBLOCK); //set to not blocking
            }
            //printf("%d\n",receviedByte);
            gettimeofday(&t_val_end, NULL);
            timersub(&t_val_end, &t_min, &t_result);
            dur = t_result.tv_sec * 1000000 + t_result.tv_usec;
            if (dur >= REALSPEED_UPDATE_TIME) {
                //printf("%d,%d",receivedTimes,dur);
                gettimeofday(&t_min, NULL);
                realSpeed = receviedByteMin / 1024.0 / (dur / 1000000.0);
                LeftTimeMin = (int) ((fileSize - receviedByte) / 1024.0 / realSpeed / 60.0);
                LeftTimeSec = (int) ((fileSize - receviedByte - LeftTimeMin * 60 * realSpeed * 1024.0) /
                                     (realSpeed * 1024.0));
                receviedByteMin = 0;
                //  printf("%lf\n", realSpeed);
            }
            if (n > 0) {
                processbar(receviedByte, fileSize, realSpeed, LeftTimeMin, LeftTimeSec, 0);
            }
            //ioctl(fd, FIONREAD, &byteinBuffer);
        }
        info.c_lflag |= ICANON;
        tcsetattr(0, TCSANOW, &info);
        fcntl(0, F_SETFL, fcntl(0, F_GETFL) & ~O_NONBLOCK);

        //printf("%d\n", n);
        // printf("\n>>>>>>>%d\n", ++i);
        // for(int c = 0; c < MAXLINE; c++ ){
        // printf("%x", line[c]);
        // }
        //printf("%s\n", line);

        if (receviedByte == fileSize) {
            printf("\nTransmission " ANSI_COLOR_GREEN "Successful!" ANSI_COLOR_RESET " (Totally Received %d Bytes)\n",
                   receviedByte);
        } else {
            if (receviedByte > 0) {
                printf("\n");
            }
            printf("Transmission " ANSI_COLOR_RED "Failed!" ANSI_COLOR_RESET " (Totally Received %d Bytes)\n",
                   receviedByte);
        }
        close(fd);
        fclose(fp);
    }
    exit(0);
}
