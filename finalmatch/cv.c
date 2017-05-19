#include "header.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

static void icheckHuge(Mat* arr);
static void* alignPtr(void* ptr, int n);
static void* fastMalloc(size_t size);
static void* ialloc(size_t size);
static int iAlign(int size, int align);
static void initMemStorage(MemStorage* storage, int block_size);
static void startReadSeq(const Seq *seq, SeqReader * reader, int reverse);
static int getSeqReaderPos(SeqReader* reader);
static void setSeqReaderPos(SeqReader* reader, int index, int is_relative);
static char* iMed3(char* a, char* b, char* c, CmpFunc cmp_func, void* aux);
static int sliceLength(Slice slice, const Seq* seq);
static void fastFree(void* ptr);
static void iDestroyMemStorage(MemStorage* storage);
static void iGetColorModel(int nchannels, const char** colorModel, const char** channelSeq);
static IplImage* initImageHeader(IplImage * image, Size size, int depth, int channels, int origin, int align);
static IplImage *createImageHeader(Size size, int depth, int channels);
static void* lAlignPtr(const void* ptr, int align);
static void createData(void* arr);
static void iFreeSeqBlock(Seq *seq, int in_front_of);
static IplROI* iCreateROI(int coi, int xOffset, int yOffset, int width, int height);
static void releaseImageHeader(IplImage** image);
static void  decRefData(void* arr);
static void releaseData(void* arr);
static Mat* createMatHeader(int rows, int cols, int type);
static int lAlignLeft(int size, int align);
static void saveMemStoragePos(const MemStorage * storage, MemStoragePos * pos);
static void restoreMemStoragePos(MemStorage * storage, MemStoragePos * pos);
static void iGoNextMemBlock(MemStorage * storage);
static void* memStorageAlloc(MemStorage* storage, size_t size);
static void iGrowSeq(Seq *seq, int in_front_of);
static void CreatGauss(double sigma, double *cf, int n);
//--------------------------------------------------------------------------------------------------------
void icheckHuge(Mat* arr)
{
	if ((int64)arr->step*arr->rows > INT_MAX)
		arr->type &= ~CV_MAT_CONT_FLAG;
}
void* alignPtr(void* ptr, int n)
{
	return (void*)(((size_t)ptr + n - 1) & -n);
}
void* fastMalloc(size_t size)
{
	uchar* udata = (uchar*)malloc(size + sizeof(void*) + CV_MALLOC_ALIGN);
	if (!udata)
		return 0;
	uchar** adata = alignPtr((uchar**)udata + 1, CV_MALLOC_ALIGN);
	adata[-1] = udata;
	return adata;
}
void* ialloc(size_t size)
{
	return fastMalloc(size);
}
int iAlign(int size, int align)
{
	if ((align & (align - 1)) != 0 || size >= INT_MAX)
		return -1;
	return (size + align - 1) & -align;
}
void initMemStorage(MemStorage* storage, int block_size)
{
	if (!storage)
		return;

	if (block_size <= 0)
		block_size = CV_STORAGE_BLOCK_SIZE;

	block_size = iAlign(block_size, CV_STRUCT_ALIGN);
	if (sizeof(MemBlock) % CV_STRUCT_ALIGN != 0)
	{
		return;
	}

	memset(storage, 0, sizeof(*storage));
	storage->signature = CV_STORAGE_MAGIC_VAL;
	storage->block_size = block_size;
}
void startReadSeq(const Seq *seq, SeqReader * reader, int reverse)
{
	SeqBlock *first_block;
	SeqBlock *last_block;

	if (reader)
	{
		reader->seq = 0;
		reader->block = 0;
		reader->ptr = reader->block_max = reader->block_min = 0;
	}

	if (!seq || !reader)
		return;

	reader->header_size = sizeof(SeqReader);
	reader->seq = (Seq*)seq;

	first_block = seq->first;

	if (first_block)
	{
		last_block = first_block->prev;
		reader->ptr = first_block->data;
		reader->prev_elem = CV_GET_LAST_ELEM(seq, last_block);
		reader->delta_index = seq->first->start_index;

		if (reverse)
		{
			char *temp = reader->ptr;

			reader->ptr = reader->prev_elem;
			reader->prev_elem = temp;

			reader->block = last_block;
		}
		else
		{
			reader->block = first_block;
		}

		reader->block_min = reader->block->data;
		reader->block_max = reader->block_min + reader->block->count * seq->elem_size;
	}
	else
	{
		reader->delta_index = 0;
		reader->block = 0;

		reader->ptr = reader->prev_elem = reader->block_min = reader->block_max = 0;
	}
}
int getSeqReaderPos(SeqReader* reader)
{
	int elem_size;
	int index = -1;

	if (!reader || !reader->ptr)
		return 0;

	elem_size = reader->seq->elem_size;
	if (elem_size <= ICV_SHIFT_TAB_MAX && (index = icvPower2ShiftTab[elem_size - 1]) >= 0)
		index = (int)((reader->ptr - reader->block_min) >> index);
	else
		index = (int)((reader->ptr - reader->block_min) / elem_size);

	index += reader->block->start_index - reader->delta_index;

	return index;
}
void setSeqReaderPos(SeqReader* reader, int index, int is_relative)
{
	SeqBlock *block;
	int elem_size, count, total;

	if (!reader || !reader->seq)
		return;

	total = reader->seq->total;
	elem_size = reader->seq->elem_size;

	if (!is_relative)
	{
		if (index < 0)
		{
			if (index < -total)
				return;

			index += total;
		}
		else if (index >= total)
		{
			index -= total;
			if (index >= total)
				return;
		}

		block = reader->seq->first;
		if (index >= (count = block->count))
		{
			if (index + index <= total)
			{
				do
				{
					block = block->next;
					index -= count;
				} while (index >= (count = block->count));
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
		}
		reader->ptr = block->data + index * elem_size;
		if (reader->block != block)
		{
			reader->block = block;
			reader->block_min = block->data;
			reader->block_max = block->data + block->count * elem_size;
		}
	}
	else
	{
		char* ptr = reader->ptr;
		index *= elem_size;
		block = reader->block;

		if (index > 0)
		{
			while (ptr + index >= reader->block_max)
			{
				int delta = (int)(reader->block_max - ptr);
				index -= delta;
				reader->block = block = block->next;
				reader->block_min = ptr = block->data;
				reader->block_max = block->data + block->count*elem_size;
			}
			reader->ptr = ptr + index;
		}
		else
		{
			while (ptr + index < reader->block_min)
			{
				int delta = (int)(ptr - reader->block_min);
				index += delta;
				reader->block = block = block->prev;
				reader->block_min = block->data;
				reader->block_max = ptr = block->data + block->count*elem_size;
			}
			reader->ptr = ptr + index;
		}
	}
}
char* iMed3(char* a, char* b, char* c, CmpFunc cmp_func, void* aux)
{
	return cmp_func(a, b, aux) < 0 ?
		(cmp_func(b, c, aux) < 0 ? b : cmp_func(a, c, aux) < 0 ? c : a)
		: (cmp_func(b, c, aux) > 0 ? b : cmp_func(a, c, aux) < 0 ? a : c);
}
int sliceLength(Slice slice, const Seq* seq)
{
	int total = seq->total;
	int length = slice.end_index - slice.start_index;

	if (length != 0)
	{
		if (slice.start_index < 0)
			slice.start_index += total;
		if (slice.end_index <= 0)
			slice.end_index += total;

		length = slice.end_index - slice.start_index;
	}

	while (length < 0)
		length += total;
	if (length > total)
		length = total;

	return length;
}
void fastFree(void* ptr)
{
	if (ptr)
	{
		uchar* udata = ((uchar**)ptr)[-1];
		if (!(udata < (uchar*)ptr && ((uchar*)ptr - udata) <= (ptrdiff_t)(sizeof(void*) + CV_MALLOC_ALIGN)))
			return;
		free(udata);
	}
}
void iDestroyMemStorage(MemStorage* storage)
{
	int k = 0;
	MemBlock *block;
	MemBlock *dst_top = 0;

	if (!storage)
		return;

	if (storage->parent)
		dst_top = storage->parent->top;

	for (block = storage->bottom; block != 0; k++)
	{
		MemBlock *temp = block;

		block = block->next;
		if (storage->parent)
		{
			if (dst_top)
			{
				temp->prev = dst_top;
				temp->next = dst_top->next;
				if (temp->next)
					temp->next->prev = temp;
				dst_top = dst_top->next = temp;
			}
			else
			{
				dst_top = storage->parent->bottom = storage->parent->top = temp;
				temp->prev = temp->next = 0;
				storage->free_space = storage->block_size - sizeof(*temp);
			}
		}
		else
		{
			ifree(&temp);
		}
	}

	storage->top = storage->bottom = 0;
	storage->free_space = 0;
}
void iGetColorModel(int nchannels, const char** colorModel, const char** channelSeq)
{
	static const char* tab[][2] =
	{
		{ "GRAY", "GRAY" },
		{ "", "" },
		{ "RGB", "BGR" },
		{ "RGB", "BGRA" }
	};

	nchannels--;
	*colorModel = *channelSeq = "";

	if ((unsigned)nchannels <= 3)
	{
		*colorModel = tab[nchannels][0];
		*channelSeq = tab[nchannels][1];
	}
}
IplImage* initImageHeader(IplImage * image, Size size, int depth,int channels, int origin, int align)
{
	const char *colorModel, *channelSeq;

	if (!image)
		return 0;

	memset(image, 0, sizeof(*image));
	image->nSize = sizeof(*image);

	iGetColorModel(channels, &colorModel, &channelSeq);
	strncpy(image->colorModel, colorModel, 4);
	strncpy(image->channelSeq, channelSeq, 4);

	if (size.width < 0 || size.height < 0)
		return 0;

	if ((depth != (int)IPL_DEPTH_1U && depth != (int)IPL_DEPTH_8U &&
		depth != (int)IPL_DEPTH_8S && depth != (int)IPL_DEPTH_16U &&
		depth != (int)IPL_DEPTH_16S && depth != (int)IPL_DEPTH_32S &&
		depth != (int)IPL_DEPTH_32F && depth != (int)IPL_DEPTH_64F) ||
		channels < 0)
		return 0;

	if (origin != CV_ORIGIN_BL && origin != CV_ORIGIN_TL)
		return 0;

	if (align != 4 && align != 8)
		return 0;

	image->width = size.width;
	image->height = size.height;

	if (image->roi)
	{
		image->roi->coi = 0;
		image->roi->xOffset = image->roi->yOffset = 0;
		image->roi->width = size.width;
		image->roi->height = size.height;
	}

	image->nChannels = MAX(channels, 1);
	image->depth = depth;
	image->align = align;
	image->widthStep = (((image->width * image->nChannels *
		(image->depth & ~IPL_DEPTH_SIGN) + 7) / 8) + align - 1) & (~(align - 1));
	image->origin = origin;
	image->imageSize = image->widthStep * image->height;

	return image;
}
IplImage *createImageHeader(Size size, int depth, int channels)
{
	IplImage *img;

	if (!IPL.createHeader)
	{
		img = (IplImage *)ialloc(sizeof(*img));
		initImageHeader(img, size, depth, channels, IPL_ORIGIN_TL,
			CV_DEFAULT_IMAGE_ROW_ALIGN);
	}
	else
	{
		const char *colorModel, *channelSeq;

		iGetColorModel(channels, &colorModel, &channelSeq);

		img = IPL.createHeader(channels, 0, depth, (char*)colorModel, (char*)channelSeq,
			IPL_DATA_ORDER_PIXEL, IPL_ORIGIN_TL,
			CV_DEFAULT_IMAGE_ROW_ALIGN,
			size.width, size.height, 0, 0, 0, 0);
	}

	return img;
}
void* lAlignPtr(const void* ptr, int align)
{
	if (!((align & (align - 1)) == 0))
		return NULL;
	return (void*)(((size_t)ptr + align - 1) & ~(size_t)(align - 1));
}
void createData(void* arr)
{
	if (CV_IS_MAT_HDR_Z(arr))
	{
		size_t step, total_size;
		Mat* mat = (Mat*)arr;
		step = mat->step;

		if (mat->rows == 0 || mat->cols == 0)
			return;

		if (mat->data.ptr != 0)
			return;

		if (step == 0)
			step = CV_ELEM_SIZE(mat->type)*mat->cols;

		int64 _total_size = (int64)step*mat->rows + sizeof(int) + CV_MALLOC_ALIGN;
		total_size = (size_t)_total_size;
		if (_total_size != (int64)total_size)
			return;

		mat->refcount = (int*)ialloc((size_t)total_size);
		mat->data.ptr = (uchar*)lAlignPtr(mat->refcount + 1, CV_MALLOC_ALIGN);
		*mat->refcount = 1;
	}
	else if (CV_IS_IMAGE_HDR(arr))
	{
		IplImage* img = (IplImage*)arr;

		if (img->imageData != 0)
			return;

		if (!IPL.allocateData)
		{
			img->imageData = img->imageDataOrigin =
				(char*)ialloc((size_t)img->imageSize);
		}
		else
		{
			int depth = img->depth;
			int width = img->width;

			if (img->depth == IPL_DEPTH_32F || img->depth == IPL_DEPTH_64F)
			{
				img->width *= img->depth == IPL_DEPTH_32F ? sizeof(float) : sizeof(double);
				img->depth = IPL_DEPTH_8U;
			}

			IPL.allocateData(img, 0, 0);

			img->width = width;
			img->depth = depth;
		}
	}
	else if (CV_IS_MATND_HDR(arr))
	{
		MatND* mat = (MatND*)arr;
		size_t total_size = CV_ELEM_SIZE(mat->type);

		if (mat->dim[0].size == 0)
			return;

		if (mat->data != 0)
			return;

		if (CV_IS_MAT_CONT(mat->type))
		{
			total_size = (size_t)mat->dim[0].size*(mat->dim[0].step != 0 ?
				(size_t)mat->dim[0].step : total_size);
		}
		else
		{
			int i;
			for (i = mat->dims - 1; i >= 0; i--)
			{
				size_t size = (size_t)mat->dim[i].step*mat->dim[i].size;

				if (total_size < size)
					total_size = size;
			}
		}

		mat->refcount = (int*)ialloc(total_size +
			sizeof(int) + CV_MALLOC_ALIGN);
		mat->data = (uchar*)lAlignPtr(mat->refcount + 1, CV_MALLOC_ALIGN);
		*mat->refcount = 1;
	}
}
void iFreeSeqBlock(Seq *seq, int in_front_of)
{
	SeqBlock *block = seq->first;

	if ((in_front_of ? block : block->prev)->count != 0)
	{
		return;
	}

	if (block == block->prev)  /* single block case */
	{
		block->count = (int)(seq->block_max - block->data) + block->start_index * seq->elem_size;
		block->data = seq->block_max - block->count;
		seq->first = 0;
		seq->ptr = seq->block_max = 0;
		seq->total = 0;
	}
	else
	{
		if (!in_front_of)
		{
			block = block->prev;
			if (seq->ptr != block->data)
			{
				return;
			}

			block->count = (int)(seq->block_max - seq->ptr);
			seq->block_max = seq->ptr = block->prev->data +
				block->prev->count * seq->elem_size;
		}
		else
		{
			int delta = block->start_index;

			block->count = delta * seq->elem_size;
			block->data -= block->count;

			/* Update start indices of sequence blocks: */
			for (;;)
			{
				block->start_index -= delta;
				block = block->next;
				if (block == seq->first)
					break;
			}

			seq->first = block->next;
		}

		block->prev->next = block->next;
		block->next->prev = block->prev;
	}

	if (block->count <= 0 || block->count % seq->elem_size != 0)
	{
		return;
	}
	block->next = seq->free_blocks;
	seq->free_blocks = block;
}
IplROI* iCreateROI(int coi, int xOffset, int yOffset, int width, int height)
{
	IplROI *roi;
	if (!IPL.createROI)
	{
		roi = (IplROI*)ialloc(sizeof(*roi));

		roi->coi = coi;
		roi->xOffset = xOffset;
		roi->yOffset = yOffset;
		roi->width = width;
		roi->height = height;
	}
	else
	{
		roi = IPL.createROI(coi, xOffset, yOffset, width, height);
	}

	return roi;
}
void releaseImageHeader(IplImage** image)
{
	if (!image)
		return;

	if (*image)
	{
		IplImage* img = *image;
		*image = 0;

		if (!IPL.deallocate)
		{
			ifree(&img->roi);
			ifree(&img);
		}
		else
		{
			IPL.deallocate(img, IPL_IMAGE_HEADER | IPL_IMAGE_ROI);
		}
	}
}
void decRefData(void* arr)
{
	if (CV_IS_MAT(arr))
	{
		Mat* mat = (Mat*)arr;
		mat->data.ptr = NULL;
		if (mat->refcount != NULL && --*mat->refcount == 0)
			ifree(&mat->refcount);
		mat->refcount = NULL;
	}
	else if (CV_IS_MATND(arr))
	{
		MatND* mat = (MatND*)arr;
		mat->data = NULL;
		if (mat->refcount != NULL && --*mat->refcount == 0)
			ifree(&mat->refcount);
		mat->refcount = NULL;
	}
}
void releaseData(void* arr)
{
	if (CV_IS_MAT_HDR(arr) || CV_IS_MATND_HDR(arr))
	{
		Mat* mat = (Mat*)arr;
		decRefData(mat);
	}
	else if (CV_IS_IMAGE_HDR(arr))
	{
		IplImage* img = (IplImage*)arr;

		if (!IPL.deallocate)
		{
			char* ptr = img->imageDataOrigin;
			img->imageData = img->imageDataOrigin = 0;
			ifree(&ptr);
		}
		else
		{
			IPL.deallocate(img, IPL_IMAGE_DATA);
		}
	}
}
Mat* createMatHeader(int rows, int cols, int type)
{
	type = CV_MAT_TYPE(type);

	if (rows < 0 || cols <= 0)
		return 0;

	int min_step = CV_ELEM_SIZE(type)*cols;
	if (min_step <= 0)
		return 0;

	Mat* arr = (Mat*)ialloc(sizeof(*arr));

	arr->step = min_step;
	arr->type = CV_MAT_MAGIC_VAL | type | CV_MAT_CONT_FLAG;
	arr->rows = rows;
	arr->cols = cols;
	arr->data.ptr = 0;
	arr->refcount = 0;
	arr->hdr_refcount = 1;

	icheckHuge(arr);
	return arr;
}
int lAlignLeft(int size, int align)
{
	return size & -align;
}
void saveMemStoragePos(const MemStorage * storage, MemStoragePos * pos)
{
	if (!storage || !pos)
		return;

	pos->top = storage->top;
	pos->free_space = storage->free_space;
}
void restoreMemStoragePos(MemStorage * storage, MemStoragePos * pos)
{
	if (!storage || !pos)
		return;
	if (pos->free_space > storage->block_size)
		return;	

	storage->top = pos->top;
	storage->free_space = pos->free_space;

	if (!storage->top)
	{
		storage->top = storage->bottom;
		storage->free_space = storage->top ? storage->block_size - sizeof(MemBlock) : 0;
	}
}
void iGoNextMemBlock(MemStorage * storage)
{
	if (!storage)
		return;

	if (!storage->top || !storage->top->next)
	{
		MemBlock *block;

		if (!(storage->parent))
		{
			block = (MemBlock *)ialloc(storage->block_size);
		}
		else
		{
			MemStorage *parent = storage->parent;
			MemStoragePos parent_pos;

			saveMemStoragePos(parent, &parent_pos);
			iGoNextMemBlock(parent);

			block = parent->top;
			restoreMemStoragePos(parent, &parent_pos);

			if (block == parent->top)  /* the single allocated block */
			{
				if (parent->bottom != block)
				{
					return;
				}
				parent->top = parent->bottom = 0;
				parent->free_space = 0;
			}
			else
			{
				/* cut the block from the parent's list of blocks */
				parent->top->next = block->next;
				if (block->next)
					block->next->prev = parent->top;
			}
		}

		/* link block */
		block->next = 0;
		block->prev = storage->top;

		if (storage->top)
			storage->top->next = block;
		else
			storage->top = storage->bottom = block;
	}

	if (storage->top->next)
		storage->top = storage->top->next;
	storage->free_space = storage->block_size - sizeof(MemBlock);
	if (!(storage->free_space % CV_STRUCT_ALIGN == 0))
		return;
}
void* memStorageAlloc(MemStorage* storage, size_t size)
{
	char *ptr = 0;
	if (!storage)
		return 0;

	if (size > INT_MAX)
		return 0;

	if (storage->free_space % CV_STRUCT_ALIGN != 0)
	{
		return 0;
	}

	if ((size_t)storage->free_space < size)
	{
		size_t max_free_space = lAlignLeft(storage->block_size - sizeof(MemBlock), CV_STRUCT_ALIGN);
		if (max_free_space < size)
			return 0;
		iGoNextMemBlock(storage);
	}

	ptr = ICV_FREE_PTR(storage);
	if ((size_t)ptr % CV_STRUCT_ALIGN != 0)
	{
		return 0;
	}
	storage->free_space = lAlignLeft(storage->free_space - (int)size, CV_STRUCT_ALIGN);

	return ptr;
}
void iGrowSeq(Seq *seq, int in_front_of)
{
	SeqBlock *block;

	if (!seq)
		return;
	block = seq->free_blocks;

	if (!block)
	{
		int elem_size = seq->elem_size;
		int delta_elems = seq->delta_elems;
		MemStorage *storage = seq->storage;

		if (seq->total >= delta_elems * 4)
			setSeqBlockSize(seq, delta_elems * 2);

		if (!storage)
			return;

		if ((size_t)(ICV_FREE_PTR(storage) - seq->block_max) < CV_STRUCT_ALIGN &&
			storage->free_space >= seq->elem_size && !in_front_of)
		{
			int delta = storage->free_space / elem_size;

			delta = MIN(delta, delta_elems) * elem_size;
			seq->block_max += delta;
			storage->free_space = lAlignLeft((int)(((char*)storage->top + storage->block_size) -
				seq->block_max), CV_STRUCT_ALIGN);
			return;
		}
		else
		{
			int delta = elem_size * delta_elems + ICV_ALIGNED_SEQ_BLOCK_SIZE;

			/* Try to allocate <delta_elements> elements: */
			if (storage->free_space < delta)
			{
				int small_block_size = MAX(1, delta_elems / 3)*elem_size +
					ICV_ALIGNED_SEQ_BLOCK_SIZE;
				/* try to allocate smaller part */
				if (storage->free_space >= small_block_size + CV_STRUCT_ALIGN)
				{
					delta = (storage->free_space - ICV_ALIGNED_SEQ_BLOCK_SIZE) / seq->elem_size;
					delta = delta*seq->elem_size + ICV_ALIGNED_SEQ_BLOCK_SIZE;
				}
				else
				{
					iGoNextMemBlock(storage);
					if (storage->free_space < delta)
						return;
				}
			}

			block = (SeqBlock*)memStorageAlloc(storage, delta);
			block->data = (char*)lAlignPtr(block + 1, CV_STRUCT_ALIGN);
			block->count = delta - ICV_ALIGNED_SEQ_BLOCK_SIZE;
			block->prev = block->next = 0;
		}
	}
	else
	{
		seq->free_blocks = block->next;
	}

	if (!(seq->first))
	{
		seq->first = block;
		block->prev = block->next = block;
	}
	else
	{
		block->prev = seq->first->prev;
		block->next = seq->first;
		block->prev->next = block->next->prev = block;
	}

	if (!(block->count % seq->elem_size == 0 && block->count > 0))
		return;

	if (!in_front_of)
	{
		seq->ptr = block->data;
		seq->block_max = block->data + block->count;
		block->start_index = block == block->prev ? 0 :
			block->prev->start_index + block->prev->count;
	}
	else
	{
		int delta = block->count / seq->elem_size;
		block->data += block->count;

		if (block != block->prev)
		{
			if (seq->first->start_index != 0)
				return;
			seq->first = block;
		}
		else
		{
			seq->block_max = seq->ptr = block->data;
		}

		block->start_index = 0;

		for (;;)
		{
			block->start_index += delta;
			block = block->next;
			if (block == seq->first)
				break;
		}
	}

	block->count = 0;
}
//------------------------------------------------------------------------------------------------
Mat* initMatHeader(Mat* arr, int rows, int cols, int type, void* data, int step)
{
	if (!arr)
		return 0;

	if ((unsigned)CV_MAT_DEPTH(type) > CV_DEPTH_MAX)
		return 0;

	if (rows < 0 || cols <= 0)
		return 0;

	type = CV_MAT_TYPE(type);
	arr->type = type | CV_MAT_MAGIC_VAL;
	arr->rows = rows;
	arr->cols = cols;
	arr->data.ptr = (uchar*)data;
	arr->refcount = 0;
	arr->hdr_refcount = 0;

	int pix_size = CV_ELEM_SIZE(type);
	int min_step = arr->cols*pix_size;

	if (step != CV_AUTOSTEP && step != 0)
	{
		if (step < min_step)
			return 0;
		arr->step = step;
	}
	else
	{
		arr->step = min_step;
	}

	arr->type = CV_MAT_MAGIC_VAL | type |
		(arr->rows == 1 || arr->step == min_step ? CV_MAT_CONT_FLAG : 0);

	icheckHuge(arr);
	return arr;
}
MemStorage* createMemStorage(int block_size)
{
	MemStorage* storage = (MemStorage *)ialloc(sizeof(MemStorage));
	initMemStorage(storage, block_size);
	return storage;
}
void changeSeqBlock(void* _reader, int direction)
{
	SeqReader* reader = (SeqReader*)_reader;

	if (!reader)
		return;

	if (direction > 0)
	{
		reader->block = reader->block->next;
		reader->ptr = reader->block->data;
	}
	else
	{
		reader->block = reader->block->prev;
		reader->ptr = CV_GET_LAST_ELEM(reader->seq, reader->block);
	}
	reader->block_min = reader->block->data;
	reader->block_max = reader->block_min + reader->block->count * reader->seq->elem_size;
}
void seqSort(Seq* seq, CmpFunc cmp_func, void* aux)
{
	int elem_size;
	int isort_thresh = 7;
	SeqReader left, right;
	int sp = 0;

	struct
	{
		SeqReaderPos lb;
		SeqReaderPos ub;
	}stack[48];

	if (!CV_IS_SEQ(seq))
		return;

	if (!cmp_func)
		return;

	if (seq->total <= 1)
		return;

	elem_size = seq->elem_size;
	isort_thresh *= elem_size;

	startReadSeq(seq, &left, 0);
	right = left;
	CV_SAVE_READER_POS(left, stack[0].lb);
	CV_PREV_SEQ_ELEM(elem_size, right);
	CV_SAVE_READER_POS(right, stack[0].ub);

	while (sp >= 0)
	{
		CV_RESTORE_READER_POS(left, stack[sp].lb);
		CV_RESTORE_READER_POS(right, stack[sp].ub);
		sp--;

		for (;;)
		{
			int i, n, m;
			SeqReader ptr, ptr2;

			if (left.block == right.block)
				n = (int)(right.ptr - left.ptr) + elem_size;
			else
			{
				n = getSeqReaderPos(&right);
				n = (n - getSeqReaderPos(&left) + 1)*elem_size;
			}

			if (n <= isort_thresh)
			{
			insert_sort:
				ptr = ptr2 = left;
				CV_NEXT_SEQ_ELEM(elem_size, ptr);
				CV_NEXT_SEQ_ELEM(elem_size, right);
				while (ptr.ptr != right.ptr)
				{
					ptr2.ptr = ptr.ptr;
					if (ptr2.block != ptr.block)
					{
						ptr2.block = ptr.block;
						ptr2.block_min = ptr.block_min;
						ptr2.block_max = ptr.block_max;
					}
					while (ptr2.ptr != left.ptr)
					{
						char* cur = ptr2.ptr;
						CV_PREV_SEQ_ELEM(elem_size, ptr2);
						if (cmp_func(ptr2.ptr, cur, aux) <= 0)
							break;
						CV_SWAP_ELEMS(ptr2.ptr, cur, elem_size);
					}
					CV_NEXT_SEQ_ELEM(elem_size, ptr);
				}
				break;
			}
			else
			{
				SeqReader left0, left1, right0, right1;
				SeqReader tmp0, tmp1;
				char *m1, *m2, *m3, *pivot;
				int swap_cnt = 0;
				int l, l0, l1, r, r0, r1;

				left0 = tmp0 = left;
				right0 = right1 = right;
				n /= elem_size;

				if (n > 40)
				{
					int d = n / 8;
					char *p1, *p2, *p3;
					p1 = tmp0.ptr;
					setSeqReaderPos(&tmp0, d, 1);
					p2 = tmp0.ptr;
					setSeqReaderPos(&tmp0, d, 1);
					p3 = tmp0.ptr;
					m1 = iMed3(p1, p2, p3, cmp_func, aux);
					setSeqReaderPos(&tmp0, (n / 2) - d * 3, 1);
					p1 = tmp0.ptr;
					setSeqReaderPos(&tmp0, d, 1);
					p2 = tmp0.ptr;
					setSeqReaderPos(&tmp0, d, 1);
					p3 = tmp0.ptr;
					m2 = iMed3(p1, p2, p3, cmp_func, aux);
					setSeqReaderPos(&tmp0, n - 1 - d * 3 - n / 2, 1);
					p1 = tmp0.ptr;
					setSeqReaderPos(&tmp0, d, 1);
					p2 = tmp0.ptr;
					setSeqReaderPos(&tmp0, d, 1);
					p3 = tmp0.ptr;
					m3 = iMed3(p1, p2, p3, cmp_func, aux);
				}
				else
				{
					m1 = tmp0.ptr;
					setSeqReaderPos(&tmp0, n / 2, 1);
					m2 = tmp0.ptr;
					setSeqReaderPos(&tmp0, n - 1 - n / 2, 1);
					m3 = tmp0.ptr;
				}

				pivot = iMed3(m1, m2, m3, cmp_func, aux);
				left = left0;
				if (pivot != left.ptr)
				{
					CV_SWAP_ELEMS(pivot, left.ptr, elem_size);
					pivot = left.ptr;
				}
				CV_NEXT_SEQ_ELEM(elem_size, left);
				left1 = left;

				for (;;)
				{
					while (left.ptr != right.ptr && (r = cmp_func(left.ptr, pivot, aux)) <= 0)
					{
						if (r == 0)
						{
							if (left1.ptr != left.ptr)
								CV_SWAP_ELEMS(left1.ptr, left.ptr, elem_size);
							swap_cnt = 1;
							CV_NEXT_SEQ_ELEM(elem_size, left1);
						}
						CV_NEXT_SEQ_ELEM(elem_size, left);
					}

					while (left.ptr != right.ptr && (r = cmp_func(right.ptr, pivot, aux)) >= 0)
					{
						if (r == 0)
						{
							if (right1.ptr != right.ptr)
								CV_SWAP_ELEMS(right1.ptr, right.ptr, elem_size);
							swap_cnt = 1;
							CV_PREV_SEQ_ELEM(elem_size, right1);
						}
						CV_PREV_SEQ_ELEM(elem_size, right);
					}

					if (left.ptr == right.ptr)
					{
						r = cmp_func(left.ptr, pivot, aux);
						if (r == 0)
						{
							if (left1.ptr != left.ptr)
								CV_SWAP_ELEMS(left1.ptr, left.ptr, elem_size);
							swap_cnt = 1;
							CV_NEXT_SEQ_ELEM(elem_size, left1);
						}
						if (r <= 0)
						{
							CV_NEXT_SEQ_ELEM(elem_size, left);
						}
						else
						{
							CV_PREV_SEQ_ELEM(elem_size, right);
						}
						break;
					}

					CV_SWAP_ELEMS(left.ptr, right.ptr, elem_size);
					CV_NEXT_SEQ_ELEM(elem_size, left);
					r = left.ptr == right.ptr;
					CV_PREV_SEQ_ELEM(elem_size, right);
					swap_cnt = 1;
					if (r)
						break;
				}

				if (swap_cnt == 0)
				{
					left = left0, right = right0;
					goto insert_sort;
				}

				l = getSeqReaderPos(&left);
				if (l == 0)
					l = seq->total;
				l0 = getSeqReaderPos(&left0);
				l1 = getSeqReaderPos(&left1);
				if (l1 == 0)
					l1 = seq->total;

				n = MIN(l - l1, l1 - l0);
				if (n > 0)
				{
					tmp0 = left0;
					tmp1 = left;
					setSeqReaderPos(&tmp1, 0 - n, 1);
					for (i = 0; i < n; i++)
					{
						CV_SWAP_ELEMS(tmp0.ptr, tmp1.ptr, elem_size);
						CV_NEXT_SEQ_ELEM(elem_size, tmp0);
						CV_NEXT_SEQ_ELEM(elem_size, tmp1);
					}
				}

				r = getSeqReaderPos(&right);
				r0 = getSeqReaderPos(&right0);
				r1 = getSeqReaderPos(&right1);
				m = MIN(r0 - r1, r1 - r);
				if (m > 0)
				{
					tmp0 = left;
					tmp1 = right0;
					setSeqReaderPos(&tmp1, 1 - m, 1);
					for (i = 0; i < m; i++)
					{
						CV_SWAP_ELEMS(tmp0.ptr, tmp1.ptr, elem_size);
						CV_NEXT_SEQ_ELEM(elem_size, tmp0);
						CV_NEXT_SEQ_ELEM(elem_size, tmp1);
					}
				}

				n = l - l1;
				m = r1 - r;
				if (n > 1)
				{
					if (m > 1)
					{
						if (n > m)
						{
							sp++;
							CV_SAVE_READER_POS(left0, stack[sp].lb);
							setSeqReaderPos(&left0, n - 1, 1);
							CV_SAVE_READER_POS(left0, stack[sp].ub);
							left = right = right0;
							setSeqReaderPos(&left, 1 - m, 1);
						}
						else
						{
							sp++;
							CV_SAVE_READER_POS(right0, stack[sp].ub);
							setSeqReaderPos(&right0, 1 - m, 1);
							CV_SAVE_READER_POS(right0, stack[sp].lb);
							left = right = left0;
							setSeqReaderPos(&right, n - 1, 1);
						}
					}
					else
					{
						left = right = left0;
						setSeqReaderPos(&right, n - 1, 1);
					}
				}
				else if (m > 1)
				{
					left = right = right0;
					setSeqReaderPos(&left, 1 - m, 1);
				}
				else
					break;
			}
		}
	}
}
void* tSeqToArray(const Seq *seq, void *array, Slice slice)
{
	int elem_size, total;
	SeqReader reader;
	char *dst = (char*)array;

	if (!seq || !array)
		return 0;

	elem_size = seq->elem_size;
	total = sliceLength(slice, seq)*elem_size;

	if (total == 0)
		return 0;

	startReadSeq(seq, &reader, 0);
	setSeqReaderPos(&reader, slice.start_index, 0);

	do
	{
		int count = (int)(reader.block_max - reader.ptr);
		if (count > total)
			count = total;

		memcpy(dst, reader.ptr, count);
		dst += count;
		reader.block = reader.block->next;
		reader.ptr = reader.block->data;
		reader.block_max = reader.ptr + reader.block->count*elem_size;
		total -= count;
	} while (total > 0);

	return array;
}
void ifree_(void* ptr)
{
	fastFree(ptr);
}
void releaseMemStorage(MemStorage** storage)
{
	if (!storage)
		return;

	MemStorage* st = *storage;
	*storage = 0;
	if (st)
	{
		iDestroyMemStorage(st);
		ifree(&st);
	}
}
IplImage *createImage(Size size, int depth, int channels)
{
	IplImage *img = createImageHeader(size, depth, channels);

	if (!img)
		return 0;
	createData(img);
	return img;
}
void setZero(IplImage* src)
{
	uchar* srcData = (uchar*)src->imageData;//数据的起始地址

	int width = src->widthStep;//图像的行长度，即多少个字节
	int height = src->height;

	for (int row = 0; row < height; row++)
	{
		for (int col = 0; col < width; col++)
		{
			srcData[row*width + col] = 0;
		}
	}
}
void setImageROI(IplImage* image, Rect rect)
{
	if (!image)
		return;
	// allow zero ROI width or height
	if (rect.width >= 0 && rect.height >= 0 &&
		rect.x < image->width && rect.y < image->height &&
		rect.x + rect.width >= (int)(rect.width > 0) &&
		rect.y + rect.height >= (int)(rect.height > 0));

	{
		rect.width += rect.x;
		rect.height += rect.y;

		rect.x = MAX(rect.x, 0);
		rect.y = MAX(rect.y, 0);
		rect.width = MIN(rect.width, image->width);
		rect.height = MIN(rect.height, image->height);

		rect.width -= rect.x;
		rect.height -= rect.y;

		if (image->roi)
		{
			image->roi->xOffset = rect.x;
			image->roi->yOffset = rect.y;
			image->roi->width = rect.width;
			image->roi->height = rect.height;
		}
		else
			image->roi = iCreateROI(0, rect.x, rect.y, rect.width, rect.height);
	}
}
void releaseImage(IplImage ** image)
{
	if (!image)
		return;

	if (*image)
	{
		IplImage* img = *image;
		*image = 0;

		releaseData(img);
		releaseImageHeader(&img);
	}
}
Mat* createMat(int height, int width, int type)
{
	Mat* arr = createMatHeader(height, width, type);
	createData(arr);

	return arr;
}
void releaseMat(Mat** array)
{
	if (!array)
		return;
	if (*array)
	{
		Mat* arr = *array;

		if (!CV_IS_MAT_HDR_Z(arr) && !CV_IS_MATND_HDR(arr))
			return;

		*array = 0;

		decRefData(arr);
		ifree(&arr);
	}
}
Seq *createSeq(int seq_flags, size_t header_size, size_t elem_size, MemStorage* storage)
{
	Seq *seq = 0;

	if (!storage)
		return 0;
	if (header_size < sizeof(Seq) || elem_size <= 0)
		return 0;

	/* allocate sequence header */
	seq = (Seq*)memStorageAlloc(storage, header_size);
	memset(seq, 0, header_size);

	seq->header_size = (int)header_size;
	seq->flags = (seq_flags & ~CV_MAGIC_MASK) | CV_SEQ_MAGIC_VAL;
	{
		int elemtype = CV_MAT_TYPE(seq_flags);
		int typesize = CV_ELEM_SIZE(elemtype);

		if (elemtype != CV_SEQ_ELTYPE_GENERIC && elemtype != CV_USRTYPE1 &&
			typesize != 0 && typesize != (int)elem_size)
			return 0;
	}
	seq->elem_size = (int)elem_size;
	seq->storage = storage;

	setSeqBlockSize(seq, (int)((1 << 10) / elem_size));

	return seq;
}
char* seqPush(Seq *seq, const void *element)
{
	char *ptr = 0;
	size_t elem_size;

	if (!seq)
		return 0;

	elem_size = seq->elem_size;
	ptr = seq->ptr;

	if (ptr >= seq->block_max)
	{
		iGrowSeq(seq, 0);

		ptr = seq->ptr;
		if (ptr + elem_size > seq->block_max)
		{
			return 0;
		}
	}

	if (element)
		memcpy(ptr, element, elem_size);
	seq->first->prev->count++;
	seq->total++;
	seq->ptr = ptr + elem_size;

	return ptr;
}
void setSeqBlockSize(Seq *seq, int delta_elements)
{
	int elem_size;
	int useful_block_size;

	if (!seq || !seq->storage)
		return;
	if (delta_elements < 0)
		return;

	useful_block_size = lAlignLeft(seq->storage->block_size - sizeof(MemBlock) - sizeof(SeqBlock), CV_STRUCT_ALIGN);
	elem_size = seq->elem_size;

	if (delta_elements == 0)
	{
		delta_elements = (1 << 10) / elem_size;
		delta_elements = MAX(delta_elements, 1);
	}
	if (delta_elements * elem_size > useful_block_size)
	{
		delta_elements = useful_block_size / elem_size;
		if (delta_elements == 0)
			return;
	}

	seq->delta_elems = delta_elements;
}
IplImage* cloneImage(const IplImage* src)
{
	IplImage* dst = 0;

	if (!CV_IS_IMAGE_HDR(src))
		return 0;

	if (!IPL.cloneImage)
	{
		dst = (IplImage*)ialloc(sizeof(*dst));

		memcpy(dst, src, sizeof(*src));
		dst->imageData = dst->imageDataOrigin = 0;
		dst->roi = 0;

		if (src->roi)
		{
			dst->roi = iCreateROI(src->roi->coi, src->roi->xOffset,
				src->roi->yOffset, src->roi->width, src->roi->height);
		}

		if (src->imageData)
		{
			int size = src->imageSize;
			createData(dst);
			memcpy(dst->imageData, src->imageData, size);
		}
	}
	else
		dst = IPL.cloneImage(src);

	return dst;
}
void seqPopFront(Seq *seq, void *element)
{
	int elem_size;
	SeqBlock *block;
	if (!seq)
		return;
	if (seq->total <= 0)
		return;

	elem_size = seq->elem_size;
	block = seq->first;

	if (element)
		memcpy(element, block->data, elem_size);
	block->data += elem_size;
	block->start_index++;
	seq->total--;

	if (--(block->count) == 0)
		iFreeSeqBlock(seq, 1);
}
void resetImageROI(IplImage* image)
{
	if (!image)
		return;

	if (image->roi)
	{
		if (!IPL.deallocate)
		{
			ifree(&image->roi);
		}
		else
		{
			IPL.deallocate(image, IPL_IMAGE_ROI);
			image->roi = 0;
		}
	}
}
Point ipoint(int x, int y)
{
	Point p;
	p.x = x;
	p.y = y;
	return p;
}
Rect irect(int x, int y, int width, int height)
{
	Rect r;

	r.x = x;
	r.y = y;
	r.width = width;
	r.height = height;

	return r;
}
Size isize(int width, int height)
{
	Size s;

	s.width = width;
	s.height = height;

	return s;
}

//建立长度为n的一维高斯卷积核
void CreatGauss(double sigma, double *cf, int n)
{
	double scale2X = -0.5 / (sigma*sigma);
	double sum = 0;

	int i;
	for (i = 0; i < n; i++)
	{
		double x = i - (n - 1)*0.5;
		double t = exp(scale2X*x*x);
		{
			cf[i] = t;
			sum += cf[i];
			
		}
	}
	

	sum = 1.0 / sum;
	for (i = 0; i < n; i++)
	{
		cf[i] = (cf[i] * sum);
		printf("%f ", cf[i]);
	}
	printf("\n");
}
//用高斯滤波器平滑原图像  
void GaussianSmooth(const IplImage* src, IplImage* dst, double sigma)
{
	float* pGray = (float*)src->imageData;
	float* pResult = (float*)dst->imageData;
	Size sz=isize(src->width,src->height);
	int  x, y, i;

	double *pdKernel;			//一维高斯滤波器  
	double *pdTemp;				//用于保存X方向滤波后的数据
	double dDotMul;				//高斯系数与图像数据的点乘  
	double dWeightSum = 0.0;	//滤波系数总和  
	int nWindowSize = iround(sigma * 4 * 2 + 1) | 1;//高斯滤波器长度  
	int nLen = nWindowSize / 2;						//窗口长度  

	if ((pdTemp = (double *)malloc(sz.width*sz.height*sizeof(double))) == NULL)
	{
		return;
	}
	if ((pdKernel = (double *)malloc(nWindowSize*sizeof(double))) == NULL)
	{
		return;
	}
	
	CreatGauss(sigma, pdKernel, nWindowSize);//产生一维高斯数据  
	
	for (i = 0; i < nWindowSize; ++i)//计算高斯核的和
		dWeightSum += pdKernel[i];

	for (y = 0; y<sz.height; y++)//x方向滤波  
	{
		for (x = 0; x<sz.width; x++)
		{
			dDotMul = 0;
			for (i = (-nLen); i <= nLen; i++)
			{
				if ((i + x) >= 0 && (i + x)<sz.width)//判断是否在图像内部  
				{
					dDotMul += (double)(pGray[y*sz.width + (i + x)] * pdKernel[nLen + i]);
				}
			}
			pdTemp[y*sz.width + x] = (float)(dDotMul / dWeightSum);
		}
	}

	for (x = 0; x < sz.width; x++)//y方向滤波 
	{
		for (y = 0; y < sz.height; y++)
		{
			dDotMul = 0;
			for (i = (-nLen); i <= nLen; i++)
			{
				if ((i + y) >= 0 && (i + y) < sz.height)
				{
					dDotMul += (double)pdTemp[(y + i)*sz.width + x] * pdKernel[nLen + i];
				}
			}
			pResult[y*sz.width + x] = (float)(dDotMul / dWeightSum);
		}
	}
	free(pdTemp);
	free(pdKernel);
}

//---------------------------------------------------------------------------------------------
Slice  islice(int start, int end)
{
	Slice slice;
	slice.start_index = start;
	slice.end_index = end;

	return slice;
}