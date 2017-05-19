#include "header.h"

static int parent( int i )
{
  return ( i - 1 ) / 2;
}

static int right( int i )
{
  return 2 * i + 2;
}

static int left( int i )
{
  return 2 * i + 1;
}

MinPq* minpq_init()
{
	MinPq* minpq;

	minpq = malloc(sizeof(MinPq));
  minpq->pq_array = calloc(MINPQ_INIT_NALLOCD, sizeof(PqNode));
  minpq->nallocd = MINPQ_INIT_NALLOCD;
  minpq->n = 0;

  return minpq;
}

int array_double(void** array, int n, int size)
{
	void* tmp;

	tmp = realloc(*array, 2 * n * size);
	if (!tmp)
	{
		if (*array)
			free(*array);
		*array = NULL;
		return 0;
	}
	*array = tmp;
	return n * 2;
}
int minpq_insert(MinPq* minpq, void* data, int key)
{
  int n = minpq->n;

  /* double array allocation if necessary */
  if( minpq->nallocd == n )
  {
	  minpq->nallocd = array_double((void**)&minpq->pq_array, minpq->nallocd, sizeof(PqNode));
      if( ! minpq->nallocd )
	  {
		  return 1;
	  }
  }

  minpq->pq_array[n].data = data;
  minpq->pq_array[n].key = INT_MAX;
  decrease_pq_node_key( minpq->pq_array, minpq->n, key );
  minpq->n++;

  return 0;
}

void* minpq_extract_min(MinPq* minpq)
{
  void* data;

  if( minpq->n < 1 )
    {
      //fprintf( stderr, "Warning: PQ empty, %s line %d\n", __FILE__, __LINE__ );
      return NULL;
    }
  data = minpq->pq_array[0].data;
  minpq->n--;
  minpq->pq_array[0] = minpq->pq_array[minpq->n];
  restore_minpq_order( minpq->pq_array, 0, minpq->n );

  return data;
}

void minpq_release(MinPq** minpq)
{
	if (!minpq)
	{
		return;
	}
	if (*minpq && (*minpq)->pq_array)
	{
		free((*minpq)->pq_array);
		free(*minpq);
		*minpq = NULL;
	}
}

static void decrease_pq_node_key(PqNode* pq_array, int i, int key)
{
	PqNode tmp;

	if (key > pq_array[i].key)
		return;

	pq_array[i].key = key;
	while (i > 0 && pq_array[i].key < pq_array[parent(i)].key)
	{
		tmp = pq_array[parent(i)];
		pq_array[parent(i)] = pq_array[i];
		pq_array[i] = tmp;
		i = parent(i);
	}
}

static void restore_minpq_order(PqNode* pq_array, int i, int n)
{
	PqNode tmp;
	int l, r, min = i;

	l = left(i);
	r = right(i);
	if (l < n)
		if (pq_array[l].key < pq_array[i].key)
			min = l;
	if (r < n)
		if (pq_array[r].key < pq_array[min].key)
			min = r;

	if (min != i)
	{
		tmp = pq_array[min];
		pq_array[min] = pq_array[i];
		pq_array[i] = tmp;
		restore_minpq_order(pq_array, min, n);
	}
}