#pragma once

#include "CodingParameters.h"

// Each row in a npf huffman table is represented by this data structure.
// Holds most of the important data.
typedef struct {
	unsigned char canOutput;
	unsigned char output;
	unsigned char canOutput2;
	unsigned char output2;
	unsigned char nextTableId;
} huffmanRow;

// Huffman Table structure for storing DisInfo rows.
// Created a new data structure because DisInfo uses less rows.
typedef struct {
	huffmanRow rows[D_SIZE + 1];
} disInfoTable;

// Holds all DisInfo Huffman Table structure for ease of access.
typedef struct {
	disInfoTable tables[256];
} disInfoTableMap;

// This data structure is used in decoding the encoded DisInfo.
// This row can have more than one output and doesn't use input length.
typedef struct {	// Reverse DisInfo Huffman Row
	unsigned char outputs[8];	// a byte can contain at most 8 outputs.
	unsigned char outputCount;
	unsigned char nextTableId;
} reverseDIHRow;

typedef struct {	// Reverse DisInfo Table
	reverseDIHRow rows[256];
} reverseDITable;

typedef struct {		// Reverse DisInfo Table Map
	reverseDITable tables[D_SIZE];
} reverseDITableMap;
