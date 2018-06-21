#include "CodingParameters.h"
#include "HuffmanTableDataStructures.h"
#include "FunctionDeclerations.h"
#include "Utilities.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

unsigned char* permutatedAlphabet;
unsigned char* reversePermutatedAlphabet;
char* originalFileName;
char* payloadFileName;
char* disInfoFileName;

decodeTableMap* decodeTM;
lengthTableMap* lengthTM;
disInfoTableMap* diTM;
reverseDITableMap* rdiTM;
npfTableMap* npfTM;

int main(int argc, char **argv) {
	srand(SEED);

	unsigned char encode = HandleCommandArguments(argc, argv);
	if (encode == -1)
		return -1;

	InitializeTableMaps();

	if (encode) {
		FILE *fptr = fopen(originalFileName, "rb");
		clock_t start, stop;

		start = clock();
		fseek(fptr, 0, SEEK_END);
		unsigned long long int fileLen = ftell(fptr);
		fseek(fptr, 0, SEEK_SET);
		unsigned char* data = (unsigned char*)malloc((fileLen + 1) * sizeof(char));
		size_t bytesRead = fread(data, 1, fileLen, fptr);
		fclose(fptr);
		stop = clock();

		double elapsed_secs = (double)(stop - start) / CLOCKS_PER_SEC;
		printf("\r\nReading file took %f seconds.", elapsed_secs);

		Encode(data, fileLen + 8);

		free(data);
		free(payloadFileName);
		free(disInfoFileName);
	}
	else {
		Decode();
		free(originalFileName);
	}

	getchar();
}

int HandleCommandArguments(int argc, char** argv) {
	// if argc is equal to 2, we are encoding the specified file
	// if argc is equal to 3, we are decoding the payload with the disambiguation information
	// later on, when you want to decode, you have to specify the coding parameters (d_size, recursion level, seed for permutation (if permutation wanted)) as well 

	unsigned char encode = -1;
	if (argc == 2) {
		encode = 1;
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
		encode = 0;
		payloadFileName = argv[1];
		disInfoFileName = argv[2];

		// We want to add "_decoded.bin" to the end of the decoded file name.
		char* fileNameWithoutExtension = remove_ext(payloadFileName, '.', '/');
		int decodedFileNameLength = strlen(fileNameWithoutExtension) + 12 + 1;

		originalFileName = (char*)malloc(decodedFileNameLength + 1);
		snprintf(payloadFileName, decodedFileNameLength, "%s%s", fileNameWithoutExtension, "_decoded.bin");

		free(fileNameWithoutExtension);
	}

	return encode;
}

void InitializeTableMaps() {
	permutatedAlphabet = PermutateAlphabet();
	reversePermutatedAlphabet = ReversePermutatedAlphabet(permutatedAlphabet);
	decodeTM = CreateDecodeTableMap();
	lengthTM = CreateLengthTableMap();
	diTM = CreateDisInfoTableMap();
	rdiTM = CreateReverseDisInfoTableMap();
	npfTM = CreateNPFTableMap();
}

// TODO : Add recursive implementation of encoding
void Encode(unsigned char* data, long long int fileLength) {
	clock_t startEncode, stopEncode, startFileWrite, stopFileWrite;
	startEncode = clock();

	// We need to figure out how much space we need to allocate before hand.
	// approximately, disInfo takes 2/d size of the original data
	// payload takes d-2/d size of the original data
	unsigned char* encodedData = (unsigned char*)malloc(fileLength + 32);
	unsigned char* disInfoData = (unsigned char*)malloc(fileLength);

	long long int currentPos = 0;
	long long int currentIndex = 0;
	long long int disInfoIndex = 0;
	unsigned char disInfoTableIndex = 1;
	unsigned char tableIndex = 1;
	while (currentPos < fileLength) {
		unsigned char perm = data[currentPos++];
		huffmanRow* row = &(npfTM->tables[tableIndex].rows[perm]);
		encodedData[currentIndex] = row->output;
		currentIndex += row->canOutput;
		tableIndex = row->nextTableId;

		huffmanRow* disInfoRow = &(diTM->tables[disInfoTableIndex].rows[row->mbrLength]);
		disInfoData[disInfoIndex] = disInfoRow->output;
		disInfoIndex += disInfoRow->canOutput;
		disInfoTableIndex = disInfoRow->nextTableId;
	}
	// TODO : Handle last remaining bits if any...

	stopEncode = clock();
	double elapsed_secs_encode = (double)(stopEncode - startEncode) / CLOCKS_PER_SEC;
	printf("\r\nEncoding took %f seconds.", elapsed_secs_encode);

	startFileWrite = clock();

	FILE* fEncoded = fopen(payloadFileName, "wb");
	fwrite(encodedData, 1, currentIndex - 1, fEncoded);
	fclose(fEncoded);

	FILE* fDisInfo = fopen(disInfoFileName, "wb");
	fwrite(disInfoData, 1, disInfoIndex - 1, fDisInfo);
	fclose(fDisInfo);

	stopFileWrite = clock();
	double elapsed_secs_write = (double)(stopFileWrite - startFileWrite) / CLOCKS_PER_SEC;
	printf("\r\nWriting encoded files took %f seconds.", elapsed_secs_write);

	free(disInfoData);
	free(encodedData);
}

