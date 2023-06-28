#include <stdio.h>
#include <windows.h>

#include "decompression.h"
#include "utils.h"

int decompressFile(HANDLE sFile, HANDLE dFile)
{
	unsigned long long nFrames;
	//LARGE_INTEGER FileSize;
	//GetFileSizeEx(sFile, &FileSize);
	//nFrames = FileSize.QuadPart / FRAME_SIZE;

	//wprintf(L"%llu frames found in source (compressed) file\n", nFrames);

	BOOL bSuccess;

	char sign[4];
	UINT16 width, height;
	UINT32 frames;
	BYTE quality;

	unsigned int headerOffset = 0;
	bSuccess = ReadBuffer(sFile, sign, 3 * sizeof(char), headerOffset);
	if (!bSuccess) {
		wprintf(L"Error: failed to read source file header (signature)\n");
		return -7;
	}
	headerOffset += 3 * sizeof(char);
	sign[3] = '\0';

	bSuccess = ReadBuffer(sFile, &width, sizeof(UINT16), headerOffset);
	if (!bSuccess) {
		wprintf(L"Error: failed to read source file header (width)\n");
		return -8;
	}
	headerOffset += sizeof(UINT16);

	bSuccess = ReadBuffer(sFile, &height, sizeof(UINT16), headerOffset);
	if (!bSuccess) {
		wprintf(L"Error: failed to read source file header (height)\n");
		return -9;
	}
	headerOffset += sizeof(UINT16);

	bSuccess = ReadBuffer(sFile, &frames, sizeof(UINT32), headerOffset);
	if (!bSuccess) {
		wprintf(L"Error: failed to read source file header (frames)\n");
		return -10;
	}
	headerOffset += sizeof(UINT32);
	nFrames = frames;

	bSuccess = ReadBuffer(sFile, &quality, sizeof(BYTE), headerOffset);
	if (!bSuccess) {
		wprintf(L"Error: failed to read source file header (height)\n");
		return -11;
	}
	headerOffset += sizeof(BYTE);

	wprintf(L"Source file info:\n");
	wprintf(L"- signature: %S\n", sign);
	wprintf(L"- width: %hu\n", width);
	wprintf(L"- height: %hu\n", height);
	wprintf(L"- frames: %u\n", frames);
	wprintf(L"- quality: %hhu\n", quality);

	if (strcmp(sign, "SMP") != 0 ||
		width != CIF_X ||
		height != CIF_Y)
	{
		wprintf(L"Error: file data does not match\n");
		return -12;
	}

	BYTE current_frame[CIF_Y][CIF_X][3];
	BYTE restore_frame[CIF_Y][CIF_X][3];

	for (int i = 0; i < nFrames; ++i)
	{
		BOOL b;
		b = ReadFrame(sFile, i, current_frame, headerOffset);

		// IDCT
		FrameIDCT(current_frame);

		// DPCM invers
		FrameReverseDPCM(current_frame, restore_frame, i);

		// YCbCr
		FrameYUV_to_RGB(current_frame);

		b = WriteFrame(dFile, i, current_frame);
	}

	return 0;
}

void FrameYUV_to_RGB(BYTE frame[CIF_Y][CIF_X][3])
{
	int r, g, b, y, u, v;

	for (int j = 0; j < CIF_X; ++j)
		for (int i = 0; i < CIF_Y; ++i)
		{
			y = frame[i][j][0];
			u = frame[i][j][1]; // cb
			v = frame[i][j][2]; // cr

			r = y + ((91881 * v) >> 16) - 179;
			g = y - ((22544 * u + 46793 * v) >> 16) + 135;
			b = y + ((116129 * u) >> 16) - 226;

			frame[i][j][0] = (BYTE)b;
			frame[i][j][1] = (BYTE)g;
			frame[i][j][2] = (BYTE)r;
		}
}

