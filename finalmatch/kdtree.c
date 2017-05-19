#include "header.h"

KdNode* kdtree_build(Feature* features, int n)
{
	KdNode* kd_root;

	if (!features || n <= 0)
	{
		return NULL;
	}

	kd_root = kd_node_init(features, n);
	expand_kd_node_subtree(kd_root);

	return kd_root;
}
int kdtree_bbf_knn(KdNode* kd_root, Feature* feat, int k, Feature*** nbrs, int max_nn_chks)
{
	KdNode* expl;
	MinPq* minpq;
	Feature* tree_feat, ** _nbrs;
	BbfData* bbfdata;
	int i, t = 0, n = 0;

	if (!nbrs || !feat || !kd_root)
	{
		return -1;
	}

	_nbrs = calloc(k, sizeof(Feature*));
	minpq = minpq_init();
	minpq_insert(minpq, kd_root, 0);
	while (minpq->n > 0 && t < max_nn_chks)
	{
		expl = (KdNode*)minpq_extract_min(minpq);
		if (!expl)
		{
			goto fail;
		}

		expl = explore_to_leaf(expl, feat, minpq);
		if (!expl)
		{
			goto fail;
		}

		for (i = 0; i < expl->n; i++)
		{
			tree_feat = &expl->features[i];
			bbfdata = malloc(sizeof(BbfData));
			if (!bbfdata)
			{
				goto fail;
			}
			bbfdata->oldData = tree_feat->feature_data;
			bbfdata->d = descr_dist_sq(feat, tree_feat);
			tree_feat->feature_data = bbfdata;
			n += insert_into_nbr_array(tree_feat, _nbrs, n, k);
		}
		t++;
	}

	minpq_release(&minpq);
	for (i = 0; i < n; i++)
	{
		bbfdata = _nbrs[i]->feature_data;
		_nbrs[i]->feature_data = bbfdata->oldData;
		free(bbfdata);
	}
	*nbrs = _nbrs;
	return n;

fail:
	minpq_release(&minpq);
	for (i = 0; i < n; i++)
	{
		bbfdata = _nbrs[i]->feature_data;
		_nbrs[i]->feature_data = bbfdata->oldData;
		free(bbfdata);
	}
	free(_nbrs);
	*nbrs = NULL;
	return -1;
}
void kdtree_release( KdNode* kd_root )
{
  if( ! kd_root )
    return;
  kdtree_release( kd_root->kd_left );
  kdtree_release( kd_root->kd_right );
  free( kd_root );
}

static KdNode* kd_node_init(Feature* features, int n)
{
  KdNode* kdnode;

  kdnode = malloc(sizeof(KdNode));
  memset(kdnode, 0, sizeof(KdNode));
  kdnode->ki = -1;
  kdnode->features = features;
  kdnode->n = n;

  return kdnode;
}

static void expand_kd_node_subtree(KdNode* kdnode)
{
  /* base case: leaf node */
  if( kdnode->n == 1  ||  kdnode->n == 0 )
    {
      kdnode->leaf = 1;
      return;
    }

  assign_part_key( kdnode );
  partition_features( kdnode );

  if( kdnode->kd_left )
    expand_kd_node_subtree( kdnode->kd_left );
  if( kdnode->kd_right )
    expand_kd_node_subtree( kdnode->kd_right );
}

static void assign_part_key(KdNode* kdnode)
{
	Feature* features;
  double kv, x, mean, var, var_max = 0;
  double* tmp;
  int d, n, i, j, ki = 0;

  features = kdnode->features;
  n = kdnode->n;
  d = features[0].d;

  /* partition key index is that along which descriptors have most variance */
  for( j = 0; j < d; j++ )
    {
      mean = var = 0;
      for( i = 0; i < n; i++ )
	mean += features[i].descr[j];
      mean /= n;
      for( i = 0; i < n; i++ )
	{
	  x = features[i].descr[j] - mean;
	  var += x * x;
	}
      var /= n;

      if( var > var_max )
	{
	  ki = j;
	  var_max = var;
	}
    }

  /* partition key value is median of descriptor values at ki */
  tmp = calloc( n, sizeof( double ) );
  for( i = 0; i < n; i++ )
    tmp[i] = features[i].descr[ki];
  kv = median_select( tmp, n );
  free( tmp );

  kdnode->ki = ki;
  kdnode->kv = kv;
}

static double median_select( double* array, int n )
{
  return rank_select( array, n, (n - 1) / 2 );
}