void Decode() {
	FILE *fPayload = fopen(payloadFileName, "rb");
	FILE *fDisInfo = fopen(disInfoFileName, "rb");
	clock_t start, stop;

	start = clock();
	fseek(fDisInfo, 0, SEEK_END);
	unsigned long long int fileLenDisInfo = ftell(fDisInfo);
	fseek(fDisInfo, 0, SEEK_SET);
	unsigned char* disInfoData = (unsigned char*)malloc((fileLenDisInfo + 1) * sizeof(char));
	fread(disInfoData, 1, fileLenDisInfo, fDisInfo);
	fclose(fDisInfo);

	fseek(fPayload, 0, SEEK_END);
	unsigned long long int fileLenPayload = ftell(fPayload);
	fseek(fPayload, 0, SEEK_SET);
	unsigned char* payloadData = (unsigned char*)malloc((fileLenPayload + 1) * sizeof(char));
	fread(payloadData, 1, fileLenPayload, fPayload);
	fclose(fPayload);
	stop = clock();

	double elapsed_secs = (double)(stop - start) / CLOCKS_PER_SEC;
	printf("Reading payload and disInfo took %f seconds.", elapsed_secs);

	clock_t decodeStart, decodeStop;
	decodeStart = clock();

	// we know for sure that decodedData size must be smaller than the total size of DisInfo and Payload.
	unsigned char* decodedData = (unsigned char*)malloc(fileLenDisInfo + fileLenPayload);
	long long int decodedDataIndex = 0;

	unsigned char currentPayloadIndex = 1;
	unsigned char currentPayloadByte = payloadData[currentPayloadIndex];
	unsigned char prevPayloadByte = payloadData[currentPayloadIndex];
	unsigned short payloadShort = prevPayloadByte << 8 | currentPayloadByte;
	unsigned char currentPayloadShortBitIndex = 0;

	unsigned char decodeTableIndex = payloadData[0];
	unsigned char disInfoTableIndex = 0;
	unsigned char lengthTableIndex = 0;

	long long int currentPos = 0;
	while (currentPos < fileLenDisInfo) {
		unsigned char data = disInfoData[currentPos++];

		reverseDIHRow* row = &(rdiTM->tables[disInfoTableIndex].rows[data]);
		for (unsigned char i = 0; i < row->outputCount; i++)
		{
			int length = row->outputs[i];

			lengthRow* lengthRow = &(lengthTM->tables[lengthTableIndex].rows[length - 1]);
			lengthTableIndex = lengthRow->remainingLength;
			currentPayloadIndex += lengthRow->getNextByte;
			prevPayloadByte = currentPayloadByte;
			currentPayloadByte = payloadData[currentPayloadIndex];
			payloadShort = prevPayloadByte << 8 | currentPayloadByte;

			decodeNPFRow* currentDecodeRow = &(decodeTM->tables[decodeTableIndex].rows[length - 1]);
			decodedData[decodedDataIndex++] = currentDecodeRow->output;

			unsigned char shiftedData = (payloadShort << currentPayloadShortBitIndex) >> (8 + currentDecodeRow->remainingLength);

			decodeTableIndex = currentDecodeRow->remaining | shiftedData;

			currentPayloadShortBitIndex = lengthRow->remainingLength;
		}

		disInfoTableIndex = row->nextTableId;
	}

	decodeStop = clock();
	elapsed_secs = (double)(decodeStop - decodeStart) / CLOCKS_PER_SEC;
	printf("\r\nDecoding took %f seconds.", elapsed_secs);

	FILE* fDecoded = fopen(originalFileName, "wb");
	fwrite(decodedData, 1, decodedDataIndex - 1, fDecoded);
	fclose(fDecoded);

	free(decodedData);
	printf("\r\nDecode completed.");
}

