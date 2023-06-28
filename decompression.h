#pragma once
#include "utils.h"

int decompressFile(HANDLE sourceFile, HANDLE destFile);

void FrameYUV_to_RGB(BYTE frame[CIF_Y][CIF_X][3]);

void FrameReverseDPCM(BYTE current_frame[CIF_Y][CIF_X][3], BYTE restore_frame[CIF_Y][CIF_X][3], int index);

//void FrameIDCT(BYTE* frame);
void FrameIDCT(BYTE frame[CIF_Y][CIF_X][3]);

void AraiBlockIDCT_2D(float block[8][8]);

void ChenBlockIDCT_2D(float block[8][8]);