#include <stdio.h>
#include <windows.h>

#include "compression.h"
#include "decompression.h"
#include "utils.h"

void printHelp();

int wmain(int argc, wchar_t* argv[])
{
	if (argc >= 2 && wcslen(argv[1]) == 2 &&
		(argv[1][0] == L'-' || argv[1][0] == L'/' ) )
	{
		//ajutor
		if (argv[1][1] == L'h' || (argv[1][1] == L'?' && argv[1][0] == L'/'))
		{
			printHelp();
			return 0;
		}

		//compresie
		if (argv[1][1] == L'c' && argc == 5)
		{
			int quality = _wtoi(argv[2]);

			//verific daca quality are valori corecte
			if (quality < 1 || quality > 100)
			{
				wprintf(L"Error: quality should be an integer between 1 and 100.\n");
				return -2;
			}

			wprintf(L"Compressing with quality %d...\nSource: %s\nDestination: %s\n", quality, argv[3], argv[4]);

			//fisierul sursa
			HANDLE sFile;
			sFile = CreateFileW(
				argv[3],						// file name
				GENERIC_READ,					// open for reading
				0,								// do not share
				NULL,							// no security
				OPEN_EXISTING,					// existing file only
				FILE_ATTRIBUTE_NORMAL,			// normal file
				NULL);
			if (sFile == INVALID_HANDLE_VALUE)
			{
				wprintf(L"Error: failed opening the source file\n");
				return -3;
			}
			wprintf(L"Source file opened.\n");

			//fisierul destinatie
			HANDLE dFile;
			dFile = CreateFileW(
				argv[4],						// file name
				GENERIC_WRITE,					// open for reading
				0,								// do not share
				NULL,							// no security
				CREATE_ALWAYS,					// existing file only
				FILE_ATTRIBUTE_NORMAL,			// normal file
				NULL);
			if (dFile == INVALID_HANDLE_VALUE)
			{
				wprintf(L"Error: failed creating the destination file\n");
				return -4;
			}
			wprintf(L"Destination file created.\n");

			compressFile(sFile, dFile, quality);

			wprintf(L"The file was compressed.\n");

			CloseHandle(sFile);
			CloseHandle(dFile);

			return 0;
		}

		//decompresie
		if (argv[1][1] == L'd' && argc == 4)
		{
			wprintf(L"Decompressing...\nSource: %s\nDestination: %s\n", argv[2], argv[3]);

			//fisierul sursa
			HANDLE sFile;
			sFile = CreateFileW(
				argv[2],						// file name
				GENERIC_READ,					// open for reading
				0,								// do not share
				NULL,							// no security
				OPEN_EXISTING,					// existing file only
				FILE_ATTRIBUTE_NORMAL,			// normal file
				NULL);
			if (sFile == INVALID_HANDLE_VALUE)
			{
				wprintf(L"Error: failed opening the source file\n");
				return -3;
			}

			wprintf(L"Source file opened.\n");

			//fisierul destinatie
			HANDLE dFile;
			dFile = CreateFileW(
				argv[3],						// file name
				GENERIC_WRITE,					// open for reading
				0,								// do not share
				NULL,							// no security
				CREATE_ALWAYS,					// existing file only
				FILE_ATTRIBUTE_NORMAL,			// normal file
				NULL);
			if (dFile == INVALID_HANDLE_VALUE)
			{
				wprintf(L"Error: failed creating the destination file\n");
				return -4;
			}
			wprintf(L"Destination file created.\n");

			decompressFile(sFile, dFile);

			wprintf(L"The file was decompressed.\n");

			CloseHandle(sFile);
			CloseHandle(dFile);

			return 0;
		}
	}

	wprintf(L"Error: arguments do not exist or they are invalid.\n");
	wprintf(L"Try running the program using - h or /h or /? \n");

	return -1;
}

void printHelp()
{
	wprintf(L"-c or /c [quality] [input filepath] [output filepath]\n");
	wprintf(L"\tCompresses a CIF RGB24 file using a specified [quality] with a value between 1 and 100, from specified [input filepath] to [output filepath]\n");
	wprintf(L"-u or /u [input filepath] [output filepath]\n");
	wprintf(L"\tUncompresses a comressed file specified by [input filepath] into [output filepath]\n");
}