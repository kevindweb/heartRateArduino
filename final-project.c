#define _BSD_SOURCE 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include <stdint.h>
#include <fcntl.h>
#include "histogram.h"

static volatile int paused = 0;

int init_tty(int fd);
static volatile int keepRunning = 1;
static pid_t childpid;

void intHandler(){
    keepRunning = 0;
    kill(childpid, SIGKILL);
}
/**
 * Run this program and pass the location of the serial file descriptor as an argument
 * On linux, this is likely /dev/ttyACM0
 * On OSX, this may be similar to /dev/cu.usbmodem1D1131
 */

int
main(int argc, char **argv) {

    int fd;
    int counter;
    char *device;
    char buf[32];
    //pipe data
    int  fdpipe[2], nbytes;
    //command received from mmap process
    int cmdbuffer[2];
    //Command sent from menu process
    int cmdToSend[2];

    if (pipe(fdpipe) == -1) {
        perror("pipe");
        return -1;
    }

    fcntl(fdpipe[0], F_SETFL, O_NONBLOCK);

    if(argc == 2 && argv[1][0] == '1'){
        resetMap();
    }

    signal(SIGINT, intHandler);

    device = "/dev/ttyACM0";
    
    printf("Connecting to %s\n", device);

    /*
     * Need the following flags to open:
     * O_RDWR: to read from/write to the devices
     * O_NOCTTY: Do not become the process's controlling terminal
     * O_NDELAY: Open the resource in nonblocking mode
     */
    fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        perror("Error opening serial");
        return -1;
    }

    /* Configure settings on the serial port */
    if (init_tty(fd) == -1) {
        perror("init");
        close(fd);
        return -1;
    }

    /* Wait for Arduino to reboot */
    usleep(1000*1000);

    /* Flush whatever is remaining in the buffer */
    tcflush(fd, TCIFLUSH);

    if((childpid = fork()) == -1) {
        perror("fork");
        exit(1);
    }

    if(childpid == 0){

        close(fdpipe[0]);
        int x;

        while(keepRunning){

            printf("---MENU---\nEnter number to run command\n1. Show X\n2. Pause\n3. Resume\n4. Rate\n5. Env\n6. Hist\n7. Hist X\n8. Date\n9. Stat X\n10. Count\n11. Regression\n12. Reset\n13. Exit\n---------\n");
            scanf("%d", &x);
            fflush(stdin);
            
            //fill command
            cmdToSend[0] = x;
            cmdToSend[1] = 0;

            switch(x){

                case 1:	
                    printf("Show an integer: \n");
                    scanf("%d", &cmdToSend[1]);
                    write(fdpipe[1], cmdToSend, 8);
                    sleep(1);
                    fflush(stdin);
                    break;
                case 2:
                    printf("Pausing\n");
                    write(fdpipe[1], cmdToSend, 8);
                    sleep(1);
                    break;
                case 3:
                    printf("Resuming\n");
                    write(fdpipe[1], cmdToSend, 8);
                    sleep(1);
                    break;
                case 4:
                    printf("Getting Rate\n");
                    write(fdpipe[1], cmdToSend, 8);
                    sleep(1);
                    break;
                case 5:
                    printf("Printing Enviornmental Data\n");
                    write(fdpipe[1], cmdToSend, 8);
                    sleep(1);
                    break;
                case 6:
                    printf("Showing Histogram\n");
                    write(fdpipe[1], cmdToSend, 8);
                    sleep(1);
                    break;
                case 7:
                    printf("Enter a number 0 through 95\n");
                    scanf("%d", &cmdToSend[1]);
                    printf("Showing Histogram at %d\n", cmdToSend[1]);
                    write(fdpipe[1], cmdToSend, 8);
                    fflush(stdin);
                    sleep(1);
                    break;
                case 8:
                    printf("Showing Date\n");
                    write(fdpipe[1], cmdToSend, 8);
                    sleep(1);
                    break;
                case 9:
                    printf("Enter a number 0 through 95\n");
                    scanf("%d", &cmdToSend[1]);
                    printf("Showing Stat at %d\n", cmdToSend[1]);
                    write(fdpipe[1], cmdToSend, 8);
                    fflush(stdin);
                    sleep(1);
                    break;
                case 10:
                    printf("Showing Count\n");
                    write(fdpipe[1], cmdToSend, 8);
                    sleep(1);
                    break;
                case 11:
                    printf("Showing Regression\n");
                    write(fdpipe[1], cmdToSend, 8);
                    sleep(1);
                    break;
                case 12:
                    printf("Resetting data\n");
                    write(fdpipe[1], cmdToSend, 2);
                    resetMap();
                    sleep(1);
                    break;
                case 13:
                    close(fd);
                    write(fdpipe[1], cmdToSend, 2);
                    sleep(1);
                    break;
                default:
                    printf("Did not recognize command\n");
                    fflush(stdin);
                    sleep(1);
                    break;
            }

        }

    }else{

        close(fdpipe[1]);

        int** arr = mapToArr();
		struct regression_data *data;
		data = calloc(1, sizeof(struct regression_data));
		// initialize all data to 0

        while(keepRunning){
            
            sleep(1);
            // Read the response
            counter = read(fd, &buf, 32);
            if (counter == -1) {
                perror("read");
                close(fd);
                return -1;
            }

            // Ensure the response is null-terminated
            buf[counter] = 0;
            int bpm = 0;
            int time = 0;
            int env = 0;
            int cmd = 0;
            if(!paused){
                //get values from message
                bpm = getBPM(buf);
                time = getTimeBucket(buf);
                env = getENV(buf);

                //update array
                arr[time][bpm] += 1;
                if(outlier(arr[time], bpm) != -1){
                    cmd = -1;
                }
            }else{
                cmd = -2;
            }

            nbytes = read(fdpipe[0], cmdbuffer, sizeof(cmdbuffer));
            if(nbytes > 0){
                int l;
                switch(cmdbuffer[0]){
                    case 1:
                        cmd = cmdbuffer[1];
                        printf("Showing %d\n", cmd);
                        break;
                    case 2:
                        paused = 1;
                        break;
                    case 3:
                        paused = 0;
                        break;
                    case 4:
                        printf("Heart Rate: %d\n\n", bpm);
                        break;
                    case 5:
                        printf("Enviornmental Reading: %d\n\n", env);
                        break;
                    case 6:
                        printf("nice\n\n\n");
                        printHist(arr[time], time);
                        break;
                    case 7:
                        if(cmdbuffer[1] >= 0 && cmdbuffer[1] < 96){
                            printHist(arr[cmdbuffer[1]], cmdbuffer[1]);
                        }else{
                            printf("Error printing histogram");
                        }
                        break;
                    case 8:
                        printf("Current Date: ");
                        for(l = 0; l < 5; l++){
                            printf("%c", buf[l+4]);
                        }
                        printf("\n");
                        break;
                    case 9:
                        if(cmdbuffer[1] >= 0 && cmdbuffer[1] < 96){
                            runStats(arr[cmdbuffer[1]]);
                        }else{
                            runStats(arr[time]);
                        }
                        break;
                    case 10:
                        count(data, bpm, env);
                        break;
                    case 11:
                        regression(data);
                        break;
                    case 12:
                        data->count = 0;
                        data->sumX = 0;
                        data->sumY = 0;
                        data->sumMult = 0;
                        data->sumXSquare = 0;
                        data->sumYSquare = 0;
                        resetArr(arr);
                        printf("Done Reseting\n");
                        break;
                    case 13:
                        intHandler();
                        printf("Ending Soon\n");
                        break;

                }
            }

            counter = write(fd, &cmd, 1);
            if (counter == -1) {
                perror("write");
                close(fd);
                return -1;
            } else if (counter == 0) {
                fprintf(stderr, "No data written\n");
                close(fd);
                return -1;
            }


        }
        printf("Exiting Program\n");
        free(data);
        updateMap(arr);
        destroyArr(arr);
        close(fd);

    }
    return 0;
}



int
init_tty(int fd) {
    struct termios tty;
    /*
     * Configure the serial port.
     * First, get a reference to options for the tty
     * Then, set the baud rate to 9600 (same as on Arduino)
     */
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(fd, &tty) == -1) {
        perror("tcgetattr");
        return -1;
    }

    if (cfsetospeed(&tty, (speed_t)B9600) == -1) {
        perror("ctsetospeed");
        return -1;
    }
    if (cfsetispeed(&tty, (speed_t)B9600) == -1) {
        perror("ctsetispeed");
        return -1;
    }

    // 8 bits, no parity, no stop bits
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;

    // No flow control
    tty.c_cflag &= ~CRTSCTS;

    // Set local mode and enable the receiver
    tty.c_cflag |= (CLOCAL | CREAD);

    // Disable software flow control
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);

    // Make raw
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_oflag &= ~OPOST;

    // Infinite timeout and return from read() with >1 byte available
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 0;

    // Update options for the port
    if (tcsetattr(fd, TCSANOW, &tty) == -1) {
        perror("tcsetattr");
        return -1;
    }

    return 0;
}
