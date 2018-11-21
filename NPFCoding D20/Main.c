#include "CodingParameters.h"
#include "HuffmanTableDataStructures.h"
#include "FunctionDeclerations.h"
#include "Utilities.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// THIS IMPLEMENTATION IS FOR D = 20

unsigned char seeded;
unsigned char encode;
unsigned int permutatedAlphabet[SYMBOL_SIZE];        // 4 MiB
unsigned int reversePermutatedAlphabet[SYMBOL_SIZE]; // 4 MiB
char* originalFileName;
char* payloadFileName;
char* disInfoFileName;
char* seedFileName;

disInfoTable DItables[256]; // Size: 36.75 kb
reverseDITableMap rdiTM;    // Size: 50 KiB

int main(int argc, char **argv) {
    HandleCommandArguments(argc, argv, &encode, &seeded);
	if (encode == -1) {
        return -1;
	}

	InitializeTableMaps();

	if (encode) {
		Encode();

		free(payloadFileName);
		free(disInfoFileName);
	}
	else {
		Decode();

		free(originalFileName);
	}
	if(encode && seeded) {
        free(seedFileName);
	}
}

void HandleCommandArguments(int argc, char** argv, unsigned char* encode, unsigned char* seeded) {
	// if argc is equal to 2, we are encoding the specified file
	// if argc is equal to 3, either we are encoding the specified file with a seed or we are decoding the payload with the disambiguation information
	// if argc is equal to 4, we are decoding the payload with the disambiguation information using the given seed

	*encode = -1;
	*seeded = -1;
	if (argc == 2) {
        // non-seeded encode
		*encode = 1;
		*seeded = 0;
		originalFileName = argv[1];

		// We want to add "_payload.bin" and "_disInfo.bin" to end of the encoded file names.
		// The length of these strings are 12 charachter long.
		char* fileNameWithoutExtension = remove_ext(originalFileName, '.', '/');
		int payloadFileNameLength = strlen(fileNameWithoutExtension) + 12 + 1;

		payloadFileName = (char*)malloc(payloadFileNameLength + 1);
		snprintf(payloadFileName, payloadFileNameLength, "%s%s", fileNameWithoutExtension, "_payload.bin");
		disInfoFileName = (char*)malloc(payloadFileNameLength + 1);
		snprintf(disInfoFileName, payloadFileNameLength, "%s%s", fileNameWithoutExtension, "_disInfo.bin");

		free(fileNameWithoutExtension);
	}
	else if (argc == 3) {
		if(argv[2][0] == '-' && argv[2][1] == 's' && argv[2][2] == ':') {
            // seeded encode
            *encode = 1;
            *seeded = 1;
            originalFileName = argv[1];

            int seedFileLength = strlen(argv[2]);
            seedFileName = (char*)malloc(seedFileLength - 2);
            memcpy(seedFileName, &argv[2][3], seedFileLength - 3);
            seedFileName[seedFileLength - 3] = '\0';

            // We want to add "_payload.bin" and "_disInfo.bin" to end of the encoded file names.
            // The length of these strings are 12 charachter long.
            char* fileNameWithoutExtension = remove_ext(originalFileName, '.', '/');
            int payloadFileNameLength = strlen(fileNameWithoutExtension) + 12 + 1;

            payloadFileName = (char*)malloc(payloadFileNameLength + 1);
            snprintf(payloadFileName, payloadFileNameLength, "%s%s", fileNameWithoutExtension, "_payload.bin");
            disInfoFileName = (char*)malloc(payloadFileNameLength + 1);
            snprintf(disInfoFileName, payloadFileNameLength, "%s%s", fileNameWithoutExtension, "_disInfo.bin");

            free(fileNameWithoutExtension);
		} else {
            // non-seeded decode
            *encode = 0;
            *seeded = 0;
            payloadFileName = argv[1];
            disInfoFileName = argv[2];

            // We want to add "_decoded.bin" to the end of the decoded file name.
            char* fileNameWithoutExtension = remove_ext(payloadFileName, '.', '/');
            int decodedFileNameLength = strlen(fileNameWithoutExtension) + 12 + 1;

            originalFileName = (char*)malloc(decodedFileNameLength + 1);
            snprintf(originalFileName, decodedFileNameLength, "%s%s", fileNameWithoutExtension, "_decoded.bin");

            free(fileNameWithoutExtension);
		}
	}
	else if(argc == 4) {
        // seeded decode
        *encode = 0;
        *seeded = 1;
        payloadFileName = argv[1];
        disInfoFileName = argv[2];
        seedFileName = argv[3];

        // We want to add "_decoded.bin" to the end of the decoded file name.
        char* fileNameWithoutExtension = remove_ext(payloadFileName, '.', '/');
        int decodedFileNameLength = strlen(fileNameWithoutExtension) + 12 + 1;

        originalFileName = (char*)malloc(decodedFileNameLength + 1);
        snprintf(originalFileName, decodedFileNameLength, "%s%s", fileNameWithoutExtension, "_decoded.bin");

        free(fileNameWithoutExtension);
	}
}

