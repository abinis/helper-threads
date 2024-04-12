#undef  partition_serial_base
#define partition_serial_base  QUICKSORT_GEN_EXPAND(partition_serial_base)
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

	while (1)
	{
		while ((i_s < i_e) && (quicksort_cmp(A[i_s], pivot, aux_data) < 0))
			i_s++;
		while ((i_s < i_e) && (quicksort_cmp(A[i_e], pivot, aux_data) > 0))
			i_e--;
		if (i_s >= i_e)
			break;
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


#undef  partition_concurrent_inplace_base
#define partition_concurrent_inplace_base  QUICKSORT_GEN_EXPAND(partition_concurrent_inplace_base)
__attribute__((always_inline))
static inline
long
partition_concurrent_inplace_base(_TYPE_V pivot, _TYPE_V * A, long i_start, long i_end, _TYPE_AD * aux_data)
{
	__label__ partition_concurrent_out;
	int num_threads = safe_omp_get_num_threads();
	int tnum = omp_get_thread_num();

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

	loop_partitioner_balance_iterations(num_threads, tnum, i_start, i_end, &i_s, &i_e);
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
	loop_partitioner_balance_iterations(num_threads, tnum, 0, g_realoc_num_l, &rel_pos_s, &rel_pos_e);
	realoc_num = rel_pos_e - rel_pos_s;

	i = 0;
	sum = t_l_e[i] - t_l_s[i];
	while (sum <= rel_pos_s)
	{
		i++;
		sum += t_l_e[i] - t_l_s[i];
	}
	l_s = t_l_e[i] - (sum - rel_pos_s);
	l_e = t_l_e[i];

	j = 0;
	sum = t_h_e[j] - t_h_s[j];
	while (sum <= rel_pos_s)
	{
		j++;
		sum += t_h_e[j] - t_h_s[j];
	}
	h_s = t_h_e[j] - (sum - rel_pos_s);
	h_e = t_h_e[j];

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

