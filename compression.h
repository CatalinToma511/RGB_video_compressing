#pragma once
#include"utils.h"

int compressFile(HANDLE sFile, HANDLE dFile, int quality);

void FrameRGB_to_YUV(BYTE frame[CIF_Y][CIF_X][3]);

void FrameDPCM(BYTE current_frame[CIF_Y][CIF_X][3], BYTE restore_frame[CIF_Y][CIF_X][3], int index);

void FrameFDCT(BYTE frame[CIF_Y][CIF_X][3], int quality = 100);

void AraiBlockFDCT_2D(float block[8][8]);

void makeQuantizationTables(int quality, BYTE luminanceTable[8][8], BYTE chromaTable[8][8]);

void ChenBlockFDCT_2D(float block[8][8]);