void InitializeTableMaps() {
	PermutateAlphabet();
	if(encode) {
        CreateDisInfoTableMap();
	} else {
        CreateReverseDisInfoTableMap();
	}
}

void Encode() {
    FILE *fptr = fopen(originalFileName, "rb");
    clock_t start, stop;

    start = clock();
    fseek(fptr, 0, SEEK_END);
    unsigned long long int fileLength = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);
    unsigned char* data = (unsigned char*)malloc(fileLength);
    fread(data, 1, fileLength, fptr);
    fclose(fptr);

    stop = clock();

    double elapsed_secs = (double)(stop - start) / CLOCKS_PER_SEC;
    printf("\r\nReading file took %f seconds.", elapsed_secs);

	// We need to figure out how much space we need to allocate before hand.
	// approximately, disInfo takes 2/d size of the original data
	// payload takes d-2/d size of the original data
	unsigned char* encodedData = (unsigned char*)malloc(fileLength);
	unsigned char* disInfoData = (unsigned char*)malloc(fileLength);

    memset(encodedData, 0, fileLength);
    memset(disInfoData, 0, fileLength);

	clock_t startEncode, stopEncode;
	startEncode = clock();

	unsigned long long int currentPos = 0;
	unsigned long long int currentIndex = 0;
	unsigned long long int disInfoIndex = 4;
	unsigned char disInfoTableIndex = 1;
    unsigned int value = 0;

    const huffmanRow* disInfoRow;

    unsigned char initialValue = 0;
    unsigned char initialLength = 0;
    unsigned char halfByte = 0;

	while (currentPos < (fileLength-2)) {
        if(halfByte) {
            value = ((data[currentPos] & 15) << 16) | (data[currentPos+ 1 ] << 8) | (data[currentPos + 2]);
            currentPos += 3;
            halfByte = 0;
        } else {
            if(currentPos + 3 >= fileLength)
                break;

            value = (data[currentPos] << 12) | (data[currentPos + 1] << 4) | (data[currentPos + 2] >> 4);
            currentPos += 2;
            halfByte = 1;
        }

        unsigned char totalLength = 0;
        unsigned char remainingLength = 0;
        unsigned int remaining = 0;
        unsigned char canOutput = 0;
        unsigned char canOutput2 = 0;
        unsigned char canOutput3 = 0;
		unsigned int mbrValue = 0;
		unsigned char mbrLength = 0;

		// THIS CAUSES A MASSIVE (x2) SLOW DOWN
		value = permutatedAlphabet[value];
		if(value < (SYMBOL_SIZE - 2)) {
            value += 2;

            MBR_Int(value, &mbrValue, &mbrLength);
            totalLength = initialLength + mbrLength;

            canOutput = totalLength >= 8 ? 1 : 0;
            canOutput2 = totalLength >= 16 ? 1 : 0;
            canOutput3 = totalLength >= 24 ? 1 : 0;

            if(canOutput) {
                remainingLength = mbrLength - (8 - initialLength);
                unsigned char shiftedInitialValue = initialValue << (8 - initialLength);
                unsigned char shiftedMBRValue = mbrValue >> remainingLength;

                encodedData[currentIndex] = shiftedInitialValue | shiftedMBRValue;
                ++currentIndex;

                unsigned int bitMask = (1 << remainingLength) - 1;
                remaining = mbrValue & bitMask;

                if(canOutput2) {
                    remainingLength = remainingLength - 8;
                    encodedData[currentIndex] = remaining >> remainingLength;
                    ++currentIndex;

                    bitMask = (1 << remainingLength) - 1;
                    remaining = mbrValue & bitMask;

                    if(canOutput3) {
                        remainingLength = remainingLength - 8;
                        encodedData[currentIndex] = remaining >> remainingLength;
                        ++currentIndex;

                        bitMask = (1 << remainingLength) - 1;
                        remaining = mbrValue & bitMask;
                    }
                }

                initialValue = remaining;
                initialLength = remainingLength;
            } else {
                unsigned char shiftedInitialValue = initialValue << mbrLength;
                initialValue = shiftedInitialValue | mbrValue;
                initialLength = totalLength;
            }
		} else {
            totalLength = initialLength + D_SIZE;
            canOutput = totalLength >= 8 ? 1 : 0;
            canOutput2 = totalLength >= 16 ? 1 : 0;
            canOutput3 = totalLength >= 24 ? 1 : 0;

            if(value == (SYMBOL_SIZE - 2)) {
                if(canOutput) {
                    encodedData[currentIndex] = initialValue << (8 - initialLength);
                    ++currentIndex;

                    remainingLength = D_SIZE - (8 - initialLength);
                    remaining = 0;

                    if(canOutput2) {
                        encodedData[currentIndex] = 0;
                        ++currentIndex;

                        remainingLength = remainingLength - 8;

                        if(canOutput3) {
                            encodedData[currentIndex] = 0;
                            ++currentIndex;

                            remainingLength = remainingLength - 8;
                        }
                    }
                } else {
                    printf("This shall not happen!");
                }
            } else if(value == (SYMBOL_SIZE - 1)) {
                if(canOutput) {
                    remainingLength = D_SIZE - (8 - initialLength);
                    encodedData[currentIndex] = (initialValue << (8 - initialLength)) | (1 >> (remainingLength));
                    ++currentIndex;

                    if(canOutput2) {
                        remainingLength = remainingLength - 8;
                        if(remainingLength == 0) {
                            encodedData[currentIndex] = 1;
                            ++currentIndex;
                            remaining = 0;
                        } else {
                            encodedData[currentIndex] = 0;
                            ++currentIndex;
                            remaining = 1;

                            if(canOutput3) {
                                remainingLength = remainingLength - 8;
                                if(remainingLength == 0) {
                                    encodedData[currentIndex] = 1;
                                    ++currentIndex;
                                    remaining = 0;
                                } else {
                                    encodedData[currentIndex] = 0;
                                    ++currentIndex;
                                    remaining = 1;
                                }
                            }
                        }
                    }
                } else {
                    printf("This shall not happen!");
                }
            }

            mbrLength = D_SIZE;
            initialValue = remaining;
            initialLength = remainingLength;
		}

		disInfoRow = &(DItables[disInfoTableIndex].rows[mbrLength]);
        disInfoData[disInfoIndex] = disInfoRow->output;
		disInfoIndex = disInfoIndex + disInfoRow->canOutput;
		disInfoData[disInfoIndex] = disInfoRow->output2;
		disInfoIndex = disInfoIndex + disInfoRow-> canOutput2;
		disInfoData[disInfoIndex] = disInfoRow->output3;
		disInfoIndex = disInfoIndex + disInfoRow->canOutput3;
		disInfoTableIndex = disInfoRow->nextTableId;
	}

	if(disInfoTableIndex != 1) {
        unsigned char lastDIOutput, lastDIRemaining;
        MBR_Char(disInfoTableIndex, &lastDIOutput, &lastDIRemaining);
        disInfoData[disInfoIndex] = lastDIOutput << (8 - lastDIRemaining);
        ++disInfoIndex;
	}
	if(initialLength != 0) {
        encodedData[currentIndex] = initialValue << (8 - initialLength);
        ++currentIndex;
	}

	// Create Header
	unsigned char dType = 128;
	// 0    -> D4
	// 32   -> D8
	// 64   -> D12
	// 96   -> D16
	// 128  -> D20

	unsigned char skipAmount = 0;
	// 0    -> Don't skip any DisInfo length while decoding.
	// 8    -> Skip last DisInfo length while decoding.
	// 16   -> Skip last two DisInfo lengths while decoding.
	// 24   -> Skip last three DisInfo lengths while decoding.

	unsigned char remainingDataCount = 0;
	if(currentPos == fileLength - 3) {
        remainingDataCount = 6;
        disInfoData[1] = data[fileLength - 3]; // First byte of the remaining data
        disInfoData[2] = data[fileLength - 2]; // Second byte of the remaining data
        disInfoData[3] = data[fileLength - 1]; // Third byte of the remaining data
	} else if(currentPos == fileLength - 2) {
        remainingDataCount = 4;
        disInfoData[1] = data[fileLength - 2]; // First byte of the remaining data
        disInfoData[2] = data[fileLength - 1]; // Second byte of the remaining data
        disInfoData[3] = 0;                    // Third byte of the remaining data
	} else if(currentPos == fileLength - 1) {
        remainingDataCount = 2;
        disInfoData[1] = data[fileLength - 1]; // First byte of the remaining data
        disInfoData[2] = 0;                    // Second byte of the remaining data
        disInfoData[3] = 0;                    // Third byte of the remaining data
	}
	// 0    -> No remaining data left
	// 2    -> 1 byte of remaining data left
	// 4    -> 2 bytes of remaining data left
	// 6    -> 3 bytes of remaining data left

	disInfoData[0] = dType | skipAmount | remainingDataCount | seeded;

	FILE* fEncoded = fopen(payloadFileName, "wb");
    fwrite(encodedData, 1, currentIndex, fEncoded);
	fclose(fEncoded);

	FILE* fDisInfo = fopen(disInfoFileName, "wb");
    fwrite(disInfoData, 1, disInfoIndex, fDisInfo);
	fclose(fDisInfo);

	stopEncode = clock();
	elapsed_secs = (double)(stopEncode - startEncode) / CLOCKS_PER_SEC;
	printf("\r\nEncoding time: %f seconds.", elapsed_secs);
	printf("\r\nEncoding speed: %f MiB/sec\r\n", (fileLength / 1024.0f / 1024.0f) / elapsed_secs);

    long long int disInfoSize = disInfoIndex;
    long long int payloadSize = currentIndex;
    long long int totalSize = disInfoSize + payloadSize;
    long long int overheadSize = totalSize - fileLength;
    float overheadPercentage = (overheadSize * 100.0f) / totalSize;

    printf("\r\nOriginal Size: %lld", fileLength);
    printf("\r\nPayload Size: %lld", payloadSize);
    printf("\r\nPayload Percentage: %f", payloadSize * 100.0f / fileLength);
    printf("\r\nDisInfo Size: %lld", disInfoSize);
    printf("\r\nDisInfo Percentage: %f", disInfoSize * 100.0f / fileLength);
    printf("\r\nTotal Size: %lld", totalSize);
    printf("\r\nOverhead Size: %lld", overheadSize);
    printf("\r\nOverhead Percentage: %f\r\n\r\n", overheadPercentage);

    free(data);
	free(encodedData);
	free(disInfoData);
}