// TODO : There seems to be an error here, fix it.
// TODO : Might rename this method as CreateAlphabet
unsigned char* PermutateAlphabet() {
	// C does not advocate to return the address of a local variable to outside of the function,
	// so you would have to define the local variable as static variable if you want to return it.
	static unsigned char permutatedAlphabet[256];
	static unsigned char alphabet[256];
	short currentLength = 256;
	for (unsigned char i = 0; i < 255; i++)
	{
		alphabet[i] = i;
	}
	alphabet[255] = 255;

	if (!PERMUTATE_ALPHABET)
		return alphabet;

	short permutatedLength = 0;
	while (currentLength != 0) {
		short randomIndex = rand() % currentLength;
		permutatedAlphabet[permutatedLength] = alphabet[randomIndex];
		permutatedLength++;
		currentLength--;

		// shift the alphabet to fill the gap of the character we took out
		for (unsigned char startIndex = randomIndex; startIndex < currentLength; startIndex++) {
			alphabet[startIndex] = alphabet[startIndex + 1];
		}
	}
	permutatedAlphabet[255] = alphabet[0];

	return permutatedAlphabet;
}

// TODO : Might rename this method as ReverseCreatedAlphabet
unsigned char* ReversePermutatedAlphabet(unsigned char* permutatedAlphabet) {
	if (!PERMUTATE_ALPHABET)
		return permutatedAlphabet;

	static unsigned char reverseAlphabet[256];
	for (short i = 0; i < 256; i++) {
		reverseAlphabet[permutatedAlphabet[i]] = i;
	}

	return reverseAlphabet;
}

decodeTableMap* CreateDecodeTableMap() {
	static decodeTableMap decodeTableMap;

	for (unsigned char t = 0; t <= 255; t++) {
		decodeNPFTable decodeTable;
		decodeTable.tableId = t;

		for (unsigned char r = 0; r < 8; r++) {
			decodeNPFRow row;
			unsigned char length = r + 1;
			unsigned char codeword = t >> (8 - length);
			if (length != 8) {
				unsigned char codewordValue = ReverseMBR(codeword, length);
				row.output = reversePermutatedAlphabet[codewordValue - 2];
			}
			else {
				if (t == 0)	// if length is 8, this can only be 0000000 => 254
					row.output = reversePermutatedAlphabet[254];
				else if (t == 1) // if length is 8, this can only be 00000001 => 255
					row.output = reversePermutatedAlphabet[255];
			}
			row.remaining = t << (length);
			row.remainingLength = 8 - (length);

			decodeTable.rows[r] = row;
		}

		decodeTableMap.tables[t] = decodeTable;
		if (t == 255)
			break;
	}

	return &decodeTableMap;
}

lengthTableMap* CreateLengthTableMap() {
	static lengthTableMap lengthTableMap;

	for (unsigned char t = 0; t < 8; t++)
	{
		lengthTable table;

		for (unsigned char r = 0; r < 8; r++)
		{
			lengthRow row;

			unsigned char totalLength = t + (r + 1);
			if (totalLength >= 8) {
				row.getNextByte = 1;
				row.remainingLength = totalLength - 8;
			}
			else {
				row.getNextByte = 0;
				row.remainingLength = totalLength;
			}

			table.rows[r] = row;
		}

		lengthTableMap.tables[t] = table;
	}

	return &lengthTableMap;
}

