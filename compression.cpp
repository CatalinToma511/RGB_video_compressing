#include <stdio.h>
#include <windows.h>

#include "compression.h"
#include "utils.h"

int compressFile(HANDLE sFile, HANDLE dFile, int quality)
{
	unsigned long long nFrames;
	LARGE_INTEGER FileSize;
	GetFileSizeEx(sFile, &FileSize);
	nFrames = FileSize.QuadPart / FRAME_SIZE;

	wprintf(L"%llu frames found in source (uncompressed) file\n", nFrames);

	BYTE current_frame[CIF_Y][CIF_X][3];
	BYTE restore_frame[CIF_Y][CIF_X][3];


	BOOL bSuccess;
	// crearea headerului fisierului
	BYTE sign[3] = { 'S', 'M', 'P'};
	UINT16 width = 352, height = 288;
	UINT32 frames = nFrames;
	BYTE q = (BYTE)quality;

	unsigned int headerOffset = 0;
	bSuccess = WriteBuffer(dFile, (LPCVOID*)sign, 3 * sizeof(BYTE), headerOffset);
	if (!bSuccess) {
		wprintf(L"Error: failed to write destination file header (signature)\n");
		return -7;
	}
	headerOffset += 3 * sizeof(BYTE);

	bSuccess = WriteBuffer(dFile, &width, sizeof(UINT16), headerOffset);
	if (!bSuccess) {
		wprintf(L"Error: failed to write destination file header (width)\n");
		return -8;
	}
	headerOffset += sizeof(UINT16);

	bSuccess = WriteBuffer(dFile, &height, sizeof(UINT16), headerOffset);
	if (!bSuccess) {
		wprintf(L"Error: failed to write destination file header (height)\n");
		return -9;
	}
	headerOffset += sizeof(UINT16);

	bSuccess = WriteBuffer(dFile, &frames, sizeof(UINT32), headerOffset);
	if (!bSuccess) {
		wprintf(L"Error: failed to write destination file header (frames)\n");
		return -10;
	}
	headerOffset += sizeof(UINT32);

	bSuccess = WriteBuffer(dFile, &q, sizeof(BYTE), headerOffset);
	if (!bSuccess) {
		wprintf(L"Error: failed to write destination file header (signature)\n");
		return -11;
	}
	headerOffset += sizeof(BYTE);

	wprintf(L"Created destination file header (%u bytes)\n", headerOffset);


	for (unsigned int i = 0; i < nFrames; ++i)
	{
		bSuccess = ReadFrame(sFile, i, current_frame);
		if (!bSuccess)
		{
			wprintf(L"Error: failed to read frame %u. Exiting...\n", i);
			return -5;
		}

		// RGB la YCbCr
		FrameRGB_to_YUV(current_frame);

		// DPCM
		FrameDPCM(current_frame, restore_frame, i);

		// FDCT
		FrameFDCT(current_frame, quality);


		bSuccess = WriteFrame(dFile, i, current_frame, headerOffset);
		if (!bSuccess)
		{
			wprintf(L"Error: failed to write frame %u. Exiting...\n", i);
			return -6;
		}
	}

	return 0;
}

void FrameRGB_to_YUV(BYTE frame[CIF_Y][CIF_X][3])
{
	int r, g, b, y, u, v;
	unsigned int i;

	for (int j = 0; j < CIF_X; ++j)
		for (int i = 0; i < CIF_Y; ++i)
		{
			b = frame[i][j][0];
			g = frame[i][j][1];
			r = frame[i][j][2];

			y = (77 * r + 150 * g + 29 * b) >> 8;
			u = ((-43 * r - 84 * g + 127 * b) >> 8) + 128;
			v = ((127 * r - 106 * g - 21 * b) >> 8) + 128;

			frame[i][j][0] = (BYTE)y;
			frame[i][j][1] = (BYTE)u;
			frame[i][j][2] = (BYTE)v;
		}
}

void FrameDPCM(BYTE current_frame[CIF_Y][CIF_X][3], BYTE restore_frame[CIF_Y][CIF_X][3], int index)
{
	if (index % KEY_FRAME_OFFSET != 0) //daca nu e key frame
	{
		// se calculeaza diferenta pentru fiecare pixel, pentru fiecare culoare
		for (int j = 0; j < CIF_X; ++j)
			for (int i = 0; i < CIF_Y; ++i)
				for (int k = 0; k < 3; ++k)
		{
			current_frame[i][j][k] = ((short)current_frame[i][j][k] - (short)restore_frame[i][j][k]) / 2 + 128;
			restore_frame[i][j][k] = (short)restore_frame[i][j][k] + ((short)current_frame[i][j][k] - 128) * 2;
		}
	}
	else //daca e key frame
	{
		memcpy_s(restore_frame, FRAME_SIZE, current_frame, FRAME_SIZE);
	}
}