void Decode() {
	FILE *fPayload = fopen(payloadFileName, "rb");
	FILE *fDisInfo = fopen(disInfoFileName, "rb");
	clock_t start, stop;

	start = clock();
	fseek(fDisInfo, 0, SEEK_END);
	unsigned long long int fileLenDisInfo = ftell(fDisInfo);
	fseek(fDisInfo, 0, SEEK_SET);
	unsigned char* disInfoData = (unsigned char*)malloc((fileLenDisInfo) * sizeof(char));
	fread(disInfoData, 1, fileLenDisInfo, fDisInfo);
	fclose(fDisInfo);

	fseek(fPayload, 0, SEEK_END);
	unsigned long long int fileLenPayload = ftell(fPayload);
	fseek(fPayload, 0, SEEK_SET);
	unsigned char* payloadData = (unsigned char*)malloc((fileLenPayload) * sizeof(char));
	fread(payloadData, 1, fileLenPayload, fPayload);
	fclose(fPayload);
	stop = clock();

	double elapsed_secs = (double)(stop - start) / CLOCKS_PER_SEC;
	printf("Reading payload and disInfo took %f seconds.", elapsed_secs);

	clock_t decodeStart, decodeStop;
	decodeStart = clock();

	// we know for sure that decodedData size must be smaller than the total size of DisInfo and Payload.
	unsigned char* decodedData = (unsigned char*)malloc(fileLenDisInfo + fileLenPayload);
	unsigned char disInfoTableIndex = 0;
	unsigned long long int decodedDataIndex = 0;
	unsigned long long int currentPayloadIndex = 0;
	unsigned long long int currentPos = 4;

	unsigned int decodeTableIndex = 0;
    unsigned char halfByte = 0;
    unsigned char remainingPayloadByte = 0;
    unsigned char remainingLength = 0;

	unsigned char headerData = disInfoData[0];
	unsigned char dTypeData = headerData >> 5;      // Get first 3 most significant bits
	unsigned char skipData = (headerData >> 3) & 3; // Get 4th and 5th most significant bits
	unsigned char remainingCountData = (headerData >> 1) & 3; // Get 2nd and 3rd least significant bits
	unsigned char isSeededData = headerData & 1;    // Get the least significant bit.
	while (currentPos < fileLenDisInfo) {
		unsigned char data = disInfoData[currentPos];
		++currentPos;

		reverseDIHRow* row = &(rdiTM.tables[disInfoTableIndex].rows[data]);
		for (unsigned char i = 0; i < row->outputCount; i++)
		{
			unsigned char length = row->outputs[i];
			unsigned char skipRemaining = 0;
			if(length <= remainingLength) {
                decodeTableIndex = remainingPayloadByte >> (remainingLength - length);
                remainingLength = remainingLength - length;
                remainingPayloadByte = remainingPayloadByte & ((1 << remainingLength) - 1);
                length = 0;
                skipRemaining = 1;
			} else {
                length = length - remainingLength;
                decodeTableIndex = remainingPayloadByte;
			}

			while(length >= 8) {
                decodeTableIndex = (decodeTableIndex << 8) | payloadData[currentPayloadIndex];
                ++currentPayloadIndex;
                length = length - 8;
			}
			if(length != 0) {
                remainingLength = 8 - length;
                decodeTableIndex = (decodeTableIndex << length) | (payloadData[currentPayloadIndex] >> remainingLength);
                remainingPayloadByte = payloadData[currentPayloadIndex] & ((1 << remainingLength) - 1);
                ++currentPayloadIndex;
			} else if(skipRemaining == 0) {
                remainingPayloadByte = 0;
                remainingLength = 0;
			}

			unsigned int output = 0;
            if(row->outputs[i] != D_SIZE) {
                unsigned int codewordValue = ReverseMBR_Int(decodeTableIndex, row->outputs[i]);
                output = reversePermutatedAlphabet[codewordValue - 2];
            } else {
                if(decodeTableIndex == 0) {
                    output = reversePermutatedAlphabet[SYMBOL_SIZE - 2];
                } else if(decodeTableIndex == 1) {
                    output = reversePermutatedAlphabet[SYMBOL_SIZE - 1];
                }
            }

			if(halfByte) {
                decodedData[decodedDataIndex] |= (unsigned char)(output >> 16);
                ++decodedDataIndex;
                decodedData[decodedDataIndex] = (unsigned char)(output >> 8);
                ++decodedDataIndex;
                decodedData[decodedDataIndex] = (unsigned char)output;
                ++decodedDataIndex;

                halfByte = 0;
			} else {
                decodedData[decodedDataIndex] = (unsigned char)(output >> 12);
                ++decodedDataIndex;
                decodedData[decodedDataIndex] = (unsigned char)(output >> 4);
                ++decodedDataIndex;
                decodedData[decodedDataIndex] = (unsigned char)(output << 4);

                halfByte = 1;
			}
		}

		disInfoTableIndex = row->nextTableId;
	}

	if(remainingCountData == 3) {
        decodedData[decodedDataIndex++] = disInfoData[1];
        decodedData[decodedDataIndex++] = disInfoData[2];
        decodedData[decodedDataIndex++] = disInfoData[3];
	} else if(remainingCountData == 2) {
        decodedData[decodedDataIndex++] = disInfoData[1];
        decodedData[decodedDataIndex++] = disInfoData[2];
	} else if(remainingCountData == 1) {
        decodedData[decodedDataIndex++] = disInfoData[1];
	}

	FILE* fDecoded = fopen(originalFileName, "wb");
	fwrite(decodedData, 1, decodedDataIndex, fDecoded);
	fclose(fDecoded);

	decodeStop = clock();
	elapsed_secs = (double)(decodeStop - decodeStart) / CLOCKS_PER_SEC;
	printf("\r\nDecoding Time: %f seconds.", elapsed_secs);
	printf("\r\nDecoding Speed: %f MiB/sec\r\n\r\n", (decodedDataIndex) / 1024.0f / 1024.0f / elapsed_secs);

	free(decodedData);
	free(payloadData);
	free(disInfoData);
}

