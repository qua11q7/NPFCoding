#pragma once

#include "HuffmanTableDataStructures.h"

int HandleCommandArguments(int, char**);
void InitializeTableMaps();
void Encode(unsigned char*, long long int);
void Decode();
unsigned char* PermutateAlphabet();
unsigned char* ReversePermutatedAlphabet(unsigned char*);
decodeTableMap* CreateDecodeTableMap();
lengthTableMap* CreateLengthTableMap();
reverseDITableMap* CreateReverseDisInfoTableMap();
disInfoTableMap* CreateDisInfoTableMap();
npfTableMap* CreateNPFTableMap();
unsigned char* MBR(unsigned char);
unsigned char ReverseMBR(unsigned char, unsigned char);