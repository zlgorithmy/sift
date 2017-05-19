#include "header.h"

ClImage* clLoadImage(const char* path)
{
	ClImage* bmpImg;
	FILE* pFile;
	unsigned short fileType;
	ClBitMapFileHeader bmpFileHeader;
	ClBitMapInfoHeader bmpInfoHeader;
	int channels = 1;
	int width = 0;
	int height = 0;
	int step = 0;
	int offset = 0;
	unsigned char pixVal;
	ClRgbQuad* quad;
	int i, j, k;

	bmpImg = (ClImage*)malloc(sizeof(ClImage));
	pFile = fopen(path, "rb");
	if (!pFile)
	{
		fclose(pFile);
		free(bmpImg);
		return NULL;
	}

	fread(&fileType, sizeof(unsigned short), 1, pFile);
	if (fileType == 0x4D42)
	{
		fread(&bmpFileHeader, sizeof(ClBitMapFileHeader), 1, pFile);
		fread(&bmpInfoHeader, sizeof(ClBitMapInfoHeader), 1, pFile);
		if (bmpInfoHeader.biBitCount == 8)
		{
			channels = 1;
			width = bmpInfoHeader.biWidth;
			height = bmpInfoHeader.biHeight;
			offset = (channels*width) % 4;
			if (offset != 0)
			{
				offset = 4 - offset;
			}
			bmpImg->width = width;
			bmpImg->height = height;
			bmpImg->channels = 1;
			bmpImg->imageData = (unsigned char*)malloc(sizeof(unsigned char)*width*height);
			step = channels*width;

			quad = (ClRgbQuad*)malloc(sizeof(ClRgbQuad) * 256);
			fread(quad, sizeof(ClRgbQuad), 256, pFile);
			free(quad);

			for (i = 0; i<height; i++)
			{
				for (j = 0; j<width; j++)
				{
					fread(&pixVal, sizeof(unsigned char), 1, pFile);
					bmpImg->imageData[(height - 1 - i)*step + j] = pixVal;
				}
				if (offset != 0)
				{
					for (j = 0; j<offset; j++)
					{
						fread(&pixVal, sizeof(unsigned char), 1, pFile);
					}
				}
			}
		}
		else if (bmpInfoHeader.biBitCount == 24)
		{
			channels = 3;
			width = bmpInfoHeader.biWidth;
			height = bmpInfoHeader.biHeight;

			bmpImg->width = width;
			bmpImg->height = height;
			bmpImg->channels = 3;
			bmpImg->imageData = (unsigned char*)malloc(sizeof(unsigned char)*width * 3 * height);
			step = channels*width;

			offset = (channels*width) % 4;
			if (offset != 0)
			{
				offset = 4 - offset;
			}

			for (i = 0; i<height; i++)
			{
				for (j = 0; j<width; j++)
				{
					for (k = 0; k<3; k++)
					{
						fread(&pixVal, sizeof(unsigned char), 1, pFile);
						bmpImg->imageData[(height - 1 - i)*step + j * 3 + k] = pixVal;
					}
				}
				if (offset != 0)
				{
					for (j = 0; j<offset; j++)
					{
						fread(&pixVal, sizeof(unsigned char), 1, pFile);
					}
				}
			}
		}
	}

	fclose(pFile);
	pFile = NULL;
	return bmpImg;
}
void bmp2Iplimg(const ClImage* bmpImg, IplImage* iplImage)
{
	iplImage->width = bmpImg->width;
	iplImage->height = bmpImg->height;
	iplImage->nChannels = bmpImg->channels;
	memcpy(iplImage->imageData, bmpImg->imageData, iplImage->height*iplImage->widthStep);
}
IplImage* loadImage(const char* path)
{
	ClImage* bmp = clLoadImage(path);
	Size s0;
	s0.width = bmp->width;
	s0.height = bmp->height;
	IplImage* img = createImage(s0, 8, 3);
	bmp2Iplimg(bmp, img);
	return img;
}

