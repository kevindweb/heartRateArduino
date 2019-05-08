#ifndef HISTORGRAM_H
#define HISTOGRAM_H

struct regression_data {
    int count;
    double sumX;
    double sumY;
    double sumMult;
    double sumXSquare;
    double sumYSquare;
};

int** mapToArr();
int updateMap(int** arr);
int getTimeBucket(char* input);
int getBPM(char* input);
int getENV(char* input);
void destroyArr(int** arr);
int resetArr(int** arr);
int resetMap();
int binarySearch(int a[], int item, int low, int high);
void insertionSort(int a[], int n);
int median(int *arr, int left, int right);
int outlier(int *arr, int data);
void printHist(int *data, int bucket);
void runStats (int *arr);
void count (struct regression_data *data, int bpm, int env);
void regression(struct regression_data *data);

#endif