void PermutateAlphabet() {
	memset(permutatedAlphabet, 0, SYMBOL_SIZE * sizeof(int));
	memset(reversePermutatedAlphabet, 0, SYMBOL_SIZE * sizeof(int));

	if(seeded == 0) {
        for (unsigned int i = 0; i < SYMBOL_SIZE; i++)
        {
            permutatedAlphabet[i] = i;
            reversePermutatedAlphabet[i] = i;
        }
	} else {
        FILE* seedFile = fopen(seedFileName, "r");
        char line[SYMBOL_SIZE];
        while(fgets(line, SYMBOL_SIZE, seedFile)) {
            char* temp1 = strdup(line);
            char* temp2 = strdup(line);

            const char* value1Str = get_field(temp1, 1);
            const char* value2Str = get_field(temp2, 2);
            int value1 = atoi(value1Str);
            int value2 = atoi(value2Str);

            free(temp1);
            free(temp2);

            permutatedAlphabet[value1] = value2;
            reversePermutatedAlphabet[value2] = value1;
        }
	}
}

void CreateDisInfoTableMap() {
	for (unsigned char t = 1; t <= 255 && t > 0; t++)
	{
		disInfoTable table;
		memset(&table, 0, sizeof(disInfoTable));

		unsigned char initialValue = 0;
		unsigned char initialLength = 0;
		if (t != 1) {
			MBR_Char(t, &initialValue, &initialLength);
		}

		for (unsigned char r = 1; r <= D_SIZE; r++)
		{
			unsigned char value = 1;
			unsigned char length = D_SIZE - r;
			if (r == D_SIZE) {
				value = 0;
				length = D_SIZE - 1;
			}

			huffmanRow row;
			memset(&row, 0, sizeof(huffmanRow));

            unsigned char totalLength = 0;
            unsigned char remainingLength = 0;
            unsigned int remaining = 0;

			totalLength = initialLength + length;
			row.canOutput = totalLength >= 8 ? 1 : 0;
			row.canOutput2 = totalLength >= 16 ? 1 : 0;
			row.canOutput3 = totalLength >= 24 ? 1 : 0;

			if (row.canOutput) {
				remainingLength = length - (8 - initialLength);
				unsigned char shiftedInitialValue = initialValue << (8 - initialLength);
				unsigned char shiftedValue = value >> remainingLength;
				row.output = shiftedInitialValue | shiftedValue;

				unsigned int bitMask = (1 << remainingLength) - 1;
				remaining = value & bitMask;

				if(row.canOutput2) {
                    remainingLength = remainingLength - 8;
                    row.output2 = remaining >> (remainingLength);
                    bitMask = (1 << remainingLength) - 1;
                    remaining = value & bitMask;

                    if(row.canOutput3) {
                        remainingLength = remainingLength - 8;
                        row.output3 = remaining >> (remainingLength);
                        bitMask = (1 << remainingLength) - 1;
                        remaining = value & bitMask;
                    }
				}
			}
			else
			{
				row.output = 0;
				unsigned char shiftedInitialValue = initialValue << length;
				remaining = shiftedInitialValue | value;
				remainingLength = totalLength;	// the total length is the actual length since we didn't output anything.
			}

			row.nextTableId = ReverseMBR_Char((unsigned char)remaining, remainingLength);

			table.rows[r] = row;
		}

		DItables[t] = table;
	}
}

