#include <stdlib.h>
#include <stdio.h>
#include <omp.h>

#include "quicksort.h"

#define error(fmt, ...)                         \
do {                                            \
	fprintf(stderr, fmt, ##__VA_ARGS__);    \
	exit(1);                                \
} while (0)


#undef  SWAP
#define SWAP(_a, _b)               \
do {                               \
        __auto_type _tmp = *_a;    \
	*_a = *_b;                 \
	*_b = _tmp;                \
} while (0)



/* compare(a, b)
 *
 *  1 -> swapped positions (b, a)
 * -1 -> as is (a, b)
 *  0 -> random (equality)
 *
 * increasing order:
 *	(a > b) ?  1 : (a < b) ? -1 : 0
 * decreasing order:
 *	(a > b) ? -1 : (a < b) ?  1 : 0
 */
//__attribute__((always_inline))
//inline

// --> moved to the header file!
//extern int quicksort_cmp(_TYPE_V a, _TYPE_V b, _TYPE_AD * aux_data);

//extern int filter_out(_TYPE_V e);

static inline
void
loop_partitioner_balance_iterations_base(long num_workers, long worker_pos, long start, long end, long incr,
		long * local_start_out, long * local_end_out, long * num_iterations_out)
{
	long sgn = (incr > 0) ? 1 : -1;
	long len = (end - start + incr - sgn) / incr;
	if (len < 0)
		error("infinite loop");
	if (sgn*incr > len)  // If |incr| >= len then worker 0 gets all the work.
	{
		if (worker_pos == 0)
		{
			*local_start_out = start;
			*local_end_out = end;
			*num_iterations_out = len;
		}
		else
		{
			*local_start_out = end;
			*local_end_out = end;
			*num_iterations_out = 0;
		}
		return;
	}
	long per_t_len = len / num_workers;
	long rem       = len % num_workers;
	long local_start, local_end;
	long num_iterations;
	if (rem != 0)
	{
		if (worker_pos < rem)
		{
			per_t_len += 1;
			rem = 0;
		}
	}
	local_start = start + incr * per_t_len * worker_pos + incr * rem;
	local_end   = local_start + incr * per_t_len;
	if (worker_pos == num_workers - 1)
		local_end = end;
	num_iterations = sgn * (local_end - local_start);

	*local_start_out = local_start;
	*local_end_out = local_end;
	*num_iterations_out = num_iterations;
}

//==========================================================================================================================================
//= Filter   
//==========================================================================================================================================


//------------------------------------------------------------------------------------------------------------------------------------------
//- Filter Serial   
//------------------------------------------------------------------------------------------------------------------------------------------

/* index i_end is inclusive
 */
__attribute__((always_inline))
static inline
long
filter_serial_base(_TYPE_V * A, long i_start, long i_end)
{
	//printf("tid#%d i_start=%ld i_end=%ld\n", omp_get_thread_num(), i_start, i_end);
	long i, end;

	// if (i_start == i_end)
		// return (quicksort_cmp(A[i_start], pivot, aux_data) < 0) ? i_start+1 : i_start;
	if (i_start > i_end)
		error("invalid bounds, i_start > i_end : %ld > %ld", i_start, i_end);

	end = i_start;

        //printf("tid#%d Working on [%ld,%ld]\n", omp_get_thread_num(), i_start, i_end);
	for ( i = i_start; i <= i_end; i++ )
	{
                //printf("A[%ld] = {%u,%u,%f}, filter_out = %d\n", i, A[i].vertex1, A[i].vertex2, A[i].weight, filter_out(A[i]));
		if ( !filter_out(A[i]) )
			A[end++] = A[i];
	}

	//printf("tid#%d end=%ld\n", omp_get_thread_num(), end);

	return end;
}

/* 'e' end index is exclusive.
 */
long
quicksort_filter_serial(_TYPE_V * A, long s, long e)
{
	return filter_serial_base(A, s, e-1);
}


//------------------------------------------------------------------------------------------------------------------------------------------
//- Filter Concurrent   
//------------------------------------------------------------------------------------------------------------------------------------------

__attribute__((always_inline))
static inline
long
filter_concurrent_inplace_base(_TYPE_V * A, long i_start, long i_end)
{
	__label__ partition_concurrent_out;
	int num_threads = omp_get_num_threads();
        //printf("%s omp_get_num_threads() = %d\n", __FUNCTION__, num_threads);
	int tnum = omp_get_thread_num();

        //printf("i_start=%ld, i_end=%ld\n", i_start, i_end);
	static int num_threads_prev = 0;
	static long g_l;
	static long g_h;
	static long g_realoc_num_l = 0;
	static long g_realoc_num_h = 0;

	static long * t_s = NULL, * t_e = NULL, * t_m = NULL;

	static long * t_l_s = NULL, * t_l_e = NULL;
	static long * t_h_s = NULL, * t_h_e = NULL;

	long i, i_s, i_e, j, m;


	if (i_start == i_end)
		//return (quicksort_cmp(A[i_start], pivot, aux_data) < 0) ? i_start+1 : i_start;
                return ( !filter_out(A[i_start]) ) ? i_start+1 : i_start;
	if (i_start > i_end)
		error("invalid bounds, i_start > i_end : %ld > %ld", i_start, i_end);
        //if (i_end-i_start <= 1000)
        //        return filter_serial_base(A, i_start, i_end);

	#pragma omp barrier

	#pragma omp single nowait
	{
		g_l = 0;
		g_h = 0;
		if (num_threads_prev < num_threads)
		{
			num_threads_prev = num_threads;
			t_l_s = (typeof(t_l_s)) malloc(num_threads * sizeof(*t_l_s));
			t_l_e = (typeof(t_l_e)) malloc(num_threads * sizeof(*t_l_e));
			t_h_s = (typeof(t_h_s)) malloc(num_threads * sizeof(*t_h_s));
			t_h_e = (typeof(t_h_e)) malloc(num_threads * sizeof(*t_h_e));

			t_s = (typeof(t_s)) malloc(num_threads * sizeof(*t_s));
			t_e = (typeof(t_e)) malloc(num_threads * sizeof(*t_e));
			t_m = (typeof(t_m)) malloc(num_threads * sizeof(*t_m));
		}
	}

	#pragma omp barrier

	i_end++;   // Make 'i_end' exclusive, to work with 'loop_partitioner_balance_iterations'.

        long num_iters;
	loop_partitioner_balance_iterations_base(num_threads, tnum, i_start, i_end, 1, &i_s, &i_e, &num_iters);
        //printf("Loop partitioner: i_s=%ld, i_e=%ld\n", i_s, i_e);
	if (i_s != i_e)
		m = filter_serial_base(A, i_s, i_e - 1);
	else
		m = i_s;
	if (m < i_s)
		error("test");
	if (m > i_e)
		error("test");

	__atomic_fetch_add(&g_l, m-i_s, __ATOMIC_RELAXED);
	__atomic_fetch_add(&g_h, i_e-m, __ATOMIC_RELAXED);
	t_s[tnum] = i_s;
	t_e[tnum] = i_e;
	t_m[tnum] = m;

	#pragma omp barrier

	#pragma omp single nowait
	{
		g_l += i_start;
		g_h += g_l;
	}

	#pragma omp barrier

	t_l_s[tnum] = t_l_e[tnum] = 0;
	t_h_s[tnum] = t_h_e[tnum] = 0;
	if (m > g_l)
	{
		t_l_s[tnum] = (i_s > g_l) ? i_s : g_l;
		t_l_e[tnum] = m;
	}
	else if (m < g_l)
	{
		t_h_s[tnum] = m;
		t_h_e[tnum] = (i_e < g_l) ? i_e : g_l;
	}

	#pragma omp barrier

	#pragma omp single nowait
	{
		g_realoc_num_l = 0;
		g_realoc_num_h = 0;
		for (i=0;i<num_threads;i++)
		{
			g_realoc_num_l += t_l_e[i] - t_l_s[i];
			g_realoc_num_h += t_h_e[i] - t_h_s[i];
		}

		if (g_realoc_num_l != g_realoc_num_h)
			error("low != high : %ld != %ld", g_realoc_num_l, g_realoc_num_h);

	}

	#pragma omp barrier

	long rel_pos_s=0, rel_pos_e=0;   // Positions if the chunks were concatenated.
	long realoc_num;
	long l_s=0, l_e=0;
	long h_s=0, h_e=0;
	long sum;
        //printf("tid#%d g_realoc_num_l=%ld\n", omp_get_thread_num(), g_realoc_num_l);
	loop_partitioner_balance_iterations_base(num_threads, tnum, 0, g_realoc_num_l, 1, &rel_pos_s, &rel_pos_e, &num_iters);
	realoc_num = rel_pos_e - rel_pos_s;
        //printf("tid#%d rel_pos_s=%ld, rel_pos_e=%ld\n", omp_get_thread_num(), rel_pos_s, rel_pos_e);

	if (realoc_num == 0)
		goto partition_concurrent_out;

	i = 0;
	sum = t_l_e[i] - t_l_s[i];
	while (sum <= rel_pos_s)
	{
		i++;
		sum += t_l_e[i] - t_l_s[i];
	}
	l_s = t_l_e[i] - (sum - rel_pos_s);
	l_e = t_l_e[i];

        //printf("tid#%d l_s=%ld, l_e=%ld\n", omp_get_thread_num(), l_s, l_e);

	j = 0;
	sum = t_h_e[j] - t_h_s[j];
	while (sum <= rel_pos_s)
	{
		j++;
		sum += t_h_e[j] - t_h_s[j];
	}
	h_s = t_h_e[j] - (sum - rel_pos_s);
	h_e = t_h_e[j];

        //printf("tid#%d h_s=%ld, h_e=%ld\n", omp_get_thread_num(), h_s, h_e);

	while (realoc_num > 0)
	{
		while (l_s == l_e)
		{
			i++;
			if (i >= num_threads)
				goto partition_concurrent_out;
			l_s = t_l_s[i];
			l_e = t_l_e[i];
			if (l_e - l_s > realoc_num)
				l_e = l_s + realoc_num;
		}
		while (h_s == h_e)
		{
			j++;
			if (j >= num_threads)
				goto partition_concurrent_out;
			h_s = t_h_s[j];
			h_e = t_h_e[j];
			if (h_e - h_s > realoc_num)
				h_e = h_s + realoc_num;
		}
		while (realoc_num > 0)
		{
			if ((l_s == l_e) || (h_s == h_e))
				break;
			//SWAP(&A[l_s], &A[h_s]);
			A[h_s] = A[l_s];
			l_s++;
			h_s++;
			realoc_num--;
		}
	}
	partition_concurrent_out:;

	#pragma omp barrier
	//printf("tid#%d g_l=%ld\n", tnum, g_l);

	return g_l;
}

long
quicksort_filter_concurrent_inplace(_TYPE_V * A, long i_start, long i_end)
{
	return filter_concurrent_inplace_base(A, i_start, i_end);
}



//==========================================================================================================================================
//= Partition
//==========================================================================================================================================


//------------------------------------------------------------------------------------------------------------------------------------------
//- Partition Serial
//------------------------------------------------------------------------------------------------------------------------------------------

__attribute__((always_inline))
static inline
long
partition_serial_base(_TYPE_V pivot, _TYPE_V * A, long i_start, long i_end, _TYPE_AD * aux_data)
{
	long i_s, i_e;

	// if (i_start == i_end)
		// return (quicksort_cmp(A[i_start], pivot, aux_data) < 0) ? i_start+1 : i_start;
	if (i_start > i_end)
		error("invalid bounds, i_start > i_end : %ld > %ld", i_start, i_end);

	i_s = i_start;
	i_e = i_end;

        //printf("tid#%d Working on [%ld,%ld]\n", omp_get_thread_num(), i_start, i_end);
	while (1)
	{
		while ((i_s < i_e) && (quicksort_cmp(A[i_s], pivot, aux_data) < 0))
			i_s++;
		while ((i_s < i_e) && (quicksort_cmp(A[i_e], pivot, aux_data) > 0))
			i_e--;
		if (i_s >= i_e)
			break;
                //printf("tid#%d SWAPping A[%ld], A[%ld]\n", omp_get_thread_num(), i_s, i_e);
		SWAP(&A[i_s], &A[i_e]);
		i_s++;
		i_e--;
	}

	// If i_s == i_e then this element has not been examined.
	// If all elements are lower than the pivot then return a position after the last one.
	if (quicksort_cmp(A[i_s], pivot, aux_data) < 0)
		i_s++;

	return i_s;
}

/* 'e' end index is inclusive.
 *
 * Expected partitioning outcome,
 * for s <= i <= e:
 *     i <  i_s  :  A[i] <= pivot
 *     i >= i_s  :  A[i] >= pivot
 */
__attribute__((always_inline))
static inline
long
partition_serial(_TYPE_V * A, long s, long e, _TYPE_AD * aux_data)
{
	_TYPE_V pivot;
	long pivot_pos;
	long i_s, i_e;
	long cmp, balance;

	if (s == e)
		return s+1;

	pivot_pos = (s+e)/2;    // A somewhat better pivot than simply the first element. Also works well when already sorted.
        //printf("%s: pivot pos = %ld\n", __FUNCTION__, pivot_pos);
        

	/* In order for the left part to be non-empty we always at least advance by a value equal to the pivot.
	 * In order for the right part to be non-empty the value at i_s always belongs to the right part, and is swapped at the end.
	 */
	SWAP(&A[s], &A[pivot_pos]);
	pivot = A[s];
	i_s = s+1;
	i_e = e;

	balance = 0;
	while (1)
	{
		while (i_s < i_e)
		{
			cmp = quicksort_cmp(A[i_s], pivot, aux_data);
			if ((cmp > 0) || (cmp == 0 && balance > 0))
				break;
			i_s++;
			balance++;
		}
		while (i_s < i_e)
		{
			cmp = quicksort_cmp(A[i_e], pivot, aux_data);
			if ((cmp < 0) || (cmp == 0 && balance < 0))
				break;
			i_e--;
			balance--;
		}
		if (i_s >= i_e)
			break;
		SWAP(&A[i_s], &A[i_e]);
		i_s++;
		i_e--;
	}

	/* Since the value at 'i_s' always belongs to the right part,
	 * if the pivot is the max value we have to swap with it for when there are only two values in the segment.
	 */
	if (quicksort_cmp(pivot, A[i_s], aux_data) > 0)
		SWAP(&A[s], &A[i_s]);

	return i_s;
}


/* 'e' end index is exclusive.
 */
long
quicksort_partition_serial(_TYPE_V * A, long s, long e, _TYPE_AD * aux_data)
{
	return partition_serial(A, s, e-1, aux_data);
}

long
quicksort_partition_serial_base(_TYPE_V pivot, _TYPE_V * A, long i_s, long i_e, _TYPE_AD * aux_data)
{
	return partition_serial_base(pivot, A, i_s, i_e-1, aux_data);
}


//------------------------------------------------------------------------------------------------------------------------------------------
//- Partition Concurrent
//------------------------------------------------------------------------------------------------------------------------------------------

__attribute__((always_inline))
static inline
long
partition_concurrent_inplace_base(_TYPE_V pivot, _TYPE_V * A, long i_start, long i_end, _TYPE_AD * aux_data)
{
	__label__ partition_concurrent_out;
	int num_threads = omp_get_num_threads();
        //printf("%s omp_get_num_threads() = %d\n", __FUNCTION__, num_threads);
	int tnum = omp_get_thread_num();

        //printf("i_start=%ld, i_end=%ld\n", i_start, i_end);
	static int num_threads_prev = 0;
	static long g_l;
	static long g_h;
	static long g_realoc_num_l = 0;
	static long g_realoc_num_h = 0;

	static long * t_s = NULL, * t_e = NULL, * t_m = NULL;

	static long * t_l_s = NULL, * t_l_e = NULL;
	static long * t_h_s = NULL, * t_h_e = NULL;

	long i, i_s, i_e, j, m;

	if (i_start == i_end)
		return (quicksort_cmp(A[i_start], pivot, aux_data) < 0) ? i_start+1 : i_start;
	if (i_start > i_end)
		error("invalid bounds, i_start > i_end : %ld > %ld", i_start, i_end);

	#pragma omp barrier

	#pragma omp single nowait
	{
		g_l = 0;
		g_h = 0;
		if (num_threads_prev < num_threads)
		{
			num_threads_prev = num_threads;
			t_l_s = (typeof(t_l_s)) malloc(num_threads * sizeof(*t_l_s));
			t_l_e = (typeof(t_l_e)) malloc(num_threads * sizeof(*t_l_e));
			t_h_s = (typeof(t_h_s)) malloc(num_threads * sizeof(*t_h_s));
			t_h_e = (typeof(t_h_e)) malloc(num_threads * sizeof(*t_h_e));

			t_s = (typeof(t_s)) malloc(num_threads * sizeof(*t_s));
			t_e = (typeof(t_e)) malloc(num_threads * sizeof(*t_e));
			t_m = (typeof(t_m)) malloc(num_threads * sizeof(*t_m));
		}
	}

	#pragma omp barrier

	i_end++;   // Make 'i_end' exclusive, to work with 'loop_partitioner_balance_iterations'.

        long num_iters;
	loop_partitioner_balance_iterations_base(num_threads, tnum, i_start, i_end, 1, &i_s, &i_e, &num_iters);
        //printf("Loop partitioner: i_s=%ld, i_e=%ld\n", i_s, i_e);
	if (i_s != i_e)
		m = partition_serial_base(pivot, A, i_s, i_e - 1, aux_data);
	else
		m = i_s;
	if (m < i_s)
		error("test");
	if (m > i_e)
		error("test");

	__atomic_fetch_add(&g_l, m-i_s, __ATOMIC_RELAXED);
	__atomic_fetch_add(&g_h, i_e-m, __ATOMIC_RELAXED);
	t_s[tnum] = i_s;
	t_e[tnum] = i_e;
	t_m[tnum] = m;

	#pragma omp barrier

	#pragma omp single nowait
	{
		g_l += i_start;
		g_h += g_l;
	}

	#pragma omp barrier

	t_l_s[tnum] = t_l_e[tnum] = 0;
	t_h_s[tnum] = t_h_e[tnum] = 0;
	if (m > g_l)
	{
		t_l_s[tnum] = (i_s > g_l) ? i_s : g_l;
		t_l_e[tnum] = m;
	}
	else if (m < g_l)
	{
		t_h_s[tnum] = m;
		t_h_e[tnum] = (i_e < g_l) ? i_e : g_l;
	}

	#pragma omp barrier

	#pragma omp single nowait
	{
		g_realoc_num_l = 0;
		g_realoc_num_h = 0;
		for (i=0;i<num_threads;i++)
		{
			g_realoc_num_l += t_l_e[i] - t_l_s[i];
			g_realoc_num_h += t_h_e[i] - t_h_s[i];
		}

		if (g_realoc_num_l != g_realoc_num_h)
			error("low != high : %ld != %ld", g_realoc_num_l, g_realoc_num_h);

	}

	#pragma omp barrier

	long rel_pos_s=0, rel_pos_e=0;   // Positions if the chunks were concatenated.
	long realoc_num;
	long l_s=0, l_e=0;
	long h_s=0, h_e=0;
	long sum;
        //printf("tid#%d g_realoc_num_l=%ld\n", omp_get_thread_num(), g_realoc_num_l);
	loop_partitioner_balance_iterations_base(num_threads, tnum, 0, g_realoc_num_l, 1, &rel_pos_s, &rel_pos_e, &num_iters);
	realoc_num = rel_pos_e - rel_pos_s;
        //printf("tid#%d rel_pos_s=%ld, rel_pos_e=%ld\n", omp_get_thread_num(), rel_pos_s, rel_pos_e);

	if (realoc_num == 0)
		goto partition_concurrent_out;

	i = 0;
	sum = t_l_e[i] - t_l_s[i];
	while (sum <= rel_pos_s)
	{
		i++;
		sum += t_l_e[i] - t_l_s[i];
	}
	l_s = t_l_e[i] - (sum - rel_pos_s);
	l_e = t_l_e[i];

        //printf("tid#%d l_s=%ld, l_e=%ld\n", omp_get_thread_num(), l_s, l_e);

	j = 0;
	sum = t_h_e[j] - t_h_s[j];
	while (sum <= rel_pos_s)
	{
		j++;
		sum += t_h_e[j] - t_h_s[j];
	}
	h_s = t_h_e[j] - (sum - rel_pos_s);
	h_e = t_h_e[j];

        //printf("tid#%d h_s=%ld, h_e=%ld\n", omp_get_thread_num(), h_s, h_e);

	while (realoc_num > 0)
	{
		while (l_s == l_e)
		{
			i++;
			if (i >= num_threads)
				goto partition_concurrent_out;
			l_s = t_l_s[i];
			l_e = t_l_e[i];
			if (l_e - l_s > realoc_num)
				l_e = l_s + realoc_num;
		}
		while (h_s == h_e)
		{
			j++;
			if (j >= num_threads)
				goto partition_concurrent_out;
			h_s = t_h_s[j];
			h_e = t_h_e[j];
			if (h_e - h_s > realoc_num)
				h_e = h_s + realoc_num;
		}
		while (realoc_num > 0)
		{
			if ((l_s == l_e) || (h_s == h_e))
				break;
			SWAP(&A[l_s], &A[h_s]);
			l_s++;
			h_s++;
			realoc_num--;
		}
	}
	partition_concurrent_out:;

	#pragma omp barrier

	return g_l;
}

/* 'i_end' end index is exclusive.
 */
__attribute__((always_inline))
static inline
void
partition_concurrent_non_equal(_TYPE_V pivot, _TYPE_V * A, _TYPE_V * buf, long i_start, long i_end, _TYPE_AD * aux_data, long * num_lower_out, long * num_equal_out, long * num_higher_out)
{
	long i, i_s, i_e;
	i_s = i_start;
	i_e = i_end;
	for (i=i_start;i<i_end;i++)
	{
		if (quicksort_cmp(A[i], pivot, aux_data) < 0)
			buf[i_s++] = A[i];
		else if (quicksort_cmp(A[i], pivot, aux_data) > 0)
			buf[--i_e] = A[i];
	}
	*num_lower_out = i_s - i_start;
	*num_higher_out = i_end - i_e;
	*num_equal_out = (i_end - i_start) - *num_lower_out - *num_higher_out;
}


/* 'i_end' end index is inclusive.
 *
 * Expected partitioning outcome,
 * for i_start <= i <= i_end:
 *     i <  m  :  buf[i] <= pivot
 *     i >= m  :  buf[i] >= pivot
 */
__attribute__((always_inline))
static inline
long
partition_concurrent(_TYPE_V * A, _TYPE_V * buf, long i_start, long i_end, _TYPE_AD * aux_data)
{
	int num_threads = omp_get_max_threads();
	int tnum = omp_get_thread_num();

	static long * t_lower_n;
	static long * t_higher_n;
	static long * t_equal_n;

	long lower_n, higher_n, equal_n;

	_TYPE_V pivot;
	long pivot_pos;
	long i, i_s, i_e, j;
	long l=0, h=0, e=0, el=0, eh=0, d, tmp;

	static long m = 0;

	if (i_start == i_end)
		return i_start + 1;

	pivot_pos = (i_start+i_end)/2;    // A somewhat better pivot than simply the first element. Also works well when already sorted.
        //printf("%s: pivot pos = %ld\n", __FUNCTION__, pivot_pos);
	pivot = A[pivot_pos];

	i_end++;   // Make 'i_end' exclusive, to work with 'loop_partitioner_balance_iterations'.

	#pragma omp single nowait
	{
		t_lower_n = (typeof(t_lower_n)) malloc(num_threads * sizeof(*t_lower_n));
		t_higher_n = (typeof(t_higher_n)) malloc(num_threads * sizeof(*t_higher_n));
		t_equal_n = (typeof(t_equal_n)) malloc(num_threads * sizeof(*t_equal_n));
		for (i=0;i<num_threads;i++)
		{
			t_lower_n[i] = 0;
			t_higher_n[i] = 0;
			t_equal_n[i] = 0;
		}
	}

	#pragma omp barrier

	long num_iters;
	loop_partitioner_balance_iterations_base(num_threads, tnum, i_start, i_end, 1, &i_s, &i_e, &num_iters);

	partition_concurrent_non_equal(pivot, A, buf, i_s, i_e, aux_data, &lower_n, &equal_n, &higher_n);

	t_lower_n[tnum] = lower_n;
	t_higher_n[tnum] = higher_n;
	t_equal_n[tnum] = equal_n;

	// printf("%d: %ld %ld %ld\n", tnum, t_lower_n[tnum], t_higher_n[tnum], t_equal_n[tnum]);
	#pragma omp barrier

	#pragma omp single nowait
	{
		for (i=0;i<num_threads;i++)
		{
			l += t_lower_n[i];
			h += t_higher_n[i];
			e += t_equal_n[i];
		}

		// In case 'e - d' is an odd number, favor the smallest part.
		// This ensures that no part is empty, unless when there is only one element (caught at the beginning).
		if (l < h)
		{
			d = h - l;
			el = (d < e) ? d + ((e - d + 1) / 2) : e;
			eh = e - el;
		}
		else
		{
			d = l - h;
			eh = (d < e) ? d + ((e - d + 1) / 2) : e;
			el = e - eh;
		}

		m = i_start + l + el;
		// printf("[%ld,%ld](%ld), pivot=%d, m=%ld, %ld, %ld, %ld(%ld,%ld)\n", i_start, i_end, i_end-i_start, ((int *) aux_data)[pivot], m, l, h, e, el, eh);

		h = i_start + l + e;
		e = i_start + l;
		l = i_start;
		for (i=0;i<num_threads;i++)
		{
			tmp = t_lower_n[i];
			t_lower_n[i] = l;
			l += tmp;

			tmp = t_equal_n[i];
			t_equal_n[i] = e;
			e += tmp;

			tmp = t_higher_n[i];
			t_higher_n[i] = h;
			h += tmp;
		}
		// printf("%d: %ld %ld\n", tnum, t_lower_n[tnum], t_higher_n[tnum]);
	}

	#pragma omp barrier

	j = t_lower_n[tnum];
	for (i=i_s;i<i_s+lower_n;i++)
		A[j++] = buf[i];

	j = t_equal_n[tnum];
	for (i=0;i<equal_n;i++)
		A[j++] = pivot;

	j = t_higher_n[tnum];
	for (i=i_e-higher_n;i<i_e;i++)
		A[j++] = buf[i];

	#pragma omp barrier

	#pragma omp single nowait
	{
		free(t_lower_n);
		free(t_higher_n);
		free(t_equal_n);
		// for (i=i_start;i<i_end;i++)
		// {
			// printf("%d, ", ((int *) aux_data)[buf[i]]);
		// }
		// printf("\n");
		// for (i=i_start;i<i_end;i++)
		// {
			// printf("%d, ", ((int *) aux_data)[A[i]]);
		// }
		// printf("\n");
	}

	#pragma omp barrier

	return m;
}


__attribute__((always_inline))
static inline
long
partition_concurrent2(_TYPE_V * A, long s, long e, _TYPE_AD * aux_data)
{
	// int num_threads = safe_omp_get_num_threads();
	// int tnum = omp_get_thread_num();

	_TYPE_V pivot;
	long pivot_pos;
	// long block_size = 1024;
	long i_s, i_e;
	long cmp, balance;

	// #pragma omp barrier
	// #pragma omp barrier

	if (s == e)
		return s+1;

	pivot_pos = (s+e)/2;    // A somewhat better pivot than simply the first element. Also works well when already sorted.

	/* In order for the left part to be non-empty we always at least advance by a value equal to the pivot.
	 * In order for the right part to be non-empty the value at i_s always belongs to the right part, and is swapped at the end.
	 */
	SWAP(&A[s], &A[pivot_pos]);
	pivot = A[s];
	i_s = s+1;
	i_e = e;

	balance = 0;
	while (1)
	{
		while (i_s < i_e)
		{
			cmp = quicksort_cmp(A[i_s], pivot, aux_data);
			if ((cmp > 0) || (cmp == 0 && balance > 0))
				break;
			i_s++;
			balance++;
		}
		while (i_s < i_e)
		{
			cmp = quicksort_cmp(A[i_e], pivot, aux_data);
			if ((cmp < 0) || (cmp == 0 && balance < 0))
				break;
			i_e--;
			balance--;
		}
		if (i_s >= i_e)
			break;
		SWAP(&A[i_s], &A[i_e]);
		i_s++;
		i_e--;
	}

	/* Since the value at 'i_s' always belongs to the right part,
	 * if the pivot is the max value we have to swap with it for when there are only two values in the segment.
	 */
	if (quicksort_cmp(pivot, A[i_s], aux_data) > 0)
		SWAP(&A[s], &A[i_s]);

	return i_s;
}


long
quicksort_partition_concurrent(_TYPE_V * A, _TYPE_V * buf, long s, long e, _TYPE_AD * aux_data)
{
	return partition_concurrent(A, buf, s, e-1, aux_data);
}

long
quicksort_partition_concurrent_inplace(_TYPE_V pivot, _TYPE_V * A, long i_start, long i_end, _TYPE_AD * aux_data)
{
	return partition_concurrent_inplace_base(pivot, A, i_start, i_end, aux_data);
}



//==========================================================================================================================================
//= Quicksort
//==========================================================================================================================================


void
quicksort_no_malloc(_TYPE_V * A, long N, _TYPE_AD * aux_data, _TYPE_I * partitions)
{
	long s, m, e;
	long i;
	if (N < 2)
		return;
	s = 0;
	e = N - 1;
	m = 0;
	i = 0;
	while (1)
	{
		while (s >= e)
		{
			if (s == 0)
				return;
			i--;
			e--;
			s = partitions[i];
		}
		m = partition_serial(A, s, e, aux_data);
		partitions[i++] = s;
		s = m;
	}
}


void
quicksort(_TYPE_V * A, long N, _TYPE_AD * aux_data)
{
	_TYPE_I * partitions;
	if (N < 2)
		return;
	partitions = (typeof(partitions)) malloc(N * sizeof(*partitions));
	quicksort_no_malloc(A, N, aux_data, partitions);
	free(partitions);
}


void
quicksort_no_malloc_parallel(_TYPE_V * A, _TYPE_V * buf, long N, _TYPE_AD * aux_data, _TYPE_I * partitions)
{
	long s, m, e;
	long i;
	if (N < 2)
		return;
	s = 0;
	e = N - 1;
	m = 0;
	i = 0;
	while (1)
	{
		while (s >= e)
		{
			if (s == 0)
				return;
			i--;
			e--;
			s = partitions[i];
		}
		if (__builtin_expect(e - s > 1 << 10, 0))
		{
			#pragma omp parallel
			{
				m = partition_concurrent(A, buf, s, e, aux_data);
			}
		}
		else
		{
			m = partition_serial(A, s, e, aux_data);
		}
		partitions[i++] = s;
		s = m;
	}
}


void
quicksort_parallel(_TYPE_V * A, long N, _TYPE_AD * aux_data)
{
	_TYPE_V * buf;
	_TYPE_I * partitions;
	if (N < 2)
		return;
	buf = (typeof(buf)) malloc(N * sizeof(*buf));
	partitions = (typeof(partitions)) malloc(N * sizeof(*partitions));
	quicksort_no_malloc_parallel(A, buf, N, aux_data, partitions);
	free(buf);
	free(partitions);
}