reverseDITableMap* CreateReverseDisInfoTableMap() {
	static reverseDITableMap reverseDIMap;

	// There should be only 7 table. Remaining value can only be zero, only its length can change. If there is a 1 bit in the remaining,
	// we can already calculate the corresponding symbol length. So the remaining value cannot contain any 1 bits.
	for (unsigned char t = 0; t < 8; t++) {
		reverseDITable table;
		table.tableId = t;

		unsigned char initialValue = 0;
		unsigned char initialLength = t;

		for (unsigned char r = 0; r <= 255; r++) {
			reverseDIHRow row;

			row.input = r;
			row.outputCount = 0;
			unsigned char totalLength = initialLength + 8;
			unsigned short currentValue = (initialValue << 8) | r;

			// left align the sequence.
			unsigned short shiftedValue = currentValue;
			for (unsigned char shift = 0; shift < 16 - totalLength; shift++) {
				shiftedValue <<= 1;
			}

			unsigned short leftMostBitMask = 32768;		// 10000000 00000000 => if shiftedValue & 32768 == 0, left most bit 0; else == 32768, left most bit 1.
			unsigned char zeroCount = 0;
			for (unsigned char i = 0; i < totalLength; i++) {
				unsigned char leftMostBit = (shiftedValue & leftMostBitMask) == 0 ? 0 : 1;
				shiftedValue <<= 1;
				if (leftMostBit == 0) {
					zeroCount++;

					if (zeroCount == 7) {
						// If there are seven zeros, this symbols length is 8.
						row.outputs[row.outputCount] = 8;	// codeword 0000000 ->length 8
						row.outputCount++;

						zeroCount = 0;
					}
				}
				else {
					//we have found a symbol length.
					if (zeroCount == 0)
						row.outputs[row.outputCount] = 7;	// codeword 1 -> length 7
					else if (zeroCount == 1)
						row.outputs[row.outputCount] = 6;	// codeword 01 -> length 6
					else if (zeroCount == 2)
						row.outputs[row.outputCount] = 5;	// codeword 001 -> length 5
					else if (zeroCount == 3)
						row.outputs[row.outputCount] = 4;	// codeword 0001 -> length 4
					else if (zeroCount == 4)
						row.outputs[row.outputCount] = 3;	// codeword 00001 -> length 3
					else if (zeroCount == 5)
						row.outputs[row.outputCount] = 2;	// codeword 000001 -> length 2
					else if (zeroCount == 6)
						row.outputs[row.outputCount] = 1;	// codeword 0000001 -> length 1

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

		reverseDIMap.tables[t] = table;
	}

	return &reverseDIMap;
}

disInfoTableMap* CreateDisInfoTableMap() {
	static disInfoTableMap disInfoMap;
	for (unsigned char t = 1; t <= 255 && t > 0; t++)
	{
		disInfoTable table;
		table.tableId = t;

		unsigned char initialValue = 0;
		unsigned char initialLength = 0;
		if (t != 1) {
			unsigned char* mbrResult = MBR(t);
			initialValue = mbrResult[0];
			initialLength = mbrResult[1];
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
			row.input = r;
			row.totalLength = initialLength + length;
			if (row.totalLength >= D_SIZE)
				row.canOutput = 1;
			else
				row.canOutput = 0;

			if (row.canOutput) {
				unsigned char shiftedInitialValue = initialValue << (D_SIZE - initialLength);
				unsigned char shiftedValue = value >> (length - (D_SIZE - initialLength));
				row.output = shiftedInitialValue | shiftedValue;

				row.remainingLength = length - (D_SIZE - initialLength);
				unsigned char bitMask = (1 << row.remainingLength) - 1;
				row.remaining = value & bitMask;
			}
			else
			{
				row.output = 0;
				unsigned char shiftedInitialValue = initialValue << length;
				row.remaining = shiftedInitialValue | value;
				row.remainingLength = row.totalLength;	// the total length is the actual length since we didn't output anything.
			}

			row.nextTableId = ReverseMBR(row.remaining, row.remainingLength);

			table.rows[r] = row;
		}

		disInfoMap.tables[t] = table;
	}

	return &disInfoMap;
}

npfTableMap* CreateNPFTableMap() {
	static npfTableMap newMap;
	// if D = 8, this many huffman table there wil be.
	for (unsigned char t = 1; t <= 255 && t > 0; t++) {
		npfTable table;
		table.tableId = t;

		unsigned char initialValue = 0;
		unsigned char initialLength = 0;
		if (t != 1) {
			unsigned char* initialMBRResult = MBR(t);
			initialValue = initialMBRResult[0];
			initialLength = initialMBRResult[1];
		}

		// for each table, create rows.
		for (unsigned char r = 0; r <= 255; r++)
		{
			huffmanRow row;

			unsigned char value = permutatedAlphabet[r];
			if (value <= 253)
				value += 2;
			else {
				row.input = r;
				row.totalLength = initialLength + D_SIZE;
				row.canOutput = 1;

				if (value == 254) {					// Consider this row as always D_SIZE many zeroes : 00000000
					row.output = initialValue << (D_SIZE - initialLength);
					row.remainingLength = D_SIZE - (D_SIZE - initialLength);
					row.remaining = 0;
				}
				else if (value == 255) {			// Consider this row as always (D_SIZE - 1) many zeroes + 1 : 00000001
					if (initialLength == 0) {
						row.output = 1;
						row.remainingLength = 0;
						row.remaining = 0;
					}
					else {
						row.output = initialValue << (D_SIZE - initialLength);
						row.remainingLength = D_SIZE - (D_SIZE - initialLength);
						row.remaining = 1;
					}
				}
				row.nextTableId = ReverseMBR(row.remaining, row.remainingLength);

				table.rows[r] = row;

				if (r == 255)
					break;
				continue;
			}

			unsigned char* mbrResult = MBR(value);
			unsigned char mbrValue = mbrResult[0];
			unsigned char mbrLength = mbrResult[1];

			row.input = r;
			row.mbrLength = mbrLength;
			row.totalLength = initialLength + mbrLength;
			if (row.totalLength >= D_SIZE)	// if more than or equal to 8 bits, output
				row.canOutput = 1;
			else
				row.canOutput = 0;

			if (row.canOutput) {
				// initialValue comes from the table's own starting value.
				// If we add another MBR value to it and the length surpasses D_SIZE (8 bits),
				// we have to output the first 8 bits.
				// Firstly, we shift right the initialValue to make space for the new MBR value.
				// This shifting amount will be determined by how many bits we can shift to left without
				// losing any bit of information. If the initialValue's length (initialLength) is 5 bits,
				// we have to shift it 8 - 5 = 3 bits. 
				// After making space for the upcoming MBR bits, we have to calculate how many of the
				// MBR bits we can concat to the end of the shiftedInitialValue. Lets say that MBR length
				// is 5 bits. We have only 3 bits of space to concat. So we have to shift 5 - 3 = 2 times right to
				// get rid of unnecessary bits (bits that won't be written quite yet). 
				// Finally we apply or bitwise operation to shiftedInitialValue and shiftedMBRValue to get the output.
				// The right shifted bits of MBR is the remaining bits.

				unsigned char shiftedInitialValue = initialValue << (D_SIZE - initialLength);
				unsigned char shiftedMBRValue = mbrValue >> (mbrLength - (D_SIZE - initialLength));
				row.output = shiftedInitialValue | shiftedMBRValue;

				// same logic in MBR is applied here. We need to get the last remaningLength bits of MBRValue.
				// So we are aplying a bitMask to remove first 8 - remaningLength of bits.
				row.remainingLength = mbrLength - (D_SIZE - initialLength);
				unsigned char bitMask = (1 << row.remainingLength) - 1;
				row.remaining = mbrValue & bitMask;

				row.nextTableId = ReverseMBR(row.remaining, row.remainingLength);
			}
			else {
				row.output = 0;
				unsigned char shiftedInitialValue = initialValue << mbrLength;
				row.remaining = initialValue | mbrValue;
				row.remainingLength = row.totalLength;	// the total length is the actual length since we didn't output anything.

				row.nextTableId = ReverseMBR(row.remaining, row.remainingLength);
			}

			table.rows[r] = row;

			if (r == 255)
				break;
		}

		newMap.tables[t] = table;
	}

	return &newMap;
}

unsigned char* MBR(unsigned char value) {
	// MBR => Minimum Binary Representation, removes the most significant bit and returns the remaining value with its length.
	// Cannot calculate MBR of 0 and 1.
	if (value == 0 || value == 1) {
		unsigned char invalid[2] = { 0, 0 };
		return invalid;
	}

	unsigned char orgValue = value;		// cache the original value
	unsigned char mbrIndex = 0;			// variable to store where the most significant bit (MSB) is
	while (value != 1) {				// shift the value to the right till it becomes 1, the shift amount will give us the MSB.
		value >>= 1;
		mbrIndex++;
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

	unsigned char result[2] = { mbr, mbrIndex };
	return result;
}

unsigned char ReverseMBR(unsigned char value, unsigned char length) {
	// Ex. value = 7, length = 5 : 00111
	// Result should be 100111 : 39
	// (1 << length) : 100000 | 00111 = 100111
	return (1 << length) | value;
}