void CreateReverseDisInfoTableMap() {
    memset(&rdiTM, 0, sizeof(reverseDITableMap));

	// There should be only D_SIZE many table. Remaining value can only be zero, only its length can change. If there is a 1 bit in the remaining,
	// we can already calculate the corresponding symbol length. So the remaining value cannot contain any 1 bits.
	for (unsigned char t = 0; t < D_SIZE; t++) {
		reverseDITable table;
		memset(&table, 0, sizeof(reverseDITable));

		unsigned char initialValue = 0;
		unsigned char initialLength = t;

		for (unsigned char r = 0; r <= 255; r++) {
			reverseDIHRow row;
			memset(&row, 0, sizeof(reverseDIHRow));

			row.outputCount = 0;
			unsigned char totalLength = initialLength + 8;
			unsigned int currentValue = (initialValue << 8) | r;

			// left align the sequence.
			unsigned int shiftedValue = currentValue << (32 - totalLength);

			unsigned int leftMostBitMask = 2147483648;		// 10000000 00000000 => if shiftedValue & 32768 == 0, left most bit 0; else == 32768, left most bit 1.
			unsigned char zeroCount = 0;
			for (unsigned char i = 0; i < totalLength; i++) {
				unsigned char leftMostBit = (shiftedValue & leftMostBitMask) == 0 ? 0 : 1;
				shiftedValue <<= 1;
				if (leftMostBit == 0) {
					zeroCount++;

					if (zeroCount == D_SIZE - 1) {
						// If there are D_SIZE-1 zeros, symbol length is D_SIZE.
						// This will be D_SIZE
						row.outputs[row.outputCount] = D_SIZE;	// codeword 0000000 ->length D_SIZE
						row.outputCount++;

						zeroCount = 0;
					}
				}
				else {
					//we have found a symbol length.

					// zeroCount    -   Codeword    -   Length
					//      0               1           D_SIZE - 1
					//      1               01          D_SIZE - 2
					//      2               001         D_SIZE - 3
					//      ...             ...         ...
					//      D_SIZE - 2    (D_SIZE-2)01  1

					row.outputs[row.outputCount] = D_SIZE - (zeroCount + 1);
					row.outputCount++;
					zeroCount = 0;
				}
			}

			row.nextTableId = zeroCount;

			table.rows[r] = row;
			if (r == 255) {
				break;
			}
		}

		rdiTM.tables[t] = table;
	}
}