void FrameReverseDPCM(BYTE current_frame[CIF_Y][CIF_X][3], BYTE restore_frame[CIF_Y][CIF_X][3], int index)
{
	if (index % KEY_FRAME_OFFSET != 0) //daca nu e key frame
	{
		for (int j = 0; j < CIF_X; ++j)
			for (int i = 0; i < CIF_Y; ++i)
				for (int k = 0; k < 3; ++k)
				{
					restore_frame[i][j][k] = (short)restore_frame[i][j][k] + ((short)current_frame[i][j][k] - 128) * 2;
					current_frame[i][j][k] = restore_frame[i][j][k];
				}
	}
	else //daca e key frame
	{
		memcpy_s(restore_frame, FRAME_SIZE, current_frame, FRAME_SIZE);
	}
}

void FrameIDCT(BYTE frame[CIF_Y][CIF_X][3])
{
	unsigned short row, col, chn, i, j;
	float block[8][8];
	for (row = 0; row < CIF_X; row += 8)
	{
		for (col = 0; col < CIF_Y; col += 8)
		{
			for (chn = 0; chn < 3; ++chn)
			{
				for (j = 0; j < 8; j++)
					for (i = 0; i < 8; i++)
						block[j][i] = (float)(frame[col + j][row + i][chn]) - 128;

				//IDCT 2D pe block
				AraiBlockIDCT_2D(block);
				//ChenBlockIDCT_2D(block);

				for (j = 0; j < 8; j++)
					for (i = 0; i < 8; i++)
					{
						float val = block[j][i] + 128;
						if (val < 0) val = 0;
						if (val > 255) val = 255;
						frame[col + j][row + i][chn] = val;
					}
			}
		}
	}
}