static double rank_select( double* array, int n, int r )
{
  double* tmp, med;
  int gr_5, gr_tot, rem_elts, i, j;

  /* base case */
  if( n == 1 )
    return array[0];

  /* divide array into groups of 5 and sort them */
  gr_5 = n / 5;
  gr_tot = iceil( n / 5.0 );
  rem_elts = n % 5;
  tmp = array;
  for( i = 0; i < gr_5; i++ )
    {
      insertion_sort( tmp, 5 );
      tmp += 5;
    }
  insertion_sort( tmp, rem_elts );

  /* recursively find the median of the medians of the groups of 5 */
  tmp = calloc( gr_tot, sizeof( double ) );
  for( i = 0, j = 2; i < gr_5; i++, j += 5 )
    tmp[i] = array[j];
  if( rem_elts )
    tmp[i++] = array[n - 1 - rem_elts/2];
  med = rank_select( tmp, i, ( i - 1 ) / 2 );
  free( tmp );
  
  /* partition around median of medians and recursively select if necessary */
  j = partition_array( array, n, med );
  if( r == j )
    return med;
  else if( r < j )
    return rank_select( array, j, r );
  else
    {
      array += j+1;
      return rank_select( array, ( n - j - 1 ), ( r - j - 1 ) );
    }
}

static void insertion_sort( double* array, int n )
{
  double k;
  int i, j;

  for( i = 1; i < n; i++ )
    {
      k = array[i];
      j = i-1;
      while( j >= 0  &&  array[j] > k )
	{
	  array[j+1] = array[j];
	  j -= 1;
	}
      array[j+1] = k;
    }
}

static int partition_array( double* array, int n, double pivot )
{
  double tmp;
  int p = 0, i, j;

  i = -1;
  for( j = 0; j < n; j++ )
    if( array[j] <= pivot )
      {
	tmp = array[++i];
	array[i] = array[j];
	array[j] = tmp;
	if( array[i] == pivot )
	  p = i;
      }
  array[p] = array[i];
  array[i] = pivot;
  
  return i;
}

static void partition_features(KdNode* kdnode)
{
	Feature* features, tmp;
  double kv;
  int n, ki, p = 0, i, j = -1;

  features = kdnode->features;
  n = kdnode->n;
  ki = kdnode->ki;
  kv = kdnode->kv;
  for( i = 0; i < n; i++ )
    if( features[i].descr[ki] <= kv )
      {
	tmp = features[++j];
	features[j] = features[i];
	features[i] = tmp;
	if( features[j].descr[ki] == kv )
	  p = j;
      }
  tmp = features[p];
  features[p] = features[j];
  features[j] = tmp;

  /* if all records fall on same side of partition, make node a leaf */
  if( j == n - 1 )
    {
      kdnode->leaf = 1;
      return;
    }

  kdnode->kd_left = kd_node_init( features, j + 1 );
  kdnode->kd_right = kd_node_init( features + ( j + 1 ), ( n - j - 1 ) );
}
static KdNode* explore_to_leaf(KdNode* kdnode, Feature* feat, MinPq* minpq)
{
	KdNode* unexpl, *expl = kdnode;
  double kv;
  int ki;

  while( expl  &&  ! expl->leaf )
    {
      ki = expl->ki;
      kv = expl->kv;
      
      if( ki >= feat->d )
	{
	  return NULL;
	}
      if( feat->descr[ki] <= kv )
	{
	  unexpl = expl->kd_right;
	  expl = expl->kd_left;
	}
      else
	{
	  unexpl = expl->kd_left;
	  expl = expl->kd_right;
	}
      
      if( minpq_insert( minpq, unexpl, ABS( kv - feat->descr[ki] ) ) )
	{
	  return NULL;
	}
    }

  return expl;
}

static int insert_into_nbr_array(Feature* feat, Feature** nbrs, int n, int k)
{
	BbfData* fdata, *ndata;
  double dn, df;
  int i, ret = 0;

  if( n == 0 )
    {
      nbrs[0] = feat;
      return 1;
    }

  /* check at end of array */
  fdata = (BbfData*)feat->feature_data;
  df = fdata->d;
  ndata = (BbfData*)nbrs[n - 1]->feature_data;
  dn = ndata->d;
  if( df >= dn )
    {
      if( n == k )
	{
	  feat->feature_data = fdata->oldData;
	  free( fdata );
	  return 0;
	}
      nbrs[n] = feat;
      return 1;
    }

  /* find the right place in the array */
  if( n < k )
    {
      nbrs[n] = nbrs[n-1];
      ret = 1;
    }
  else
    {
      nbrs[n-1]->feature_data = ndata->oldData;
      free( ndata );
    }
  i = n-2;
  while( i >= 0 )
    {
		ndata = (BbfData*)nbrs[i]->feature_data;
      dn = ndata->d;
      if( dn <= df )
	break;
      nbrs[i+1] = nbrs[i];
      i--;
    }
  i++;
  nbrs[i] = feat;

  return ret;
}

double descr_dist_sq(Feature* f1, Feature* f2)
{
	double diff, dsq = 0;
	double* descr1, *descr2;
	int i, d;

	d = f1->d;
	if (f2->d != d)
		return DBL_MAX;
	descr1 = f1->descr;
	descr2 = f2->descr;

	for (i = 0; i < d; i++)
	{
		diff = descr1[i] - descr2[i];
		dsq += diff*diff;
	}
	return dsq;
}