#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if(argc < 3) {
        return -1;
    }

    const char* file1Name = argv[1];
    const char* file2Name = argv[2];
    int writeAmount = argc >= 4 ? atoi(argv[3]) : 15;

    FILE *f1ptr = fopen(file1Name, "rb");
    fseek(f1ptr, 0, SEEK_END);
    unsigned long long int file1Length = ftell(f1ptr);
    fseek(f1ptr, 0, SEEK_SET);
    unsigned char* file1Data = (unsigned char*)malloc(file1Length);
    fread(file1Data, 1, file1Length, f1ptr);
    fclose(f1ptr);

    FILE *f2ptr = fopen(file2Name, "rb");
    fseek(f2ptr, 0, SEEK_END);
    unsigned long long int file2Length = ftell(f2ptr);
    fseek(f2ptr, 0, SEEK_SET);
    unsigned char* file2Data = (unsigned char*)malloc(file2Length);
    fread(file2Data, 1, file2Length, f2ptr);
    fclose(f2ptr);

    if(file1Length != file2Length) {
        printf("\r\nFile lengths of the two file are not equal! Comparing files until the smallest data runs out.\r\n");
    }

    unsigned long long int differenceCount = 0;
    unsigned long long int length = file1Length > file2Length ? file2Length : file1Length;

    unsigned char prev1 = 0, prev2 = 0, prev3 = 0;
    for(unsigned long long int i = 0; i < length; ++i) {
        unsigned char file1Byte = file1Data[i];
        unsigned char file2Byte = file2Data[i];

        if(file1Byte != file2Byte) {
            if(differenceCount == 0) {
                printf("Prev1: %d - Prev2: %d - Prev3: %d\r\n", prev1, prev2, prev3);
            }

            ++differenceCount;
            // printf("Files are not identical. Difference found in index %lld\r\n", i);

            if(differenceCount <= writeAmount) {
                printf("Two bytes are not identical. Byte1: %d - Byte2: %d - Index: %lld\r\n", file1Byte, file2Byte, i);
            }
        } else {
            prev3 = prev2;
            prev2 = prev1;
            prev1 = file1Byte;
        }
    }

    printf("\r\nTotal Difference Count: %lld\r\n\r\n", differenceCount);
}