void FrameFDCT(BYTE frame[CIF_Y][CIF_X][3], int quality)
{
	unsigned short row, col, chn, i, j;
	float block[8][8];

	BYTE luminance[8][8], chroma[8][8];
	BYTE quantized, tableVal;
	makeQuantizationTables(quality, luminance, chroma);

	for (row = 0; row < CIF_X; row += 8)
	{
		for (col = 0; col < CIF_Y; col += 8)
		{
			for (chn = 0; chn < 3; ++chn)
			{
				for (j = 0; j < 8; j++)
					for (i = 0; i < 8; i++)
						block[j][i] = frame[col + j][row + i][chn] - 128.0f;

				//FDCT 2D pe block
				AraiBlockFDCT_2D(block);
				//ChenBlockFDCT_2D(block);

				for (j = 0; j < 8; j++)
					for (i = 0; i < 8; i++)
					{
						//verific daca folosesc tabelul de luminanta sau crominanta
						tableVal = (chn == 0) ? luminance[j][i] : chroma[j][i];
						quantized = block[j][i] / tableVal;
						block[j][i] = quantized * tableVal;
						frame[col + j][row + i][chn] = (BYTE)((block[j][i] + 1023.0f) / 8);
					}
			}
		}
	}
}

void AraiBlockFDCT_2D(float block[8][8])
{
	float p1[8], p2[8], p3[8], aux, p4[8], p5[8];
	// algoritmul lui Arai
	// pe linii
	for (int i = 0; i < 8; ++i)
	{
		// faza 1
		p1[0] = block[i][0] + block[i][7];
		p1[1] = block[i][1] + block[i][6];
		p1[2] = block[i][2] + block[i][5];
		p1[3] = block[i][3] + block[i][4];
		p1[4] = block[i][3] - block[i][4];
		p1[5] = block[i][2] - block[i][5];
		p1[6] = block[i][1] - block[i][6];
		p1[7] = block[i][0] - block[i][7];

		// faza 2
		p2[0] = p1[0] + p1[3];
		p2[1] = p1[1] + p1[2];
		p2[2] = p1[1] - p1[2];
		p2[3] = p1[0] - p1[3];
		p2[4] = -p1[4] - p1[5];
		p2[5] = p1[5] + p1[6];
		p2[6] = p1[6] + p1[7];
		p2[7] = p1[7];

		// faza 3
		p3[0] = p2[0] + p2[1];
		p3[1] = p2[0] - p2[1];
		p3[2] = (p2[2] + p2[3]) * c4;
		p3[3] = p2[3];
		aux = (p2[4] + p2[6]) * c6;
		p3[4] = -p2[4] * (c2 - c6) - aux;
		p3[5] = p2[5] * c4;
		p3[6] = p2[6] * (c2 + c6) - aux;
		p3[7] = p2[7];

		// faza 4
		p4[0] = p3[0];
		p4[1] = p3[1];
		p4[2] = p3[2] + p3[3];
		p4[3] = p3[3] - p3[2];
		p4[4] = p3[4];
		p4[5] = p3[5] + p3[7];
		p4[6] = p3[6];
		p4[7] = p3[7] - p3[5];

		// faza 5
		p5[0] = p4[0] / 2 * sqrt2;
		p5[1] = p4[1] / 4 * c4;
		p5[2] = p4[2] / 4 * c2;
		p5[3] = p4[3] / 4 * c6;
		p5[4] = (p4[4] + p4[7]) / 4 * c5;
		p5[5] = (p4[5] + p4[6]) / 4 * c1;
		p5[6] = (p4[5] - p4[6]) / 4 * c7;
		p5[7] = (p4[7] - p4[4]) / 4 * c3;

		block[i][0] = p5[0];
		block[i][1] = p5[5];
		block[i][2] = p5[2];
		block[i][3] = p5[7];
		block[i][4] = p5[1];
		block[i][5] = p5[4];
		block[i][6] = p5[3];
		block[i][7] = p5[6];
	}

	// pe coloane 
	for (int i = 0; i < 8; ++i)
	{
		// faza 1
		p1[0] = block[0][i] + block[7][i];
		p1[1] = block[1][i] + block[6][i];
		p1[2] = block[2][i] + block[5][i];
		p1[3] = block[3][i] + block[4][i];
		p1[4] = block[3][i] - block[4][i];
		p1[5] = block[2][i] - block[5][i];
		p1[6] = block[1][i] - block[6][i];
		p1[7] = block[0][i] - block[7][i];

		// faza 2
		p2[0] = p1[0] + p1[3];
		p2[1] = p1[1] + p1[2];
		p2[2] = p1[1] - p1[2];
		p2[3] = p1[0] - p1[3];
		p2[4] = -p1[4] - p1[5];
		p2[5] = p1[5] + p1[6];
		p2[6] = p1[6] + p1[7];
		p2[7] = p1[7];

		// faza 3
		p3[0] = p2[0] + p2[1];
		p3[1] = p2[0] - p2[1];
		p3[2] = (p2[2] + p2[3]) * c4;
		p3[3] = p2[3];
		aux = (p2[4] + p2[6]) * c6;
		p3[4] = -p2[4] * (c2 - c6) - aux;
		p3[5] = p2[5] * c4;
		p3[6] = p2[6] * (c2 + c6) - aux;
		p3[7] = p2[7];

		// faza 4
		p4[0] = p3[0];
		p4[1] = p3[1];
		p4[2] = p3[2] + p3[3];
		p4[3] = p3[3] - p3[2];
		p4[4] = p3[4];
		p4[5] = p3[5] + p3[7];
		p4[6] = p3[6];
		p4[7] = p3[7] - p3[5];

		// faza 5
		p5[0] = p4[0] / 2 * sqrt2;
		p5[1] = p4[1] / 4 * c4;
		p5[2] = p4[2] / 4 * c2;
		p5[3] = p4[3] / 4 * c6;
		p5[4] = (p4[4] + p4[7]) / 4 * c5;
		p5[5] = (p4[5] + p4[6]) / 4 * c1;
		p5[6] = (p4[5] - p4[6]) / 4 * c7;
		p5[7] = (p4[7] - p4[4]) / 4 * c3;

		block[0][i] = p5[0];
		block[1][i] = p5[5];
		block[2][i] = p5[2];
		block[3][i] = p5[7];
		block[4][i] = p5[1];
		block[5][i] = p5[4];
		block[6][i] = p5[3];
		block[7][i] = p5[6];
	}
}

