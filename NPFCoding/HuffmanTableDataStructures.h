#pragma once

#include "CodingParameters.h"

#pragma region NPF Coding Structs
// Each row in a npf huffman table is represented by this data structure.
// Holds most of the important data.
typedef struct huffmanRow {
	unsigned char input;
	unsigned char mbrLength;
	unsigned char totalLength;
	unsigned char canOutput;
	unsigned char output;
	unsigned char remaining;
	unsigned char remainingLength;
	unsigned char nextTableId;
};

// Each table has its own id and rows.
typedef struct npfTable {
	unsigned char tableId;
	huffmanRow rows[SYMBOL_SIZE];
};

// Holds all npf huffman tables for ease of access.
typedef struct npfTableMap {
	npfTable tables[SYMBOL_SIZE];
};
#pragma endregion

#pragma region DisInfo Coding Structs
// Huffman Table structure for storing DisInfo rows. 
// Created a new data structure because DisInfo uses less table and rows.
typedef struct disInfoTable {
	unsigned char tableId;
	huffmanRow rows[D_SIZE + 1];
};

// Holds all DisInfo Huffman Table structure for ease of access.
typedef struct disInfoTableMap {
	disInfoTable tables[SYMBOL_SIZE];
};
#pragma endregion

#pragma region NPF Decoding Structs
typedef struct decodeNPFRow {
	unsigned char output;
	unsigned char remaining;
	unsigned char remainingLength;
};

typedef struct decodeNPFTable {
	unsigned char tableId;
	decodeNPFRow rows[8];
};

typedef struct decodeTableMap {
	decodeNPFTable tables[256];
};

typedef struct lengthRow {
	unsigned char getNextByte;
	unsigned char remainingLength;
};

typedef struct lengthTable {
	lengthRow rows[8];
};

typedef struct lengthTableMap {
	lengthTable tables[8];
};
#pragma endregion

#pragma region DisInfo Decoding Structs
// This data structure is used in decoding the encoded DisInfo.
// This row can have more than one output and doesn't use input length.
typedef struct reverseDIHRow {	// Reverse DisInfo Huffman Row
	unsigned char input;
	unsigned char outputs[8];	// a byte can contain at most 8 outputs.
	unsigned char outputCount;
	unsigned char nextTableId;
};

typedef struct reverseDITable {	// Reverse DisInfo Table
	unsigned char tableId;
	reverseDIHRow rows[SYMBOL_SIZE];
};

typedef struct reverseDITableMap {		// Reverse DisInfo Table Map
	reverseDITable tables[8];
};	
#pragma endregion