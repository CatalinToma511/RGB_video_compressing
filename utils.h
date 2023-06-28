#pragma once

#include <windows.h>

#define CIF_X 352
#define CIF_Y 288
#define FRAME_SIZE (CIF_X * CIF_Y * 3) // X*Y*3 bytes

#define KEY_FRAME_OFFSET 32

/// ci = cos(i*pi/16)
#define c1 0.9807852804032304491262 // cos(pi/16)
#define c2 0.9238795325112867561282 // cos(2*pi/16) = cos(pi/8)
#define c3 0.8314696123025452370788 // cos(3*pi/16)
#define c4 0.7071067811865475244008 // cos(4*pi/16) = cos(pi/4)
#define c5 0.5555702330196022247428 // cos(5*pi/16)
#define c6 0.3826834323650897717285 // cos(6*pi/16) = cos(3*pi/8)
#define c7 0.1950903220161282678483 // cos(7*pi/16)
#define sqrt2 1.41421356237

BOOL ReadBuffer(HANDLE hFile, void* buffer, unsigned int bufferSize, unsigned int offset);

BOOL ReadFrame(HANDLE hFile, long long nFrameNr, BYTE buffer[CIF_Y][CIF_X][3], long long offset = 0);

BOOL WriteBuffer(HANDLE hFile, void* buffer, unsigned int bufferSize, unsigned int offset);

BOOL WriteFrame(HANDLE hFile, long long nFrameNr, BYTE buffer[CIF_Y][CIF_X][3], long long offset = 0);