void makeQuantizationTables(int quality, BYTE luminanceTable[8][8], BYTE chromaTable[8][8])
{
	const BYTE standardLuminanceTable[8][8] = {
		{16,  11,  10,  16,  24,  40,  51,  61},
		{12,  12,  14,  19,  26,  58,  60,  55},
		{14,  13,  16,  24,  40,  57,  69,  56},
		{14,  17,  22,  29,  51,  87,  80,  62},
		{18,  22,  37,  56,  68, 109, 103,  77},
		{24,  35,  55,  64,  81, 104, 113,  92},
		{49,  64,  78,  87, 103, 121, 120, 101},
		{72,  92,  95,  98, 112, 100, 103,  99 } };

	const BYTE standardChromaTable[8][8] = {
		{17, 18, 24, 47, 99, 99, 99, 99},
		{18, 21, 26, 66, 99, 99, 99, 99},
		{24, 26, 56, 99, 99, 99, 99, 99},
		{47, 66, 99, 99, 99, 99, 99, 99},
		{99, 99, 99, 99, 99, 99, 99, 99},
		{99, 99, 99, 99, 99, 99, 99, 99},
		{99, 99, 99, 99, 99, 99, 99, 99},
		{99, 99, 99, 99, 99, 99, 99, 99}
	};

	unsigned long val, scaleFactor;

	scaleFactor = (quality == 0) ? 1 :
					(quality > 100) ? 100 : quality;
	scaleFactor = (scaleFactor < 50) ? (5000 / scaleFactor) : (200 - scaleFactor * 2);

	for (int i = 0; i < 8; ++i)
	{
		for (int j = 0; j < 8; ++j)
		{
			val = (scaleFactor * standardLuminanceTable[i][j] + 50l) / 100l;
			luminanceTable[i][j] = (val < 8) ? 8 :
				(val > 255) ? 255 :
				(BYTE)val;

			val = (scaleFactor * standardChromaTable[i][j] + 50l) / 100l;
			chromaTable[i][j] = (val < 8) ? 8 :
				(val > 255) ? 255 :
				(BYTE)val;
		}
	}
}

