#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <math.h>

#define FILEPATH "./mmapped.bin"
#define NUMINTS  (24000)
#define ROW (96)
#define COL (250)
#define regionBPM (14)
#define rangeBPM (210)
#define regionENV (24)
#define rangeENV (360)
#define bucketNUM (15)
#define FILESIZE (NUMINTS * sizeof(int))

struct regression_data {
    int count;
    double sumX;
    double sumY;
    double sumMult;
    double sumXSquare;
    double sumYSquare;
};

int resetMap(){

    int i;
    int fd;
    int result;
    int *map; 


    fd = open(FILEPATH, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
    if (fd == -1) {
    	perror("Error opening file for writing");
    	return -1;
    }

    result = lseek(fd, FILESIZE-1, SEEK_SET);
    if (result == -1) {
    	close(fd);
    	perror("Error calling lseek() to 'stretch' the file");
    	return -1;
    }

    result = write(fd, "", 1);
    if (result != 1) {
    	close(fd);
    	perror("Error writing last byte of the file");
    	return -1;
    }

    map = mmap(0, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
    	close(fd);
    	perror("Error mmapping the file");
    	return -1;
    }

    for(i = 0; i < NUMINTS; i++){
        map[i] = 0;
    }

    if (msync(map, FILESIZE, MS_SYNC) == -1 || munmap(map, FILESIZE) == -1) {
        perror("Error un-mmapping the file");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;

}

int resetArr(int** arr){
    int i, j;
    for(i = 0; i < ROW; i++){
        for(j = 0; j < COL; j++){
            arr[i][j] = 0;
        }
    }
    return 0;
}

int** mapToArr(){
    int i, j;
    int fd;
    int *map; 

    fd = open(FILEPATH, O_RDONLY);
    if (fd == -1) {
    	perror("Error opening file for reading");
    	return NULL;
    }

    map = mmap(0, FILESIZE, PROT_READ, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
    	close(fd);
    	perror("Error mmapping the file");
    	return NULL;
    }
    
    //Read in integers from file into 2D int array for use.
    int** arr;
    if((arr = (int**)malloc(ROW*sizeof(int*))) == NULL){
        perror("Malloc ERR");
        return NULL;
    }

    for (i = 0; i < ROW; i++) {
        if((arr[i] = (int*)malloc(COL*sizeof(int))) == NULL){
            return NULL;
        }
        for (j = 0; j < COL; j++) {
            arr[i][j] = map[(i*COL)+(j)];
        }
    }

    if (munmap(map, FILESIZE) == -1) {
        perror("Error un-mmapping the file");
    }
    close(fd);

    return arr;

}

int updateMap(int** arr){

    int i, j;
    int fd;
    int result;
    int *map; 

    if(arr == NULL){
        return -1;
    }

    fd = open(FILEPATH, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
    if (fd == -1) {
    	perror("Error opening file for writing");
    	return -1;
    }

    result = lseek(fd, FILESIZE-1, SEEK_SET);
    if (result == -1) {
    	close(fd);
    	perror("Error calling lseek() to 'stretch' the file");
    	return -1;
    }

    result = write(fd, "", 1);
    if (result != 1) {
    	close(fd);
    	perror("Error writing last byte of the file");
    	return -1;
    }

    map = mmap(0, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
    	close(fd);
    	perror("Error mmapping the file");
    	return -1;
    }

    for(i = 0; i < ROW; i++){
        for(j = 0; j < COL; j++){
            map[(i*COL)+j] = arr[i][j];
        }
    }

    if (msync(map, FILESIZE, MS_SYNC) == -1 || munmap(map, FILESIZE) == -1) {
        perror("Error un-mmapping the file");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;

}

int getTimeBucket(char* input){

    char hour[3];
    char minute[3];
    int hr, min, num;

    hour[0] = input[4];
    hour[1] = input[5];
    hour[2] = '\0';

    minute[0] = input[7];
    minute[1] = input[8];
    minute[2] = '\0';

    hr = atoi(hour);
    min = atoi(minute);
    num = (hr*4)+(min/15);

    if(num > 95)
        return 95;
    else if(num < 0)
        return 0;

    return num;

}

int getBPM(char* input){
    int n;
    char num[4];
    num[0] = input[0];
    num[1] = input[1];
    num[2] = input[2];
    num[3] = '\0';
    
    n = atoi(num);

    if(n > 249)
        return 249;
    else if(n < 0)
        return 0;

    return n; 

}

int getENV(char* input){
    int n;
    char num[4];
    num[0] = input[10];
    num[1] = input[11];
    num[2] = input[12];
    num[3] = '\0';
    
    n = atoi(num);

    return n; 

}

void destroyArr(int** arr){

    for(int i = 0; i < ROW; i++){
        free(arr[i]);
    }
    free(arr);

}

int median(int *arr, int left, int right){
    if(!arr)
        return -1;

    int i;
    int sum = 0;

    for(i = left; i < right; i++){
        sum += arr[i]; //sums the values of all of the heart rates
    }

    sum /= 2; //divides summation in half

    int counter = 0;
    for(i = left; i < right; i++){
        counter += arr[i];
        if(counter > sum) //The first heartrate which is above the half way point created above is the median. 
            break;
    }

    return i;
}

int outlier(int *arr, int data){
    if(!arr)
        return -1;

    int n = 250;

    // Index of median of entire data
    int mid_index = median(arr, 0, n);

    // Median of first half
    int Q1 = median(arr, 0, mid_index);

    // Median of second half
    int Q3 = median(arr, mid_index + 1, n);
    
    if(data && (data > Q3 || data < Q1)){
        return data;
    }
    return -1;
}


void printHist(int *data, int bucket){ //every ten seconds graphically represents each bucket in the current time increment
    if(!data)
        return;

    int i, j;

    printf("Bucket: %d\n\n", bucket);

    for(i = 0; i < COL; i++){
        printf("[%d]: ", i);
        int numX = data[i];
        for(j = 0; j < numX; j++){
            printf("x ");
        }
        printf("\n");
    }

    printf("\n");
}

void runStats (int *arr) {
	// help from https://knowpapa.com/sd-freq/
    int i;
    int mode;
    int frequency;
    int total;
    int counter;
    int foundMedian;
    double freqSum;
    double freqCheck;
    double sumDev;
    double avg;
    double sd;

    mode = 0;
    total = 0;
    freqSum = 0;

    for(i = 0; i < COL; i++){
        frequency = arr[i];
        if (frequency > 0) {
            if (frequency > arr[mode])
                // new highest frequency location
                mode = i;
        }
        freqSum += frequency;
        total += i * frequency;
    }

    counter = 0;
    foundMedian = -1;
    sumDev = 0;
    sd = 0;
	avg = 0;

    if (freqSum) {
        // don't run if there is no data 
        avg = total / (double)freqSum;
        freqCheck = freqSum;

        freqSum /= 2;
        // get middle of frequencies

        for(i = 0; i < COL; i++){
            // loop through to finish median and get stdDev
            frequency = arr[i];
            if (foundMedian < 0) {
                counter += frequency;
                if(counter > freqSum)
                    // found median bpm, stop searching
                    foundMedian = i;
            }

            sumDev += frequency * (i - avg) * (i - avg);
            // variance is frequency_i*(x_i - average)^2
        }
        sd = sqrt(sumDev / freqCheck);
    } else
		foundMedian = 0;

	printf("Data points collected: %d\n", (int)freqCheck);
    printf("Average: %.2f bpm\n", avg);
    printf("Median: %d bpm\n", foundMedian);
    printf("Mode is %d bpm with frequency %d\n", mode, arr[mode]);
    printf("Standard Deviation: %.2f\n", sd);
}

void count (struct regression_data *data, int bpm, int env) {
    if (!data){
        // make sure we have the data
        printf("Regression data NULL, no data added\n");
        return;
    }

    int bpmBucket;
    if (bpm >= rangeBPM)
        bpmBucket = bucketNUM - 1;
    else if (bpm < 0)
        bpmBucket = 0;
    else
        bpmBucket= bpm / regionBPM;
    // make sure the buckets work
    int start = bpmBucket * regionBPM;
    int end = start + regionBPM - 1;
    printf("Placing bpm data within %d-%d\n", start, end);

    int envBucket;
    if (env >= rangeENV)
        envBucket = bucketNUM - 1;
    else if (env < 0)
        envBucket = 0;
    else
        envBucket = env / regionENV;
    // data shouldn't be out of range
    start = envBucket * regionENV;
    end = start + regionENV - 1;
    printf("Placing env data within %d-%d\n", start, end);

    data->count++;
    // increment data in list
    double envData = (envBucket * regionENV) + (regionENV / 2);
    double bpmData = (bpmBucket * regionBPM) + (regionBPM / 2);
    data->sumX += envData;
    data->sumY += bpmData;
    // get the value of the middle of their respective buckets
    data->sumMult += envData * bpmData;
    data->sumXSquare += envData * envData;
    data->sumYSquare += bpmData * bpmData;
    // add the data from multiplication for easier
    // regression calculation
}

void regression(struct regression_data *data) {
    if(!data){
        printf("Data is NULL, can't get regression\n");
        return;
    } else if(!data->count){
        printf("No data, can't get regression\n");
        return;
    }

    double xSquare = data->sumX * data->sumX;
    double multSums = data->sumX * data->sumY;
    double numerator = (data->sumMult * data->count) - (multSums);
    double denominator = (data->sumXSquare * data->count) - xSquare;
    if(!(int)denominator){
        printf("Not enough variability in data, denominator 0\n");
        // can't divide by 0
        return;
    }

    double b1 = numerator / denominator;
    double b0 = (data->sumY - b1*data->sumX) / data->count;
    // equation of line from this site
    // http://mathworld.wolfram.com/CorrelationCoefficient.html

    double ySquare = data->sumY * data->sumY;
    denominator = (data->sumYSquare * data->count) - ySquare;
    if(!(int)denominator){
        printf("Not enough variability in data, denominator 0\n");
        // can't divide by 0
        return;
    }

    double bPrime = numerator / denominator;

    char posOrNeg;
    if(b0 < 0){
        // print out the negative number correctly
        posOrNeg = '-';
        b0 *= -1;
    } else
        posOrNeg = '+';

    double r2 = b1 * bPrime;
    // http://mathworld.wolfram.com/CorrelationCoefficient.html
    // coefficient of correlation R from this site
    printf("Line of best fit: y = %.2fx %c %.2f\n", b1, posOrNeg, b0);
    printf("Correlation coefficient r^2: %.2f\n", r2);
}
