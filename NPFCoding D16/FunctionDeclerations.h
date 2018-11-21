#pragma once

#include "HuffmanTableDataStructures.h"

void HandleCommandArguments(int, char**, unsigned char*, unsigned char*);
void InitializeTableMaps();
void Encode();
void Decode();
void PermutateAlphabet();
void CreateReverseDisInfoTableMap();
void CreateDisInfoTableMap();
void MBR_Char(unsigned char, unsigned char*, unsigned char*);
void MBR_Short(unsigned short, unsigned short*, unsigned short*);
unsigned char ReverseMBR_Char(unsigned char, unsigned char);
unsigned short ReverseMBR_Short(unsigned short, unsigned char);