void AraiBlockIDCT_2D(float block[8][8])
{
	float p1[8], p2[8], p3[8], aux, p4[8], p5[8];
	// algoritmul lui Arai
	// pe linii
	for (int i = 0; i < 8; ++i)
	{
		// faza 1;
		p1[0] = block[i][0] / 2 * sqrt2;
		p1[1] = block[i][4] / 4 * c4;
		p1[2] = block[i][2] / 4 * c2;
		p1[3] = block[i][6] / 4 * c6;
		p1[4] = block[i][5] / 4 * c5;
		p1[5] = block[i][1] / 4 * c1;
		p1[6] = block[i][7] / 4 * c7;
		p1[7] = block[i][3] / 4 * c3;

		// faza 2
		p2[0] = p1[0];
		p2[1] = p1[1];
		p2[2] = p1[2];
		p2[3] = p1[3];
		p2[4] = p1[4] - p1[7];
		p2[5] = p1[5] + p1[6];
		p2[6] = p1[5] - p1[6];
		p2[7] = p1[4] + p1[7];

		// faza 3
		p3[0] = p2[0];
		p3[1] = p2[1];
		p3[2] = p2[2] - p2[3];
		p3[3] = p2[3] + p2[2];
		p3[4] = p2[4];
		p3[5] = p2[5] - p2[7];
		p3[6] = p2[6];
		p3[7] = p2[7] + p2[5];

		// faza 4
		p4[0] = p3[0] + p3[1];
		p4[1] = p3[0] - p3[1];
		p4[2] = p3[2] * c4;
		p4[3] = p3[3] + p3[2] * c4;
		aux = (-p3[4] - p3[6]) * c6;
		p4[4] = -p3[4] * (c2 - c6) + aux;
		p4[5] = p3[5] * c4;
		p4[6] = p3[6] * (c2 + c6) + aux;
		p4[7] = p3[7];

		// faza 5
		p5[0] = p4[0] + p4[3];
		p5[1] = p4[1] + p4[2];
		p5[2] = p4[1] - p4[2];
		p5[3] = p4[0] - p4[3];
		p5[4] = -p4[4];
		p5[5] = p4[5] - p4[4];
		p5[6] = p4[5] + p4[6];
		p5[7] = p5[6] + p5[6];

		// faza 6
		block[i][0] = p5[0] + p5[7];
		block[i][1] = p5[1] + p5[6];
		block[i][2] = p5[2] + p5[5];
		block[i][3] = p5[3] + p5[4];
		block[i][4] = p5[3] - p5[4];
		block[i][5] = p5[2] - p5[5];
		block[i][6] = p5[1] - p5[6];
		block[i][7] = p5[0] - p5[7];
	}

	// pe coloane
	for (int i = 0; i < 8; ++i)
	{
		// faza 1;
		p1[0] = block[0][i] / 2 * sqrt2;
		p1[1] = block[4][i] / 4 * c4;
		p1[2] = block[2][i] / 4 * c2;
		p1[3] = block[6][i] / 4 * c6;
		p1[4] = block[5][i] / 4 * c5;
		p1[5] = block[1][i] / 4 * c1;
		p1[6] = block[7][i] / 4 * c7;
		p1[7] = block[3][i] / 4 * c3;


		// faza 2
		p2[0] = p1[0];
		p2[1] = p1[1];
		p2[2] = p1[2];
		p2[3] = p1[3];
		p2[4] = p1[4] - p1[7];
		p2[5] = p1[5] + p1[6];
		p2[6] = p1[5] - p1[6];
		p2[7] = p1[4] + p1[7];

		// faza 3
		p3[0] = p2[0];
		p3[1] = p2[1];
		p3[2] = p2[2] - p2[3];
		p3[3] = p2[3] + p2[2];
		p3[4] = p2[4];
		p3[5] = p2[5] - p2[7];
		p3[6] = p2[6];
		p3[7] = p2[7] + p2[5];

		// faza 4
		p4[0] = p3[0] + p3[1];
		p4[1] = p3[0] - p3[1];
		p4[2] = p3[2] * c4;
		p4[3] = p3[3] + p3[2] * c4;
		aux = (-p3[4] - p3[6]) * c6;
		p4[4] = -p3[4] * (c2 - c6) + aux;
		p4[5] = p3[5] * c4;
		p4[6] = p3[6] * (c2 + c6) + aux;
		p4[7] = p3[7];

		// faza 5
		p5[0] = p4[0] + p4[3];
		p5[1] = p4[1] + p4[2];
		p5[2] = p4[1] - p4[2];
		p5[3] = p4[0] - p4[3];
		p5[4] = -p4[4];
		p5[5] = p4[5] - p4[4];
		p5[6] = p4[5] + p4[6];
		p5[7] = p5[6] + p5[6];

		// faza 6
		block[0][i] = p5[0] + p5[7];
		block[1][i] = p5[1] + p5[6];
		block[2][i] = p5[2] + p5[5];
		block[3][i] = p5[3] + p5[4];
		block[4][i] = p5[3] - p5[4];
		block[5][i] = p5[2] - p5[5];
		block[6][i] = p5[1] - p5[6];
		block[7][i] = p5[0] - p5[7];
	}
}

