#ifndef _HEADER_H_
#define _HEADER_H_
#include "type.h"
#include <stdio.h>
#include <stdlib.h>

static const char icvPower2ShiftTab[] =
{
	0, 1, -1, 2, -1, -1, -1, 3, -1, -1, -1, -1, -1, -1, -1, 4,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 5
};

//sift features
extern int sift_features(IplImage* img, Feature** feat);
extern IplImage* stack_imgs(IplImage* img1, IplImage* img2);

static int _sift_features(IplImage* img, Feature** feat, int intvls, double sigma, double contr_thr, int curv_thr, int img_dbl, int descr_width, int descr_hist_bins);
static float pixval32f(IplImage* img, int r, int c);
static IplImage* create_init_img(IplImage*, int, double);
static IplImage* convert_to_gray32(IplImage*);
static IplImage*** build_gauss_pyr(IplImage*, int, int, double);
static IplImage* downsample(IplImage*);
static IplImage*** build_dog_pyr(IplImage***, int, int);
static Seq* scale_space_extrema(IplImage***, int, int, double, int, MemStorage*);
static int is_extremum(IplImage***, int, int, int, int);
static Feature* interp_extremum(IplImage***, int, int, int, int, int, double);
static void interp_step(IplImage***, int, int, int, int, double*, double*, double*);
static Mat* deriv_3D(IplImage***, int, int, int, int);
static Mat* hessian_3D(IplImage***, int, int, int, int);
static double interp_contr(IplImage***, int, int, int, int, double, double, double);
static Feature* new_feature(void);
static int is_too_edge_like(IplImage*, int, int, int);
static void calc_feature_scales(Seq*, double, int);
static void adjust_for_img_dbl(Seq*);
static void calc_feature_oris(Seq*, IplImage***);
static double* ori_hist(IplImage*, int, int, int, int, double);
static int calc_grad_mag_ori(IplImage*, int, int, double*, double*);
static void smooth_ori_hist(double*, int);
static double dominant_ori(double*, int);
static void add_good_ori_features(Seq*, double*, int, double, Feature*);
static Feature* clone_feature(Feature*);
static void compute_descriptors(Seq*, IplImage***, int, int);
static double*** descr_hist(IplImage*, int, int, double, double, int, int);
static void interp_hist_entry(double***, double, double, double, double, int, int);
static void hist_to_descr(double***, int, int, Feature*);
static void normalize_descr(Feature*);
static int feature_cmp(void*, void*, void*);
static void release_descr_hist(double****, int);
static void release_pyr(IplImage****, int, int);

//minpq
extern MinPq* minpq_init();
extern int minpq_insert(MinPq* minpq, void* data, int key);
extern void* minpq_extract_min(MinPq* minpq);
extern void minpq_release(MinPq** minpq);

static void restore_minpq_order(PqNode*, int, int);
static void decrease_pq_node_key(PqNode*, int, int);

//kdtree
extern KdNode* kdtree_build(Feature* features, int n);
extern int kdtree_bbf_knn(KdNode* kd_root, Feature* feat, int k, Feature*** nbrs, int max_nn_chks);
extern void kdtree_release(KdNode* kd_root);
extern double descr_dist_sq(Feature* f1, Feature* f2);

static KdNode* kd_node_init(Feature*, int);
static void expand_kd_node_subtree(KdNode*);
static void assign_part_key(KdNode*);
static double median_select(double*, int);
static double rank_select(double*, int, int);
static void insertion_sort(double*, int);
static int partition_array(double*, int, double);
static void partition_features(KdNode*);
static KdNode* explore_to_leaf(KdNode*, Feature*, MinPq*);
static int insert_into_nbr_array(Feature*, Feature**, int, int);

//img相关
ClImage* clLoadImage(const char* path);											//从文件读取BMP格式图像
void bmp2Iplimg(const ClImage* bmpImg, IplImage* iplImage);					//BMP图像转成IplImage格式
IplImage* loadImage(const char* path);										//从文件读取BMP图像
int invert(Mat* src, Mat* dst, int n);							//高斯消去法求逆矩阵
void imgSub(IplImage* src1, IplImage* src2, IplImage* dst);	//cvSub
void resizeImg(IplImage* src, IplImage* dst);								//图像缩放
IplImage* rgb2gray(const IplImage* src);									//RGB转单通道灰度图
int iround(double value);
void convertScale(IplImage* gray8, IplImage* gray32, float alpha);
void mSet(Mat* mat, int row, int col, double value);
char* getSeqElem(const Seq *seq, int index);
void GEMM(const Mat* src1, const Mat* src2, double alpha, Mat* dst0, int tABC);
int iceil(double value);
int ifloor(double value);
Point ipoint(int x, int y);
Rect  irect(int x, int y, int width, int height);
Size  isize(int width, int height);

//cv.c
void ifree_(void* ptr);

Mat* initMatHeader(Mat* mat, int rows, int cols, int type, void* data, int step);
MemStorage* createMemStorage(int block_size);
Seq *createSeq(int seq_flags, size_t header_size, size_t elem_size, MemStorage* storage);
void seqSort(Seq* seq, CmpFunc func, void* userdata);
void* tSeqToArray(const Seq* seq, void* elements, Slice slice);
void releaseMemStorage(MemStorage** storage);
IplImage* createImage(Size size, int depth, int channels);
void setZero(void* arr);
void setImageROI(IplImage* image, Rect rect);
void releaseImage(IplImage** image);
Mat* createMat(int rows, int cols, int type);
IplImage* cloneImage(const IplImage* image);
void seqPopFront(Seq* seq, void* element);
void resetImageROI(IplImage* image);
void setSeqBlockSize(Seq *seq, int delta_elements);
void GaussianSmooth(const IplImage* pGray, IplImage* pResult, double sigma);
Slice islice(int start, int end);
#endif