void MBR_Char(unsigned char value, unsigned char* mbrValue, unsigned char* mbrLength) {
	// MBR => Minimum Binary Representation, removes the most significant bit and returns the remaining value with its length.
	// Cannot calculate MBR of 0 and 1.
	if (value == 0 || value == 1) {
        unsigned char zero = 0;
        *mbrValue = zero;
        *mbrLength = zero;
        return;
	}

	unsigned char orgValue = value;		// cache the original value
	unsigned char mbrIndex = 0;			// variable to store where the most significant bit (MSB) is
	while (value != 1) {				    // shift the value to the right till it becomes 1, the shift amount will give us the MSB.
		value >>= 1;
		++mbrIndex;
	}

	unsigned char bitMask = (1 << mbrIndex) - 1;		// To get rid of the MSB, we have to and the value with 2^(MSBindex) - 1
	unsigned char mbr = orgValue & bitMask;				// Obtain MBR.

	// Ex. value = 23 : 00010111
	// MSB index is 4
	// bitMask : 2^4 - 1 = 15 : 00001111
	// 00010111 & 00001111 = 00000111 (MBR)
	// MBR length is actually 4 bits (0111)
	// so we have to return the length of the MBR as well.
	// MBR length is actually the MBR index.

    *mbrValue = mbr;
    *mbrLength = mbrIndex;
}