void ChenBlockIDCT_2D(float block[8][8]) {
	float p1[8], p2[8], p3[8];
	size_t i;
	// process columns first
	for (i = 0; i < 8; i++) {
		// IDCT 1D Phase 1
		p1[0] = block[0][i];
		p1[1] = block[4][i];
		p1[2] = block[2][i];
		p1[3] = block[6][i];
		p1[4] = (block[1][i] * c7 - block[7][i] * c1); // c1=cos(pi/16), c7=cos(7*pi/16)
		p1[5] = (block[5][i] * c3 - block[3][i] * c5); // c3=cos(3*pi/16), c5=sin(5*pi/16)
		p1[6] = (block[3][i] * c3 + block[5][i] * c5); // c3=cos(3*pi/16), c5=sin(5*pi/16)
		p1[7] = (block[7][i] * c7 + block[1][i] * c1); // c1=cos(pi/16), c7=cos(7*pi/16)
		// IDCT 1D Phase 2
		p2[0] = (p1[0] + p1[1]) * c4; // c4=cos(pi/4)
		p2[1] = (p1[0] - p1[1]) * c4; // c4=cos(pi/4)
		p2[2] = p1[2] * c6 - p1[3] * c2; // c2=sin(2*pi/16), c6=cos(6*pi/16)
		p2[3] = p1[2] * c2 + p1[3] * c6;
		p2[4] = p1[4] + p1[5];
		p2[5] = p1[4] - p1[5];
		p2[6] = p1[7] - p1[6];
		p2[7] = p1[7] + p1[6];
		// IDCT 1D Phase 3
		p3[0] = p2[0] + p2[3];
		p3[1] = p2[1] + p2[2];
		p3[2] = p2[1] - p2[2];
		p3[3] = p2[0] - p2[3];
		p3[4] = p2[4];
		p3[5] = (p2[6] - p2[5]) * c4; // c4=cos(pi/4)
		p3[6] = (p2[6] + p2[5]) * c4; // c4=cos(pi/4)
		p3[7] = p2[7];
		// IDCT 1D Phase 4
		block[0][i] = (p3[0] + p3[7]) / 2;
		block[1][i] = (p3[1] + p3[6]) / 2;
		block[2][i] = (p3[2] + p3[5]) / 2;
		block[3][i] = (p3[3] + p3[4]) / 2;
		block[4][i] = (p3[3] - p3[4]) / 2;
		block[5][i] = (p3[2] - p3[5]) / 2;
		block[6][i] = (p3[1] - p3[6]) / 2;
		block[7][i] = (p3[0] - p3[7]) / 2;
	}
	// then process rows
	for (i = 0; i < 8; i++) {
		// IDCT 1D Phase 1
		p1[0] = block[i][0];
		p1[1] = block[i][4];
		p1[2] = block[i][2];
		p1[3] = block[i][6];
		p1[4] = (block[i][1] * c7 - block[i][7] * c1); // c1=cos(pi/16), c7=cos(7*pi/16)
		p1[5] = (block[i][5] * c3 - block[i][3] * c5); // c3=cos(3*pi/16), c5=sin(5*pi/16)
		p1[6] = (block[i][3] * c3 + block[i][5] * c5); // c3=cos(3*pi/16), c5=sin(5*pi/16)
		p1[7] = (block[i][7] * c7 + block[i][1] * c1); // c1=cos(pi/16), c7=cos(7*pi/16)
		// IDCT 1D Phase 2
		p2[0] = (p1[0] + p1[1]) * c4; // c4=cos(pi/4)
		p2[1] = (p1[0] - p1[1]) * c4; // c4=cos(pi/4)
		p2[2] = p1[2] * c6 - p1[3] * c2; // c2=sin(2*pi/16), c6=cos(6*pi/16)
		p2[3] = p1[2] * c2 + p1[3] * c6;
		p2[4] = p1[4] + p1[5];
		p2[5] = p1[4] - p1[5];
		p2[6] = p1[7] - p1[6];
		p2[7] = p1[7] + p1[6];
		// IDCT 1D Phase 3
		p3[0] = p2[0] + p2[3];
		p3[1] = p2[1] + p2[2];
		p3[2] = p2[1] - p2[2];
		p3[3] = p2[0] - p2[3];
		p3[4] = p2[4];
		p3[5] = (p2[6] - p2[5]) * c4; // c4=cos(pi/4)
		p3[6] = (p2[6] + p2[5]) * c4; // c4=cos(pi/4)
		p3[7] = p2[7];
		// IDCT 1D Phase 4
		block[i][0] = (p3[0] + p3[7]) / 2;
		block[i][1] = (p3[1] + p3[6]) / 2;
		block[i][2] = (p3[2] + p3[5]) / 2;
		block[i][3] = (p3[3] + p3[4]) / 2;
		block[i][4] = (p3[3] - p3[4]) / 2;
		block[i][5] = (p3[2] - p3[5]) / 2;
		block[i][6] = (p3[1] - p3[6]) / 2;
		block[i][7] = (p3[0] - p3[7]) / 2;
	}
}