#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(int argc, char** argv) {
    if(argc == 3) {
        int dSize = atoi(argv[1]);
        int seed = atoi(argv[2]);
        srand(seed);

        // seed_{dSize}_{seed}.csv
        int dSizeLength = strlen(argv[1]);
        int seedLength = strlen(argv[2]);
        char* fileName = (char*)malloc(dSizeLength + seedLength + 12);
        snprintf(fileName, dSizeLength + seedLength + 11, "seed_%s_%s.csv", argv[1], argv[2]);

        int symbolSize = pow(2, dSize);
        unsigned int alphabet[symbolSize];
        for(int i = 0; i < symbolSize; i++) {
            alphabet[i] = i;
        }

        int assignedSymbolCount = 0;
        int currentMaxIndex = symbolSize;
        FILE *seedFile = fopen(fileName, "w");
        while(assignedSymbolCount < symbolSize) {
            int randomIndex = rand() % currentMaxIndex;
            int randomSymbol = alphabet[randomIndex];

            fprintf(seedFile, "%d;%d\n", assignedSymbolCount, randomSymbol);

            ++assignedSymbolCount;
            currentMaxIndex--;

            for(int i = randomIndex; i < currentMaxIndex; i++) {
                alphabet[i] = alphabet[i+1];
            }
        }
        fclose(seedFile);

        free(fileName);
    } else {
        return -1;
    }
}
