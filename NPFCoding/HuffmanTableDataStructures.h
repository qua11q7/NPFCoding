#pragma once

#include "CodingParameters.h"

#pragma region NPF Coding Structs
// Each row in a npf huffman table is represented by this data structure.
// Holds most of the important data.
typedef struct {
	unsigned char input;
	unsigned char mbrLength;
	unsigned char totalLength;
	unsigned char canOutput;
	unsigned char output;
	unsigned char remaining;
	unsigned char remainingLength;
	unsigned char nextTableId;
} huffmanRow;

// Each table has its own id and rows.
typedef struct {
	unsigned char tableId;
	huffmanRow rows[SYMBOL_SIZE];
} npfTable;

// Holds all npf huffman tables for ease of access.
typedef struct {
	npfTable tables[SYMBOL_SIZE];
} npfTableMap;
#pragma endregion

#pragma region DisInfo Coding Structs
// Huffman Table structure for storing DisInfo rows. 
// Created a new data structure because DisInfo uses less table and rows.
typedef struct {
	unsigned char tableId;
	huffmanRow rows[D_SIZE + 1];
} disInfoTable;

// Holds all DisInfo Huffman Table structure for ease of access.
typedef struct {
	disInfoTable tables[SYMBOL_SIZE];
} disInfoTableMap;
#pragma endregion

#pragma region NPF Decoding Structs
typedef struct {
	unsigned char output;
	unsigned char remaining;
	unsigned char remainingLength;
} decodeNPFRow;

typedef struct {
	unsigned char tableId;
	decodeNPFRow rows[8];
} decodeNPFTable;

typedef struct {
	decodeNPFTable tables[256];
} decodeTableMap;

typedef struct {
	unsigned char getNextByte;
	unsigned char remainingLength;
} lengthRow;

typedef struct {
	lengthRow rows[8];
} lengthTable;

typedef struct {
	lengthTable tables[8];
} lengthTableMap;
#pragma endregion

#pragma region DisInfo Decoding Structs
// This data structure is used in decoding the encoded DisInfo.
// This row can have more than one output and doesn't use input length.
typedef struct {	// Reverse DisInfo Huffman Row
	unsigned char input;
	unsigned char outputs[8];	// a byte can contain at most 8 outputs.
	unsigned char outputCount;
	unsigned char nextTableId;
} reverseDIHRow;

typedef struct {	// Reverse DisInfo Table
	unsigned char tableId;
	reverseDIHRow rows[SYMBOL_SIZE];
} reverseDITable;

typedef struct {		// Reverse DisInfo Table Map
	reverseDITable tables[8];
} reverseDITableMap;
#pragma endregion