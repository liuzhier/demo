﻿#include <windows.h>
#include <stdio.h>

#include "pallib.h"
#include <direct.h>

//
// 敌方战斗图像导出目录
//
#define outPathOfABC					"ABC\\"

//
// 道具图像导出目录
//
#define outPathOfBALL				"BALL\\"

//
// 我方战斗图像导出目录
//
#define outPathOfFIGHT				"FIGHT\\"

//
// 卷动/背景图像导出目录
//
#define outPathOfFBP					"FBP\\"

//
// 仙术特效图像导出目录
//
#define outPathOfFIRE				"FIRE\\"

//
// 地图图像导出目录
//
#define outPathOfMAP					"MAP\\"

//
// 地图角色移动/动作图像导出目录
//
#define outPathOfMGO					"MGO\\"

//
// 事件图像导出目录
//
#define outPathOfEVENT				"EVENT\\"

//
// 角色肖像导出目录
//
#define outPathOfRGM					"RGM\\"

//
// 过场动漫静态图像导出目录
//
#define outPathOfRNG					"RNG\\"

//
// 敌方战斗图像背景色索引
//
#define bgColorIndexOfABC			200

//
// 道具图像背景色索引
//
#define bgColorIndexOfBALL			160

//
// 我方战斗图像背景色索引
//
#define bgColorIndexOfFIGHT			200

//
// 卷动/背景图像背景色索引
//
#define bgColorIndexOfFBP			200

//
// 仙术特效图像背景色索引（默认自动，暂不提供）
//
#define bgColorIndexOfFIRE			0

//
// 地图图像背景色索引（默认自动，暂不提供）
//
#define bgColorIndexOfMAP			0

//
//  地图角色移动/动作图像背景色索引
//
#define bgColorIndexOfMGO			200

//
// 事件图像背景色索引
//
#define bgColorIndexOfEVENT			200

//
// 角色肖像背景色索引
//
#define bgColorIndexOfRGM			160

//
// 过场动漫静态图像背景色索引（默认自动，暂不提供）
//
#define bgColorIndexOfRNG			0

CHAR szInputPath[0xFF];
CHAR szOutputPath[0xFF];
CHAR szOutputPathBuk[0xFF];

CHAR szOutputFile[0xFF];
CHAR szOutputFileBuk[0xFF];

// 资源文件路径
CHAR szAbcJ[0xFF];
CHAR szBallNew[0xFF];
CHAR szFbpPx[0xFF];
CHAR szFffJ[0xFF];
CHAR szFirenAv[0xFF];
CHAR szMgoJ[0xFF];
CHAR szMgoNew[0xFF];
CHAR szMgopJoe[0xFF];
CHAR szRgmJoe[0xFF];
CHAR szRngAv[0xFF];
CHAR szRngPal[0xFF];
CHAR szAlldatNew[0xFF];

#pragma pack(2)
struct RGB555
{
	unsigned short	red : 5;
	unsigned short	green : 5;
	unsigned short	blue : 5;
};
#pragma pack()