void ChenBlockFDCT_2D(float block[8][8]) {
	float p1[8], p2[8], p3[8];
	size_t i;
	// process rows first
	for (i = 0; i < 8; i++) {
		// FDCT 1D Phase 1
		p1[0] = block[i][0] + block[i][7];
		p1[1] = block[i][1] + block[i][6];
		p1[2] = block[i][2] + block[i][5];
		p1[3] = block[i][3] + block[i][4];
		p1[4] = block[i][3] - block[i][4];
		p1[5] = block[i][2] - block[i][5];
		p1[6] = block[i][1] - block[i][6];
		p1[7] = block[i][0] - block[i][7];
		// FDCT 1D Phase 2
		p2[0] = p1[0] + p1[3];
		p2[1] = p1[1] + p1[2];
		p2[2] = p1[1] - p1[2];
		p2[3] = p1[0] - p1[3];
		p2[4] = p1[4];
		p2[5] = (p1[6] - p1[5]) * c4; // c4=cos(pi/4)
		p2[6] = (p1[6] + p1[5]) * c4; // c4=cos(pi/4)
		p2[7] = p1[7];
		// FDCT 1D Phase 3
		p3[0] = (p2[0] + p2[1]) * c4; // c4=cos(pi/4)
		p3[1] = (p2[0] - p2[1]) * c4; // c4=cos(pi/4)
		p3[2] = p2[3] * c2 + p2[2] * c6; // c2=sin(2*pi/16), c6=cos(6*pi/16)
		p3[3] = p2[3] * c6 - p2[2] * c2; // c2=sin(2*pi/16), c6=cos(6*pi/16)
		p3[4] = p2[4] + p2[5];
		p3[5] = p2[4] - p2[5];
		p3[6] = p2[7] - p2[6];
		p3[7] = p2[7] + p2[6];
		// FDCT 1D Pase 4
		block[i][0] = p3[0] / 2;
		block[i][1] = (p3[7] * c1 + p3[4] * c7) / 2; // c1=cos(pi/16), c7=cos(7*pi/16)
		block[i][2] = p3[2] / 2;
		block[i][3] = (p3[6] * c3 - p3[5] * c5) / 2; // c3=cos(3*pi/16), c5=cos(5*pi/16)
		block[i][4] = p3[1] / 2;
		block[i][5] = (p3[5] * c3 + p3[6] * c5) / 2; // c3=cos(3*pi/16), c5=sin(5*pi/16)
		block[i][6] = p3[3] / 2;
		block[i][7] = (p3[7] * c7 - p3[4] * c1) / 2; // c1=cos(pi/16), c7=cos(7*pi/16)
	}
	// then process columns
	for (i = 0; i < 8; i++) {
		// FDCT 1D Phase 1
		p1[0] = block[0][i] + block[7][i];
		p1[1] = block[1][i] + block[6][i];
		p1[2] = block[2][i] + block[5][i];
		p1[3] = block[3][i] + block[4][i];
		p1[4] = block[3][i] - block[4][i];
		p1[5] = block[2][i] - block[5][i];
		p1[6] = block[1][i] - block[6][i];
		p1[7] = block[0][i] - block[7][i];
		// FDCT 1D Phase 2
		p2[0] = p1[0] + p1[3];
		p2[1] = p1[1] + p1[2];
		p2[2] = p1[1] - p1[2];
		p2[3] = p1[0] - p1[3];
		p2[4] = p1[4];
		p2[5] = (p1[6] - p1[5]) * c4; // c4=cos(pi/4)
		p2[6] = (p1[6] + p1[5]) * c4; // c4=cos(pi/4)
		p2[7] = p1[7];
		// FDCT 1D Phase 3
		p3[0] = (p2[0] + p2[1]) * c4; // c4=cos(pi/4)
		p3[1] = (p2[0] - p2[1]) * c4; // c4=cos(pi/4)
		p3[2] = p2[3] * c2 + p2[2] * c6; // c2=sin(2*pi/16), c6=cos(6*pi/16)
		p3[3] = p2[3] * c6 - p2[2] * c2; // c2=sin(2*pi/16), c6=cos(6*pi/16)
		p3[4] = p2[4] + p2[5];
		p3[5] = p2[4] - p2[5];
		p3[6] = p2[7] - p2[6];
		p3[7] = p2[7] + p2[6];
		// FDCT 1D Pase 4
		block[0][i] = p3[0] / 2;
		block[1][i] = (p3[7] * c1 + p3[4] * c7) / 2; // c1=cos(pi/16), c7=cos(7*pi/16)
		block[2][i] = p3[2] / 2;
		block[3][i] = (p3[6] * c3 - p3[5] * c5) / 2; // c3=cos(3*pi/16), c5=cos(5*pi/16)
		block[4][i] = p3[1] / 2;
		block[5][i] = (p3[5] * c3 + p3[6] * c5) / 2; // c3=cos(3*pi/16), c5=sin(5*pi/16)
		block[6][i] = p3[3] / 2;
		block[7][i] = (p3[7] * c7 - p3[4] * c1) / 2; // c1=cos(pi/16), c7=cos(7*pi/16)
	}
}

