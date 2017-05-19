#ifndef _DEF_H_
#define _DEF_H_


#define KDTREE_BBF_MAX_NN_CHKS			100			/* the maximum number of keypoint NN candidates to check during BBF search */
#define NN_SQ_DIST_RATIO_THR			0.49		/* threshold on squared ratio of distances between NN and 2nd NN */
/******************************* Defs and macros *****************************/

#define SIFT_INTVLS						3			/** default number of sampled intervals per octave */
#define SIFT_SIGMA						1.6			/** default sigma for initial gaussian smoothing */
#define SIFT_CONTR_THR					0.04		/** default threshold on keypoint contrast |D(x)| */
#define SIFT_CURV_THR					10			/** default threshold on keypoint ratio of principle curvatures */
#define SIFT_IMG_DBL					1			/** double image size before pyramid construction? */
#define SIFT_DESCR_WIDTH				4			/** default width of descriptor histogram array */
#define SIFT_DESCR_HIST_BINS			8			/** default number of bins per histogram in descriptor array */
#define SIFT_INIT_SIGMA					0.5			/* assumed gaussian blur for input image */
#define SIFT_IMG_BORDER					5			/* width of border in which to ignore keypoints */
#define SIFT_MAX_INTERP_STEPS			5			/* maximum steps of keypoint interpolation before failure */
#define SIFT_ORI_HIST_BINS				36			/* default number of bins in histogram for orientation assignment */
#define SIFT_ORI_SIG_FCTR				1.5			/* determines gaussian sigma for orientation assignment */
#define SIFT_ORI_RADIUS					3.0 * SIFT_ORI_SIG_FCTR	/* determines the radius of the region used in orientation assignment */
#define SIFT_ORI_SMOOTH_PASSES			2			/* number of passes of orientation histogram smoothing */
#define SIFT_ORI_PEAK_RATIO				0.8			/* orientation magnitude relative to max that results in new feature */
#define SIFT_DESCR_SCL_FCTR				3.0			/* determines the size of a single descriptor orientation histogram */
#define SIFT_DESCR_MAG_THR				0.2			/* threshold on magnitude of elements of descriptor vector */
#define SIFT_INT_DESCR_FCTR				512.0		/* factor used to convert floating-point descriptor to unsigned char */

#define MINPQ_INIT_NALLOCD				512
#define FEATURE_MAX_D					128			/** max feature descriptor length */
#define INT_MAX							2147483647  /* maximum (signed) int value */
#define IPL_DEPTH_8U					8
#define IPL_DEPTH_32F					32
#define CV_WHOLE_SEQ_END_INDEX			0x3fffffff
#define CV_AUTOSTEP						0x7fffffff
#define CV_WHOLE_SEQ_END_INDEX			0x3fffffff
#define IPL_DEPTH_32F					32
#define ICV_SHIFT_TAB_MAX				32

#define IPL_DEPTH_SIGN					0x80000000

#define IPL_DEPTH_1U					1
#define IPL_DEPTH_8U					8
#define IPL_DEPTH_16U					16

#define IPL_DEPTH_8S					(IPL_DEPTH_SIGN| 8)
#define IPL_DEPTH_16S					(IPL_DEPTH_SIGN|16)
#define IPL_DEPTH_32S					(IPL_DEPTH_SIGN|32)
#define IPL_ORIGIN_TL					0
#define IPL_DATA_ORDER_PIXEL			0

#define  CV_MALLOC_ALIGN				16
#define  CV_STORAGE_BLOCK_SIZE			((1<<16) - 128)
#define  CV_STRUCT_ALIGN				((int)sizeof(double))
#define  CV_ORIGIN_TL					0
#define  CV_ORIGIN_BL					1
#define  CV_DEFAULT_IMAGE_ROW_ALIGN		4

#define CV_CN_MAX						512
#define CV_CN_SHIFT						3
#define CV_DEPTH_MAX					(1 << CV_CN_SHIFT)

#define CV_32F							5
#define CV_64F							6
#define CV_USRTYPE1						7

#define CV_MAT_DEPTH_MASK				(CV_DEPTH_MAX - 1)

#define CV_32FC1						CV_MAKETYPE(CV_32F,1)
#define CV_64FC1						CV_MAKETYPE(CV_64F,1)

#define CV_MAT_CONT_FLAG_SHIFT			14
#define CV_MAT_CONT_FLAG				(1 << CV_MAT_CONT_FLAG_SHIFT)
#define CV_MAT_CN_MASK					((CV_CN_MAX - 1) << CV_CN_SHIFT)

#define CV_MAT_TYPE_MASK				(CV_DEPTH_MAX*CV_CN_MAX - 1)

#define PI								3.1415926535897932384626433832795

#define IPL_IMAGE_HEADER				1
#define IPL_IMAGE_DATA					2
#define IPL_IMAGE_ROI					4

#define IPL_DEPTH_64F					64
#define CV_MAGIC_MASK					0xFFFF0000
#define CV_MAT_MAGIC_VAL				0x42420000
#define CV_MATND_MAGIC_VAL				0x42430000
#define CV_MAX_DIM						32

