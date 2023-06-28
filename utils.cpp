#include <stdio.h>
#include <windows.h>

#include "utils.h"

BOOL ReadBuffer(HANDLE hFile, void* buffer, unsigned int bufferSize, unsigned int offset)
{
	DWORD dwRead;

	LARGE_INTEGER filePos;
	filePos.QuadPart = offset;

	BOOL bSuccess = SetFilePointerEx(
		hFile,						// file handle
		filePos,					// number of bytes to move the file pointer
		NULL,						// new File Pointer
		FILE_BEGIN);				// starting point for the file pointer move

	if (!bSuccess)
		return FALSE;

	bSuccess = ReadFile(
		hFile,						// file handle
		buffer,						// the buffer for reading data
		bufferSize,					// number of bytes to be read
		&dwRead,					// number of bytes actually readed
		NULL);						// NO OVERLAPPED (asynchronous) file read

	if (!bSuccess || (dwRead != bufferSize)) {
		return FALSE;
	}

	return TRUE;
}

BOOL WriteBuffer(HANDLE hFile, void* buffer, unsigned int bufferSize, unsigned int offset)
{
	DWORD dwRead;

	LARGE_INTEGER filePos;
	filePos.QuadPart = offset;

	BOOL bSuccess = SetFilePointerEx(
		hFile,						// file handle
		filePos,					// number of bytes to move the file pointer
		NULL,						// new File Pointer
		FILE_BEGIN);				// starting point for the file pointer move

	if (!bSuccess)
		return FALSE;

	bSuccess = WriteFile(
		hFile,						// file handle
		buffer,						// the buffer for reading data
		bufferSize,					// number of bytes to be read
		&dwRead,					// number of bytes actually readed
		NULL);						// NO OVERLAPPED (asynchronous) file read

	if (!bSuccess || (dwRead != bufferSize)) {
		return FALSE;
	}

	return TRUE;
}

BOOL ReadFrame(HANDLE hFile, long long nFrameNr, BYTE buffer[CIF_Y][CIF_X][3], long long offset)
{
	DWORD dwRead;

	if (nFrameNr < 0)
		return FALSE;

	LARGE_INTEGER filePos;
	filePos.QuadPart = nFrameNr * FRAME_SIZE + offset;

	BOOL bSuccess = SetFilePointerEx(
		hFile,						// file handle
		filePos,					// number of bytes to move the file pointer
		NULL,						// new File Pointer
		FILE_BEGIN);				// starting point for the file pointer move

	if (!bSuccess)
		return FALSE;

	bSuccess = ReadFile(
		hFile,						// file handle
		buffer,						// the buffer for reading data
		FRAME_SIZE,					// number of bytes to be read
		&dwRead,					// number of bytes actually readed
		NULL);						// NO OVERLAPPED (asynchronous) file read

	if (!bSuccess || (dwRead != FRAME_SIZE)) {
		return FALSE;
	}

	return TRUE;
}

BOOL WriteFrame(HANDLE hFile, long long nFrameNr, BYTE buffer[CIF_Y][CIF_X][3], long long offset)
{
	DWORD dwRead;

	if (nFrameNr < 0)
		return FALSE;

	LARGE_INTEGER filePos;
	filePos.QuadPart = nFrameNr * FRAME_SIZE + offset;

	BOOL bSuccess = SetFilePointerEx(
		hFile,						// file handle
		filePos,					// number of bytes to move the file pointer
		NULL,						// new File Pointer
		FILE_BEGIN);				// starting point for the file pointer move

	if (!bSuccess)
		return FALSE;

	bSuccess = WriteFile(
		hFile,						// file handle
		buffer,						// the buffer for reading data
		FRAME_SIZE,					// number of bytes to be read
		&dwRead,					// number of bytes actually readed
		NULL);						// NO OVERLAPPED (asynchronous) file read

	if (!bSuccess || (dwRead != FRAME_SIZE)) {
		return FALSE;
	}

	return TRUE;
}