VOID OutputBitmap(char* filename, RGB555 palette[], LPBYTE lpBuffer, DWORD dwWidth, DWORD dwHeight)
{
	LPBITMAPFILEHEADER	lpFileHeader;
	LPBITMAPINFOHEADER	lpInfoHeader;
	LPRGBQUAD	lpBmpPalette;
	LPBYTE	lpBmpContent;
	DWORD	dwContentLength = dwWidth * dwHeight;

	HANDLE hFile = CreateFile(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	HANDLE hMapping = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, 1078 + dwContentLength, NULL);
	lpFileHeader = (LPBITMAPFILEHEADER)MapViewOfFile(hMapping, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
	lpInfoHeader = (LPBITMAPINFOHEADER)(lpFileHeader + 1);
	lpBmpPalette = (LPRGBQUAD)(lpInfoHeader + 1);
	lpBmpContent = (LPBYTE)(lpBmpPalette + 256);

	lpFileHeader->bfType = 'MB';
	lpFileHeader->bfSize = 1078 + dwContentLength;
	lpFileHeader->bfReserved1 = lpFileHeader->bfReserved2 = 0;
	lpFileHeader->bfOffBits = 1078;

	lpInfoHeader->biSize = sizeof(BITMAPINFOHEADER);
	lpInfoHeader->biWidth = dwWidth;
	lpInfoHeader->biHeight = dwHeight;
	lpInfoHeader->biPlanes = 1;
	lpInfoHeader->biBitCount = 8;
	lpInfoHeader->biCompression = BI_RGB;
	lpInfoHeader->biSizeImage = dwContentLength;
	lpInfoHeader->biXPelsPerMeter = lpInfoHeader->biYPelsPerMeter = 0;
	lpInfoHeader->biClrUsed = lpInfoHeader->biClrImportant = 0;

	for (int i = 0; i < 256; i++)
	{
		lpBmpPalette[i].rgbRed = palette[i].red << 3;
		lpBmpPalette[i].rgbGreen = palette[i].green << 3;
		lpBmpPalette[i].rgbBlue = palette[i].blue << 3;
		lpBmpPalette[i].rgbReserved = 0;
	}

	lpBmpContent += dwContentLength;
	for (int i = 0; i < dwHeight; i++)
	{
		lpBmpContent -= dwWidth;
		CopyMemory(lpBmpContent, lpBuffer, dwWidth);
		lpBuffer += dwWidth;
	}

	UnmapViewOfFile(lpFileHeader);
	CloseHandle(hMapping);
	CloseHandle(hFile);
}

/*
RNG.AV 结构：以 0x800 (2048) 字节（即一个光盘扇区大小）为一个完整的数据块。其中数据均为 Bit-Endian
第 1 块：RNG 各段动画的起始位置索引，以数据块为单位，每个索引 2 字节，最后一个索引为文件的总长度，因此共 44 个索引。

第 2 块：第一段动画的信息块
		第 1 个 DWORD，表示该段动画的帧数
		接下来 N 个 DWORD，表示动画每一帧相对于信息块首地址的偏移量。其中最高的一位 bit 表示该帧是全编码(1)还是部分编码(0)。
		最后 1 个 DWORD，表示该段动画帧的结束位置。
……

0x10: 跳至下一行

0x2W: 跳过 0xW + 1 个像素
0x3W 0xYZ: 跳过 0xWYZ 个像素
0x4W: 写入 0xW 个 0xFF （似乎应为透明色更准确）
0x5W 0xYZ: 写入 0xWYZ 个 0xFF （似乎应为透明色更准确）
0x6W: 写入接下来的 0xW 个像素
0x7W 0xYZ: 写入接下来的 0xWYZ 个像素
0x8W: 重复接下来的一个像素 0xW 次
*0x9W 0xYZ: 重复接下来的一个像素 0xWYZ 次

RNG.PAL 结构：共 44 个调色板，每个调色板共 256 项，每项 2 字节（RGB555）
*/

void decode_rng(unsigned char* buf, unsigned char* buffer)
{
	int x = 0, y = 0;
	int code;

	for (;;)
	{
		code = *buf++;
		if (code == 0x00)
			break;
		else if (code == 0x10)
			buffer += 320, x = 0, y++;
		else if (code >= 0x20 && code <= 0x2f)
			x += code & 0xf;
		else if (code >= 0x30 && code <= 0x3f)
			x += ((code & 0xf) << 8) | *buf++;
		else if (code >= 0x41 && code <= 0x4f)
		{
			code &= 0xf;
			for (int i = 0; i < code; i++)
				buffer[x++] = 0xff;
		}
		else if (code >= 0x50 && code <= 0x5f)
		{
			code = ((code & 0xf) << 8) | *buf++;
			for (int i = 0; i < code; i++)
				buffer[x++] = 0xff;
		}
		else if (code >= 0x61 && code <= 0x6f)
		{
			code &= 0xf;
			for (int i = 0; i < code; i++)
				buffer[x++] = *buf++;
		}
		else if (code >= 0x70 && code <= 0x7f)
		{
			code = ((code & 0xf) << 8) | *buf++;
			for (int i = 0; i < code; i++)
				buffer[x++] = *buf++;
		}
		else if (code >= 0x81 && code <= 0x8f)
		{
			unsigned char ch = *buf++;
			code &= 0xf;
			for (int i = 0; i < code; i++)
				buffer[x++] = ch;
		}
		else if (code >= 0x90 && code <= 0x9f)
		{
			code = ((code & 0xf) << 8) | *buf++;
			unsigned char ch = *buf++;
			for (int i = 0; i < code; i++)
				buffer[x++] = ch;
		}
		else
		{
			printf("unknown code: %d\n", code);
			int offset = y * 320 + x;
			continue;
		}
	}
	//	printf("y = %d\t", y);
}

void read_rng()
{
	FILE* fp = fopen(szRngAv, "rb");
	FILE* fp0 = fopen(szRngPal, "rb");
	int seq = 0;
	unsigned long num;
	RGB555 palette[0x100];
	unsigned short* ppat;
	unsigned short indices[0x200];
	unsigned long indoff[0x200];
	unsigned long offsets[0x200];
	unsigned char buffer[0xfa00];
	fread(indices, 2, 45, fp);
	for (int i = 0; i < 45; i++)
		indoff[i] = (unsigned long)ntohs(indices[i]) * 0x800;

	for (seq = 0; seq < 44; seq++)
	{
		fseek(fp, indoff[seq], SEEK_SET);
		fread(&num, 4, 1, fp);
		num = ntohl(num);
		fread(offsets, 4, num, fp);

		fseek(fp0, seq * 0x200, SEEK_SET);
		fread(palette, 2, 0x100, fp0);
		ppat = (unsigned short*)palette;
		for (int i = 0; i < 256; i++)
			ppat[i] = ntohs(ppat[i]);

		for (int i = 0; i < num; i++)
			offsets[i] = ntohl(offsets[i]);

		sprintf(szOutputFile, "%s%s", szOutputPath, outPathOfRNG);

		// 创建目录 ./game/PalSSOut/RNG
		mkdir(szOutputFile);

		// 备份路径
		sprintf(szOutputFileBuk, "%s", szOutputFile);

		for (int i = 0; i < num - 1; i++)
		{
			if (offsets[i] >= 0x80000000)
			{
				offsets[i] &= 0x7fffffff;
				fseek(fp, indoff[seq] + offsets[i], SEEK_SET);
				fread(buffer, 0xfa00, 1, fp);
			}
			else
			{
				unsigned char* buf = (unsigned char*)malloc((offsets[i + 1] & 0x7fffffff) - offsets[i]);
				fseek(fp, indoff[seq] + offsets[i], SEEK_SET);
				fread(buf, (offsets[i + 1] & 0x7fffffff) - offsets[i], 1, fp);
				decode_rng(buf, buffer);
				free(buf);
			}

			// 还原路径....
			sprintf(szOutputFile, "%s", szOutputFileBuk);

			sprintf(szOutputFile, "%s\\RNG%d", szOutputFile, seq);

			// 创建目录 ./game/PalSSOut/RNG/RNG0
			mkdir(szOutputFile);

			sprintf(szOutputFile, "%s\\RNG%d", szOutputFile, seq);

			//
			// 分开命名，方便 enrle
			//
			sprintf(szOutputFile, "%s%d.bmp", szOutputFile, i);

			OutputBitmap(szOutputFile, palette, buffer, 320, 200);
		}
		printf("Processed RNG: %d\n", seq);
		//		getchar();
	}
	fclose(fp0);
	fclose(fp);
}

/*
FIREN.AV：按 0x800 分块
第 1 块为索引（以块为单位）

第 2 块：
  本块调色板（0x200字节，RGB555）
  块内索引
  内容（格式同RNG）

*/

void read_firen()
{
	FILE* fp = fopen(szFirenAv, "rb");
	int seq = 0;
	unsigned long num;
	RGB555 palette[0x100];
	unsigned short* ppat = (unsigned short*)palette;
	unsigned short indices[0x200];
	unsigned long indoff[0x200];
	unsigned long offsets[0x200];
	unsigned char buffer[0xfa00];
	fread(indices, 2, 112, fp);
	for (int i = 0; i < 112; i++)
		indoff[i] = (unsigned long)ntohs(indices[i]) * 0x800;

	sprintf(szOutputFile, "%s%s", szOutputPath, outPathOfFIRE);

	// 创建目录 ./game/PalSSOut/FIRE
	mkdir(szOutputFile);

	// 备份路径
	sprintf(szOutputFileBuk, "%s", szOutputFile);

	for (seq = 0; seq < 111; seq++)
	{
		fseek(fp, indoff[seq], SEEK_SET);

		fread(palette, 2, 0x100, fp);
		for (int i = 0; i < 256; i++)
			ppat[i] = ntohs(ppat[i]);

		fread(offsets, 4, 1, fp);
		num = ((ntohl(offsets[0]) & 0x0fffffff) >> 2) - 1;
		fread(offsets + 1, 4, num, fp);
		for (int i = 0; i <= num; i++)
			offsets[i] = ntohl(offsets[i]);

		for (int i = 0; i < num; i++)
		{
			if (offsets[i] >= 0x20000000) {}
			offsets[i] &= 0x0fffffff;
			unsigned char* buf = (unsigned char*)malloc((offsets[i + 1] & 0x0fffffff) - offsets[i]);
			fseek(fp, indoff[seq] + 0x200 + offsets[i], SEEK_SET);
			fread(buf, (offsets[i + 1] & 0x0fffffff) - offsets[i], 1, fp);
			decode_rng(buf, buffer);
			free(buf);

			// 还原路径....
			sprintf(szOutputFile, "%s", szOutputFileBuk);

			sprintf(szOutputFile, "%s\\FIRE%d", szOutputFile, seq);

			// 创建目录 ./game/PalSSOut/FIRE/FIRE0
			mkdir(szOutputFile);

			sprintf(szOutputFile, "%s\\FIRE%d", szOutputFile, seq);

			//
			// 分开命名，方便 enrle
			//
			sprintf(szOutputFile, "%s%d.bmp", szOutputFile, i);

			OutputBitmap(szOutputFile, palette, buffer, 320, 200);
		}
		printf("Processed FIREN: %d\n", seq);
		//		getchar();
	}
	fclose(fp);
}

/* ALLDAT.NEW: 前 0x1000 字节为 8 个调色板，其中 3、4 同，5、6 同；
				0为战时背景调色板；1为平时人物模型调色板；2为战时人物模型调色板；3、4为人物头像（状态背景）调色板
	RGM.JOE: 人物头像，RLE 格式，每 0x2800 字节为一幅
	FBP.PX: 背景，全图，每 0x10000 字节为一幅，其中前 0xFA00 有效
			0、1、3、4、5：状态背景，2(片头)->1
	BALL.NEW: 格式同 BALL.MKF，不同之处为索引是BE，且子文件不再有 4 字节头

*/

bool DecoderCallBack(int srcVal, uint8* pOutVal, void* pUserData)
{
	return false;
}

void read_rgm()
{
	FILE* fp = fopen(szRgmJoe, "rb");
	FILE* fp0 = fopen(szAlldatNew, "rb");
	int seq = -1, ikun, jkun;
	RGB555 palette[8][0x100];
	unsigned short* ppat = (unsigned short*)palette;
	unsigned char inbuf[0x2800];
	unsigned char buffer[0xfa00];
	unsigned short* pw = (unsigned short*)inbuf;
	unsigned short* ph = pw + 1;

	fread(palette, 2, 0x800, fp0);
	for (int i = 0; i < 0x8; i++)
		for (int j = 0; j < 256; j++)
			if (j == 0)
				ppat[(i << 8) + j] = ntohs(ppat[(i << 8) + bgColorIndexOfRGM]);
			else
				ppat[(i << 8) + j] = ntohs(ppat[(i << 8) + j]);

	sprintf(szOutputFile, "%s%s", szOutputPath, outPathOfRGM);

	// 创建目录 ./game/PalSSOut/RGM
	mkdir(szOutputFile);

	// 备份路径
	sprintf(szOutputFileBuk, "%s", szOutputFile);

	while (fread(inbuf, 1, 0x2800, fp))
	{
		seq++;
		if (*pw == 0 || *ph == 0)
			continue;
		*pw = ntohs(*pw); *ph = ntohs(*ph);
		memset(buffer, 0, 0xfa00);
		Pal::Tools::DecodeRle(inbuf, buffer, 320, 200, 0, 0, DecoderCallBack, NULL);

		// 还原路径....
		sprintf(szOutputFile, "%s", szOutputFileBuk);

		sprintf(szOutputFile, "%s\\RGM%d%d.bmp", szOutputFile, 0, seq);

		OutputBitmap(szOutputFile, palette[3], buffer, 320, 200);
	}
	fclose(fp0);
	fclose(fp);
}

void read_ball()
{
	FILE* fp = fopen(szBallNew, "rb");
	FILE* fp0 = fopen(szAlldatNew, "rb");
	RGB555 palette[8][0x100];
	unsigned short* ppat = (unsigned short*)palette;
	unsigned long offsets[253];

	// HACK: 宽高
	INT iRleWidth = 48;
	INT iRleHeight = 47;
	INT iRleLength = iRleWidth * iRleHeight;
	unsigned char inbuf[48 * 47];
	unsigned char buffer[48 * 47];

	unsigned short* pw = (unsigned short*)inbuf;
	unsigned short* ph = pw + 1;

	fread(palette, 2, 0x800, fp0);
	for (int i = 0; i < 0x8; i++)
		for (int j = 0; j < 256; j++)
			if (j == 0)
				ppat[(i << 8) + j] = ntohs(ppat[(i << 8) + bgColorIndexOfBALL]);
			else
				ppat[(i << 8) + j] = ntohs(ppat[(i << 8) + j]);

	sprintf(szOutputFile, "%s%s", szOutputPath, outPathOfBALL);

	// 创建目录 ./game/PalSSOut/BALL
	mkdir(szOutputFile);

	// 备份路径
	sprintf(szOutputFileBuk, "%s", szOutputFile);

	fread(offsets, 4, 253, fp);
	for (int i = 0; i < 253; i++)
		offsets[i] = ntohl(offsets[i]);

	for (int i = 0; i < 252; i++)
	{
		int len = offsets[i + 1] - offsets[i];
		if (len == 0)
			continue;
		fseek(fp, offsets[i], SEEK_SET);
		fread(inbuf, len, 1, fp);
		if (*pw == 0 || *ph == 0)
			continue;
		*pw = ntohs(*pw); *ph = ntohs(*ph);

		// HACK
		memset(buffer, 0, iRleLength);
		Pal::Tools::DecodeRle(inbuf, buffer, iRleWidth, iRleHeight, 0, 0, DecoderCallBack, NULL);

		// 还原路径....
		sprintf(szOutputFile, "%s", szOutputFileBuk);

		sprintf(szOutputFile, "%s\\%03d.bmp", szOutputFile, i);

		OutputBitmap(szOutputFile, palette[3], buffer, iRleWidth, iRleHeight);
	}
	fclose(fp0);
	fclose(fp);
}

void read_fbp()
{
	FILE* fp = fopen(szFbpPx, "rb");
	FILE* fp0 = fopen(szAlldatNew, "rb");
	int seq = -1;
	RGB555 palette[8][0x100];
	unsigned short* ppat = (unsigned short*)palette;
	unsigned char buffer[0x10000];

	fread(palette, 2, 0x800, fp0);
	for (int i = 0; i < 0x8; i++)
		for (int j = 0; j < 256; j++)
			if (j == 0)
				ppat[(i << 8) + j] = ntohs(ppat[(i << 8) + bgColorIndexOfFBP]);
			else
				ppat[(i << 8) + j] = ntohs(ppat[(i << 8) + j]);

	sprintf(szOutputFile, "%s%s", szOutputPath, outPathOfFBP);

	// 创建目录 ./game/PalSSOut/FBP
	mkdir(szOutputFile);

	// 备份路径
	sprintf(szOutputFileBuk, "%s", szOutputFile);

	while (fread(buffer, 0x10000, 1, fp))
	{
		seq++;

		// 还原路径....
		sprintf(szOutputFile, "%s", szOutputFileBuk);

		sprintf(szOutputFile, "%s\\%03d.bmp", szOutputFile, seq);
		if (seq == 0 || seq == 1 || (seq >= 3 && seq <= 5))
			OutputBitmap(szOutputFile, palette[3], buffer, 320, 200);
		else if (seq == 2)
			OutputBitmap(szOutputFile, palette[1], buffer, 320, 200);
		else
			OutputBitmap(szOutputFile, palette[0], buffer, 320, 200);
	}

	fclose(fp0);
	fclose(fp);
}

void read_j(int index)
{
	CHAR szOutputFile[255];
	CHAR szOutputFileBuk[255];

	char* filename[] = { szAbcJ, szFffJ, szMgoJ };
	FILE* fp = fopen(filename[index], "rb");
	FILE* fp0 = fopen(szAlldatNew, "rb");
	RGB555 palette[8][0x100];
	int max = 0, pi = (index == 2 ? 1 : 2);
	unsigned short* ppat = (unsigned short*)palette;
	unsigned short indices[0x400];
	unsigned long offset;
	unsigned long offsets[0x400];
	unsigned char* inbuf = NULL;
	unsigned char* buffer;

	unsigned short* num;
	struct __size {
		unsigned short w;
		unsigned short h;
	} *psize;

	fread(palette, 2, 0x800, fp0);
	for (int i = 0; i < 0x8; i++)
		for (int j = 0; j < 256; j++)
			if (j == 0)
				// 根据不同的图像类别分配透明色背景和输出路径
				switch (index)
				{
				case 0:
					ppat[(i << 8) + j] = ntohs(ppat[(i << 8) + bgColorIndexOfABC]);

					sprintf(szOutputFile, "%s%s", szOutputPath, outPathOfABC);
					break;

				case 1:
					ppat[(i << 8) + j] = ntohs(ppat[(i << 8) + bgColorIndexOfFIGHT]);

					sprintf(szOutputFile, "%s%s", szOutputPath, outPathOfFIGHT);
					break;

				case 2:
					ppat[(i << 8) + j] = ntohs(ppat[(i << 8) + bgColorIndexOfMGO]);

					sprintf(szOutputFile, "%s%s", szOutputPath, outPathOfMGO);
					break;
				}
			else
				ppat[(i << 8) + j] = ntohs(ppat[(i << 8) + j]);

	// 创建目录 ./game/PalSSOut/ABC    FIGHT    MGO
	mkdir(szOutputFile);

	// 备份路径
	sprintf(szOutputFileBuk, "%s", szOutputFile);

	fread(indices, 2, 0x400, fp);

	for (int i = 0; i < 0x400 && indices[i]; i++)
		offsets[i] = (unsigned long)ntohs(indices[i]) * 0x800, max = i;

	for (int i = 0; i < max; i++)
	{
		int len = offsets[i + 1] - offsets[i];
		if (len == 0)
			continue;
		inbuf = (unsigned char*)realloc(inbuf, len);
		num = (unsigned short*)inbuf;
		psize = (struct __size*)(inbuf + 2);
		fseek(fp, offsets[i], SEEK_SET);
		fread(inbuf, len, 1, fp);

		//
		// 图像横坐标偏移
		//
		*num = ntohs(*num); offset = 2 + 4 * *num;
		for (int j = 0; j < *num; j++)
		{
			psize[j].w = ntohs(psize[j].w);
			psize[j].h = ntohs(psize[j].h);

			// 申请空间并清理画布内存
			buffer = (unsigned char*)malloc(psize[j].w * psize[j].h);

			memset(buffer, 0x0, sizeof(buffer));

			for (int y = 0; y < psize[j].h * psize[j].w; y++)
				buffer[y] = inbuf[offset++];

			// 还原输出路径
			sprintf(szOutputFile, "%s", szOutputFileBuk);

			sprintf(szOutputFile, "%s\\%03d-%02d.bmp", szOutputFile, i, j);

			OutputBitmap(szOutputFile, palette[pi], (LPBYTE)buffer, psize[j].w, psize[j].h);

			// 释放内存
			free(buffer);
		}
	}

	free(inbuf);
	fclose(fp0);
	fclose(fp);
}

void read_mgo_new()
{
	FILE* fp = fopen(szMgoNew, "rb");
	FILE* fp0 = fopen(szAlldatNew, "rb");
	RGB555 palette[8][0x100];
	int sub_count[] = { 0, 11, 6, 8, 22, 17, 7, 2, 5, 2, 2, 1, 1, 2, 2, 1, 7, 4, 1, 15, 17, 10, 12, 15, 9, 12, 8, 7, 9, 3, 2, 3, 0, 0, 9, 9, 5, 5, 3, 1, 7, 7, 3, 1, 6, 4, 1, 13, 9, 20, 12, 8, 14, 13, 3, 2, 3, 0, 9, 10, 7, 3, 1, 2, 4, 8, 3, 8, 1, 2, 4, 5, 4, 3, 1, 3, 7, 7, 0, 4, 3, 12, 10, 1, 10, 1, 7, 1, 0, 6, 1, 11, 7, 21, 0, 1, 7, 4, 0, 4, 18, 8, 8, 3, 4, 0, 10, 1, 14, 8, 11, 8, 16, 3, 3, 4, 1, 4, 5, 10, 5, 3, 0, 0, 6, 1, 5, 8, 9, 12, 12, 3, 8, 2, 4, 4, 3, 2, 6, 5, 8, 3, 3, 5, 1, 3, 1, 8, 8, 6, 3, 4, 1, 7, 8, 2, 10, 1, 3, 3, 3, 2, 2, 2, 0, 4, 5, 5, 7, 3, 1, 1, 0, 7, 8, 6, 3, 0, 2, 5, 4, 2, 10, 4, 6, 5, 2, 3, 4, 6, 0, 8, 4, 4, 3, 2, 1, 5, 8, 5, 1, 10, 11, 6, 12, 6, 0, 2, 2, 3, 5, 3, 5, 2, 3, 2, 4, 4, 8, 14, 5, 0, 0, 0, 1, 1, 3, 2, 8, 2, 5, 3, 6, 5, 1, 0, 3, 6, 0, 1, 1, 2, 1, 9, 5, 2, 2, 5, 0, 4, 8, 2, 5, 3, 2, 5, 7, 1, 7, 6, 1, 1, 2, 4, 13, 11, 6, 5, 6, 5, 8, 0, 7, 0, 4, 0, 0, 2, 0, 9, 9, 7, 0, 8, 6, 0, 4, 0, 0, 0, 5, 5, 5, 6, 7, 2, 5, 6, 5 };
	int max = 0;//, pi = (index == 2 ? 1 : 2);
	unsigned short* ppat = (unsigned short*)palette;
	unsigned short indices[0x400];
	unsigned long offset, off;
	unsigned long offsets[0x400];
	unsigned char* inbuf = NULL;
	LPSTR buffer;
	unsigned short* num;
	struct __size {
		unsigned short w;
		unsigned short h;
	} *psize;

	fread(palette, 2, 0x800, fp0);
	for (int i = 0; i < 0x8; i++)
		for (int j = 0; j < 256; j++)
			if (j == 0)
				ppat[(i << 8) + j] = ntohs(ppat[(i << 8) + bgColorIndexOfEVENT]);
			else
				ppat[(i << 8) + j] = ntohs(ppat[(i << 8) + j]);

	fread(indices, 2, 0x400, fp);
	for (int i = 0; i < 0x400 && indices[i]; i++)
		offsets[i] = (unsigned long)ntohs(indices[i]) * 0x800, max = i;

	//	FILE* fp3 = fopen(outPathOf % "mgo--.txt", "w");
	for (int i = 0; i < max - 1; i++)
	{
		int len = offsets[i + 1] - offsets[i];
		int cnt = 0;
		if (len == 0)
			//		{
			//			fprintf(fp3, "0, ");
			continue;
		//		}
		inbuf = (unsigned char*)realloc(inbuf, len);
		num = (unsigned short*)inbuf;
		fseek(fp, offsets[i], SEEK_SET);
		fread(inbuf, len, 1, fp); offset = 0;
		for (int j = 0; j < sub_count[i]; j++, offset += num[offset] * 2 + 1)
		{
			num[offset] = ntohs(num[offset]);
			psize = (struct __size*)(num + offset + 1);
			for (int k = 0; k < num[offset]; k++)
			{
				psize[k].w = ntohs(psize[k].w);
				psize[k].h = ntohs(psize[k].h);
			}
		}
		offset <<= 1;

		// 创建目录 ./game/PalSSOut/EVENT
		sprintf(szOutputFile, "%s%s", szOutputPath, outPathOfEVENT);
		mkdir(szOutputFile);

		// 备份路径
		sprintf(szOutputFileBuk, "%s", szOutputFile);

		for (int j = 0, n = 0; n < sub_count[i]; j += num[j] * 2 + 1, n++)
		{
			psize = (struct __size*)(num + j + 1);
			for (int k = 0; k < num[j]; k++)
			{
				// 申请内存空间
				buffer = (LPSTR)malloc(psize[k].w * psize[k].h);

				memset(buffer, 0x0001, sizeof(buffer));

				for (int y = 0; y < psize[k].w * psize[k].h; y++)
					buffer[y] = inbuf[offset++];

				// 还原输出路径
				sprintf(szOutputFile, "%s", szOutputFileBuk);

				sprintf(szOutputFile, "%s\\%03d-%02d-%02d.bmp", szOutputFile, i, n, k);

				OutputBitmap(szOutputFile, palette[1], (LPBYTE)buffer, psize[k].w, psize[k].h);

				free(buffer);
			}
		}
	}
	free(inbuf);
	fclose(fp0);
	fclose(fp);
}

void read_mgop()
{
	FILE* fp = fopen(szMgopJoe, "rb");
	FILE* fp0 = fopen(szAlldatNew, "rb");
	RGB555 palette[8][0x100];
	int max = 0, cnt = 0;
	unsigned short* ppat = (unsigned short*)palette;
	unsigned short indices[0x200];
	unsigned short counts[0x200];
	unsigned long offset;
	unsigned long offsets[0x200];
	unsigned char* inbuf = NULL;
	unsigned char* buffer;
	struct _map
	{
		unsigned short base;
		unsigned short overlay;
	} *map;
	struct _gop
	{
		unsigned char pixel[0x0f][0x20];
	} *gop;

	fread(palette, 2, 0x800, fp0);
	for (int i = 0; i < 0x8; i++)
		for (int j = 0; j < 256; j++)
			ppat[(i << 8) + j] = ntohs(ppat[(i << 8) + j]);

	fread(indices, 2, 0x200, fp);
	for (int i = 0; i < 0x200 && indices[i]; i++)
		offsets[i] = (unsigned long)ntohs(indices[i]) * 0x800, max = i;
	fread(counts, 2, 0x200, fp);
	for (int i = 0; i <= max; i++)
		counts[i] = (unsigned long)ntohs(counts[i]);

	buffer = (unsigned char*)malloc(2064 * 2055);

	for (int i = 0; i < max - 1; i++)
	{
		int len = offsets[i + 1] - offsets[i];
		if (len == 0)
			continue;
		inbuf = (unsigned char*)realloc(inbuf, len);
		map = (_map*)inbuf;
		gop = (_gop*)(inbuf + 0x10000);
		fseek(fp, offsets[i], SEEK_SET);
		fread(inbuf, len, 1, fp); offset = 65536;
		for (int index = 0; index < 128 * 128; index++)
		{
			map[index].base = ntohs(map[index].base);
			map[index].overlay = ntohs(map[index].overlay);
		}

		sprintf(szOutputFile, "%s%s", szOutputPath, outPathOfMAP);
		sprintf(szOutputFileBuk, "%s", szOutputFile);

		// 创建目录    .\game\PalSSOut\MAP\ 
		mkdir(szOutputFile);

		memset(buffer, 0, 2064 * 2055);

		for (int y = 0; y < 128; y++)
			for (int x = 0, index = y << 7; x < 128; x++, index++)
			{
				unsigned short ibase = (map[index].base & 0x1ff);
				unsigned short ioverlay = (map[index].overlay & 0x1ff);
				int basey = (y << 4) + (x & 0x1 ? 8 : 0);
				for (int dy = basey, sy = 0; sy < 15; dy++, sy++)
					for (int dx = x << 4, sx = 0; sx < 32; dx++, sx++)
					{
						if (gop[ibase].pixel[sy][sx])
							buffer[dy * 2064 + dx] = gop[ibase].pixel[sy][sx];
						if (ioverlay && gop[ioverlay - 1].pixel[sy][sx])
							buffer[dy * 2064 + dx] = gop[ioverlay - 1].pixel[sy][sx];
					}
			}

		// 还原输出路径
		sprintf(szOutputFile, "%s", szOutputFileBuk);

		sprintf(szOutputFile, "%s\\%03d.bmp", szOutputFile, i);

		OutputBitmap(szOutputFile, palette[0], (LPBYTE)buffer, 2064, 2055);
	}
	free(inbuf);
	free(buffer);

	fclose(fp0);
	fclose(fp);
}

int main(int argc, char* argv[])
{
	// 获取游戏路径
	getcwd(szInputPath, 0xFF);

	sprintf(szOutputPath, "%s", szInputPath);

	// 获取图像输出目录工作路径
	sprintf(szOutputPath, "%s\\PalSSOut\\", szOutputPath);

	// 备份游戏路径
	sprintf(szOutputPathBuk, "%s", szOutputPath);
	mkdir(szOutputPath);

	sprintf(szAlldatNew, "%s\\ALLDAT.NEW", szInputPath);
	sprintf(szAbcJ, "%s\\abc.j", szInputPath);
	sprintf(szBallNew, "%s\\ball.new", szInputPath);
	sprintf(szFbpPx, "%s\\fbp.px", szInputPath);
	sprintf(szFffJ, "%s\\fff.j", szInputPath);
	sprintf(szFirenAv, "%s\\firen.av", szInputPath);
	sprintf(szMgoJ, "%s\\mgo.j", szInputPath);
	sprintf(szMgoNew, "%s\\mgo.new", szInputPath);
	sprintf(szMgopJoe, "%s\\mgop.joe", szInputPath);
	sprintf(szRgmJoe, "%s\\rgm.joe", szInputPath);
	sprintf(szRngAv, "%s\\rng.av", szInputPath);
	sprintf(szRngPal, "%s\\rng.pal", szInputPath);

	//read_rng1();
	read_rng();
	read_firen();
	read_rgm();
	read_ball();
	read_fbp();
	read_j(0);
	read_j(1);
	read_j(2);
	read_mgo_new();
	read_mgop();
	return 0;
}