#define CV_STORAGE_MAGIC_VAL			0x42890000
#define CV_SEQ_MAGIC_VAL				0x42990000
#define CV_SEQ_ELTYPE_GENERIC			0
#define DBL_MAX							1.7976931348623158e+308 /* max value */

#define CV_MAT_TYPE(flags)				((flags) & CV_MAT_TYPE_MASK)
#define CV_IS_MAT_CONT(flags)			((flags) & CV_MAT_CONT_FLAG)
#define CV_MAT_DEPTH(flags)				((flags) & CV_MAT_DEPTH_MASK)
#define CV_MAKETYPE(depth,cn)			(CV_MAT_DEPTH(depth) + (((cn)-1) << CV_CN_SHIFT))
#define CV_GET_LAST_ELEM(seq, block)	((block)->data + ((block)->count - 1)*((seq)->elem_size))
#define CV_MAT_CN(flags)				((((flags) & CV_MAT_CN_MASK) >> CV_CN_SHIFT) + 1)
#define CV_IS_MATND_HDR(mat)			((mat) != NULL && (((const MatND*)(mat))->type & CV_MAGIC_MASK) == CV_MATND_MAGIC_VAL)
#define CV_IS_MATND(mat)				(CV_IS_MATND_HDR(mat) && ((const MatND*)(mat))->data != NULL)
#define feat_detection_data(f)			((DetectionData*)(f->feature_data))
#define ABS(x)							(((x) < 0) ? -(x) : (x))
#define MIN(a,b)						((a) > (b) ? (b) : (a))
#define MAX(a,b)						((a) < (b) ? (b) : (a))
#define ICV_FREE_PTR(storage)			((char*)(storage)->top + (storage)->block_size - (storage)->free_space)
#define CV_ELEM_SIZE(type)				(CV_MAT_CN(type) << ((((sizeof(size_t)/4+1)*16384|0x3a50) >> CV_MAT_DEPTH(type)*2) & 3))
#define CV_IS_IMAGE_HDR(img)			((img) != NULL && ((const IplImage*)(img))->nSize == sizeof(IplImage))
#define CV_IS_SEQ(seq)					((seq) != NULL && (((Seq*)(seq))->flags & CV_MAGIC_MASK) == CV_SEQ_MAGIC_VAL)
#define CV_IS_MAT(mat)					(CV_IS_MAT_HDR(mat) && ((const Mat*)(mat))->data.ptr != NULL)

#define interp_hist_peak( l, c, r )		(0.5 * ((l)-(r)) / ((l) - 2.0*(c) + (r)))
#define CV_WHOLE_SEQ					islice(0, CV_WHOLE_SEQ_END_INDEX)
#define ICV_ALIGNED_SEQ_BLOCK_SIZE		(int)iAlign(sizeof(SeqBlock), CV_STRUCT_ALIGN)
#define ifree(ptr)						(ifree_(*(ptr)), *(ptr)=0)

#define CV_SWAP_ELEMS(a,b,elem_size)						\
{															\
    int k;													\
    for( k = 0; k < elem_size; k++ )						\
	{														\
        char t0 = (a)[k];									\
        char t1 = (b)[k];									\
        (a)[k] = t1;										\
        (b)[k] = t0;										\
	}														\
}

#define CV_NEXT_SEQ_ELEM(elem_size, reader)					\
{                                                           \
    if(((reader).ptr += (elem_size)) >= (reader).block_max) \
	{                                                       \
		changeSeqBlock( &(reader), 1 );                     \
	}                                                       \
}

#define CV_RESTORE_READER_POS( reader, pos )				\
{															\
    (reader).block = (pos).block;							\
    (reader).ptr = (pos).ptr;								\
    (reader).block_min = (pos).block_min;					\
    (reader).block_max = (pos).block_max;					\
}

#define CV_SAVE_READER_POS( reader, pos )					\
{															\
    (pos).block = (reader).block;							\
    (pos).ptr = (reader).ptr;								\
    (pos).block_min = (reader).block_min;					\
    (pos).block_max = (reader).block_max;					\
}

#define CV_PREV_SEQ_ELEM( elem_size, reader )               \
{                                                           \
    if( ((reader).ptr -= (elem_size)) < (reader).block_min )\
	{                                                       \
		  changeSeqBlock( &(reader), -1 );                  \
	}                                                       \
}

#define CV_IS_MAT_HDR(mat)									\
    ((mat) != NULL &&										\
    (((const Mat*)(mat))->type & CV_MAGIC_MASK) == CV_MAT_MAGIC_VAL && \
    ((const Mat*)(mat))->cols > 0 && ((const Mat*)(mat))->rows > 0)

#define CV_IS_MAT_HDR_Z(mat)								\
    ((mat) != NULL && \
    (((const Mat*)(mat))->type & CV_MAGIC_MASK) == CV_MAT_MAGIC_VAL && \
    ((const Mat*)(mat))->cols >= 0 && ((const Mat*)(mat))->rows >= 0)
#endif