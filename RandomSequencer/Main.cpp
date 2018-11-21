#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>

const int INT_COUNT_FOR_MB = 262144;
const int MAX_TARGET_MB = 2048;
const int SEED = 15;

int main(int argc, char **argv) {
    srand(SEED);

    int size = 2048;
    if(argc == 2) {
        sscanf(argv[1], "%d", &size);
    } else if(argc == 3) {
        sscanf(argv[1], "%d", &size);
        int seed = SEED;
        sscanf(argv[2], "%d", &seed);

        srand(seed);
    }

    if(size > MAX_TARGET_MB)
        size = MAX_TARGET_MB;

    int totalSize = INT_COUNT_FOR_MB * size;
    char fileName[50];
    sprintf(fileName, "randomSequence_%d.bin", size); ;

    FILE* pFile;
    pFile = fopen(fileName, "wb");

    int randomNumberCount = 0;
    while(++randomNumberCount <= totalSize) {
        int rnd = rand();
        fwrite(&rnd, 4, 1, pFile);
    }
    fclose(pFile);

    return 0;
}