void MBR_Short(unsigned short value, unsigned short* mbrValue, unsigned char* mbrLength) {
	if (value == 0 || value == 1) {
        unsigned short zero = 0;
        *mbrValue = zero;
        *mbrLength = zero;
        return;
	}

	unsigned short orgValue = value;
	unsigned char mbrIndex = 0;
	while (value != 1) {
		value >>= 1;
		++mbrIndex;
	}

	unsigned short bitMask = (1 << mbrIndex) - 1;
	unsigned short mbr = orgValue & bitMask;

    *mbrValue = mbr;
    *mbrLength = mbrIndex;
}

void MBR_Int(unsigned int value, unsigned int* mbrValue, unsigned char* mbrLength) {
	if (value == 0 || value == 1) {
        unsigned int zero = 0;
        *mbrValue = zero;
        *mbrLength = zero;
        return;
	}

	unsigned int orgValue = value;
	unsigned char mbrIndex = 0;
	while (value != 1) {
		value >>= 1;
		++mbrIndex;
	}

	unsigned int bitMask = (1 << mbrIndex) - 1;
	unsigned int mbr = orgValue & bitMask;

    *mbrValue = mbr;
    *mbrLength = mbrIndex;
}

unsigned char ReverseMBR_Char(unsigned char value, unsigned char length) {
	// Ex. value = 7, length = 5 : 00111
	// Result should be 100111 : 39
	// (1 << length) : 100000 | 00111 = 100111
	return (1 << length) | value;
}

unsigned short ReverseMBR_Short(unsigned short value, unsigned char length) {
	return (1 << length) | value;
}

unsigned int ReverseMBR_Int(unsigned int value, unsigned char length) {
	return (1 << length) | value;
}