int invert(Mat* src, Mat* dst, int n)
{
	int i, j, k;
	double max, temp;
	//double t[N][N];

	double **t;
	t = (double **)malloc(sizeof(double *) * n);//分配指针数组
	for (i = 0; i < n; i++)
	{
		t[i] = (double *)malloc(sizeof(double) * n);//分配每个指针所指向的数组
	}
	//临时矩阵  
	//将A矩阵存放在临时矩阵t[n][n]中  
	for (i = 0; i < n; i++)
	{
		//t[i] = (double*)(src->data.ptr + n * src->step);
		for (j = 0; j < n; j++)
		{
			t[i][j] = src->data.db[i*n + j];
		}
	}
	//初始化B矩阵为单位阵  
	for (i = 0; i < n; i++)
	{
		for (j = 0; j < n; j++)
		{
			dst->data.db[i*n + j] = (i == j) ? (float)1 : 0;
		}
	}
	for (i = 0; i < n; i++)
	{
		//寻找主元  
		max = t[i][i];
		k = i;
		for (j = i + 1; j < n; j++)
		{
			if (fabs(t[j][i]) > fabs(max))
			{
				max = t[j][i];
				k = j;
			}
		}
		//如果主元所在行不是第i行，进行行交换  
		if (k != i)
		{
			for (j = 0; j < n; j++)
			{
				temp = t[i][j];
				t[i][j] = t[k][j];
				t[k][j] = temp;
				//B伴随交换  
				temp = dst->data.db[i*n + j];
				dst->data.db[i*n + j] = dst->data.db[k*n + j];
				dst->data.db[k*n + j] = temp;
			}
		}
		//判断主元是否为0, 若是, 则矩阵A不是满秩矩阵,不存在逆矩阵  
		if (t[i][i] == 0)
		{
			//cout << "There is no inverse matrix!";
			for (i = 0; i < n; i++)
			{
				free(t[i]);
			}
			free(t);
			return 0;
		}
		//消去A的第i列除去i行以外的各行元素  
		temp = t[i][i];
		for (j = 0; j < n; j++)
		{
			t[i][j] = t[i][j] / temp;        //主对角线上的元素变为1  
			dst->data.db[i*n + j] = dst->data.db[i*n + j] / temp;        //伴随计算  
		}
		for (j = 0; j < n; j++)        //第0行->第n行  
		{
			if (j != i)                //不是第i行  
			{
				temp = t[j][i];
				for (k = 0; k < n; k++)        //第j行元素 - i行元素*j列i行元素  
				{
					t[j][k] = t[j][k] - t[i][k] * temp;
					dst->data.db[j*n + k] = dst->data.db[j*n + k] - dst->data.db[i*n + k] * temp;
				}
			}
		}
	}
	for (i = 0; i < n; i++)
	{
		free(t[i]);
	}
	free(t);
	return 1;
}
void imgSub(IplImage* src1, IplImage* src2, IplImage* dst0)
{
	float* src1Data = src1->imageData;	//数据的起始地址
	float* src2Data = src2->imageData;
	float* dst0Data = dst0->imageData;

	int width = src1->width;				//图像的行长度，即多少个字节
	int height = src1->height;
	int i, tmp;
	for (int row = 0; row < height; row++)
	{
		for (int col = 0; col < width; col++)
		{
			i = row*width + col;
			dst0Data[i] = src1Data[i] > src2Data[i] ? src1Data[i] - src2Data[i] : src2Data[i] - src1Data[i];
		}
	}
}
void resizeImg(IplImage* src, IplImage* dst)
{
	float* srcData = src->imageData;	//数据的起始地址
	float* dstData = dst->imageData;

	int srcStep = src->width;				//图像的行长度，即多少个字节
	int dstStep = dst->width;

	float fw = (float)src->width / dst->width;
	float fh = (float)src->height / dst->height;
	for (int row = 0; row < dst->height; row++)
	{
		for (int col = 0; col < dst->width; col++)
		{
			dstData[row*dstStep + col] = srcData[(int)(row*fh)*srcStep + (int)(col*fw )];
		}
	}
}
IplImage* rgb2gray(const IplImage* src)
{
	Size s;
	s.height = src->height;
	s.width = src->width;
	IplImage* dst = createImage(s, 8, 1);
	uchar* srcData = (uchar*)src->imageData;	//数据的起始地址
	uchar* dstData = (uchar*)dst->imageData;

	int srcStep = src->widthStep;				//图像的行长度，即多少个字节
	int dstStep = dst->widthStep;

	for (int row = 0; row < dst->height; row++)
	{
		for (int col = 0; col < dst->width; col++)
		{
			dstData[row*dstStep + col] = (int)(
				srcData[row*srcStep + col * 3 + 0] * 0.114 + 
				srcData[row*srcStep + col * 3 + 1] * 0.587 + 
				srcData[row*srcStep + col * 3 + 2] * 0.299);
		}
	}
	return dst;
}
int iround(double value)
{
	return (int)(value + (value >= 0 ? 0.5 : -0.5));
}
void convertScale(IplImage* gray8, IplImage* gray32, float alpha)
{
	uchar* srcData = (uchar*)gray8->imageData;//数据的起始地址
	float* dstData = (float*)gray32->imageData;

	int width = gray8->width;//图像的行长度，即多少个字节
	int height = gray8->height;

	for (int row = 0; row < height; row++)
	{
		for (int col = 0; col < width; col++)
		{
			dstData[row*width + col] = srcData[row*width + col] * alpha;
		}
	}
}
void mSet(Mat* mat, int row, int col, double value)
{
	int type;
	type = CV_MAT_TYPE(mat->type);
	if ((unsigned)row >= (unsigned)mat->rows || (unsigned)col >= (unsigned)mat->cols)
		return;

	if (type == CV_32FC1)
		((float*)(void*)(mat->data.ptr + (size_t)mat->step*row))[col] = (float)value;
	else
	{
		if (type != CV_64FC1)
			return;
		((double*)(void*)(mat->data.ptr + (size_t)mat->step*row))[col] = value;
	}
}
char* getSeqElem(const Seq *seq, int index)
{
	SeqBlock *block;
	int count, total = seq->total;

	if ((unsigned)index >= (unsigned)total)
	{
		index += index < 0 ? total : 0;
		index -= index >= total ? total : 0;
		if ((unsigned)index >= (unsigned)total)
			return 0;
	}

	block = seq->first;
	if (index + index <= total)
	{
		while (index >= (count = block->count))
		{
			block = block->next;
			index -= count;
		}
	}
	else
	{
		do
		{
			block = block->prev;
			total -= block->count;
		} while (index < total);
		index -= total;
	}

	return block->data + index * seq->elem_size;
}
void GEMM(const Mat* src1, const Mat* src2, double alpha, Mat* dst0, int tABC)
{
	double a, b;
	for (int i = 0; i < dst0->rows; i++)
	{
		for (int j = 0; j < dst0->cols; j++)
		{
			dst0->data.db[i*dst0->cols + j] = 0.0;
			for (int k = 0; k < src2->rows; k++)
			{
				{
					a = src1->data.db[i*src1->rows + k];
					b = src2->data.db[k*src2->cols + j];
					dst0->data.db[i*dst0->cols + j] += alpha*a*b;
				}
			}
		}
	}
}
int iceil(double value)
{
	int i = iround(value);
	float diff = (float)(i - value);
	return i + (diff < 0);
}
int ifloor(double value)
{
	int i = iround(value);
	float diff = (float)(value - i);
	return i - (diff < 0);
}