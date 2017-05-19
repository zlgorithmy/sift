#include "header.h"

void match()
{
	IplImage* img1, *img2, *stacked;
	Feature* feat1, *feat2, *feat;
	Feature** nbrs;
	KdNode* kd_root;
	Point pt1, pt2;
	double d0, d1;
	char* bmp1, *bmp2;
	int n1, n2, k, i, m = 0;
	img1 = loadImage("img/3.bmp");
	img2 = loadImage("img/33.bmp");
	//stacked = stack_imgs(img1, img2);
	
	n1 = sift_features(img1, &feat1);
	n2 = sift_features(img2, &feat2);

	kd_root = kdtree_build(feat2, n2);
	for (i = 0; i < n1; i++)
	{
		feat = feat1 + i;
		k = kdtree_bbf_knn(kd_root, feat, 2, &nbrs, KDTREE_BBF_MAX_NN_CHKS);
		if (k == 2)
		{
			d0 = descr_dist_sq(feat, nbrs[0]);
			d1 = descr_dist_sq(feat, nbrs[1]);
			if (d0 < d1 * NN_SQ_DIST_RATIO_THR)
			{
				pt1 = ipoint(iround(feat->x), iround(feat->y));
				pt2 = ipoint(iround(nbrs[0]->x), iround(nbrs[0]->y));
				pt2.y += img1->height;
				//cvLine(stacked, pt1, pt2, CV_RGB(255, 0, 255), 1, 8, 0);
				m++;
				feat1[i].fwd_match = nbrs[0];
			}
		}
		free(nbrs);
	}
	printf("n1=%d n2=%d mt = %d\n\n", n1, n2, m);

	//cvShowImage("s",stacked);
	//cvWaitKey(0);
	//releaseImage(&stacked);
	releaseImage(&img1);
	releaseImage(&img2);
	kdtree_release(kd_root);
	free(feat1);
	free(feat2);
}
int main(int argc, char** argv)
{
	match();
	while (1);
	return 0;
}
