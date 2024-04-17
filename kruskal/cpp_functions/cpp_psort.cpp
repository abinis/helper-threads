#include <vector>
//#include <iostream>
//#include <stddef.h>
#include <stdio.h>
//#include <ratio>
#include <assert.h>
//#include <array>
//#include <thread>

#if defined USE_BOOST_PSORT || defined USE_BOOST_SAMSORT || defined USE_BOOST_BLISORT
#include <boost/sort/sort.hpp>
#else
#include <algorithm>
#if __cplusplus >= 201703L
#include <execution>
#endif
#endif
//#include "/home/tkats/playground/kruskal/cpp_integrate/graph/edgelist.h"

#include "graph/edgelist.h"

//#ifdef PROFILE
//#include "machine/tsc_x86_64.h"
//#endif
//#ifndef SORT_NUM_THREADS
//#define SORT_NUM_THREADS 16
//#endif

using namespace std;

/*
class EdgeArrayWrapperIterator
{
private:
    edge_t *edge_array;
    size_t nedges;
    int index = 0;

public:
    EdgeArrayWrapperIterator()
        : index(0)
    {
    }

    EdgeArrayWrapperIterator(edge_t *ea_, size_t ne_)
        : edge_array(ea_), nedges(ne_), index(0)
    {
    }

    EdgeArrayWrapperIterator(edge_t *ea_, size_t ne_, int i_)
        : edge_array(ea_), nedges(ne_), index(i_)
    {
    }

    edge_t& operator*()
    {
        return edge_array[index];
    }

    const edge_t& operator*() const
    {
        return edge_array[index];
    }

    EdgeArrayWrapperIterator& operator++()
    {
        index++;
        return *this;
    }
    // required by boost 
    EdgeArrayWrapperIterator& operator++(int)
    {
        index++;
        return *this;
    }

    EdgeArrayWrapperIterator& operator--()
    {
        index--;
        return *this;
    }
    // requiired by boost
    EdgeArrayWrapperIterator& operator--(int)
    {
        index--;
        return *this;
    }

    friend int operator- (const EdgeArrayWrapperIterator& lhs, const EdgeArrayWrapperIterator& rhs)
    {
        return lhs.index - rhs.index;
    }

    friend EdgeArrayWrapperIterator operator-(EdgeArrayWrapperIterator const& lhs, int rhs)
    {
        return EdgeArrayWrapperIterator(lhs.edge_array, lhs.nedges, lhs.index - rhs);
    }

    friend EdgeArrayWrapperIterator operator-(int lhs, EdgeArrayWrapperIterator const& rhs)
    {
        return EdgeArrayWrapperIterator(rhs.edge_array, rhs.nedges, lhs - rhs.index);
    }

    friend EdgeArrayWrapperIterator operator+(EdgeArrayWrapperIterator const& lhs, int rhs)
    {
        return EdgeArrayWrapperIterator(lhs.edge_array, lhs.nedges, lhs.index + rhs);
    }
friend EdgeArrayWrapperIterator operator+(int lhs, EdgeArrayWrapperIterator const& rhs) {
        return EdgeArrayWrapperIterator(rhs.edge_array, rhs.nedges, lhs + rhs.index);
    }

    friend EdgeArrayWrapperIterator& operator+= (EdgeArrayWrapperIterator& lhs, const EdgeArrayWrapperIterator& rhs)
    {
        lhs.index += rhs.index;
        return lhs;
    }

    friend EdgeArrayWrapperIterator& operator+= (EdgeArrayWrapperIterator& lhs, int rhs)
    {
        lhs.index += rhs;
        return lhs;
    }

    friend EdgeArrayWrapperIterator& operator-= (EdgeArrayWrapperIterator& lhs, const EdgeArrayWrapperIterator& rhs)
    {
        lhs.index -= rhs.index;
        return lhs;
    }

    friend EdgeArrayWrapperIterator& operator-= (EdgeArrayWrapperIterator& lhs, int rhs)
    {
        lhs.index -= rhs;
        return lhs;
    }

    friend bool operator== (const EdgeArrayWrapperIterator& lhs, const EdgeArrayWrapperIterator& rhs)
    {
        //if (rhs.index >= nedges)
        //{
            return lhs.index == rhs.index;
        //}

        //return (*lhs) == (*rhs);
    }

    friend bool operator!= (const EdgeArrayWrapperIterator& lhs, const EdgeArrayWrapperIterator& rhs)
    {
        //if (rhs.index >= nedges)
        //{
            return lhs.index != rhs.index;
        //}

        //return (*lhs) != (*rhs);
    }

    friend bool operator<= (const EdgeArrayWrapperIterator& lhs, const EdgeArrayWrapperIterator& rhs)
    {
        //if (rhs.index >= nedges)
        //{
            return lhs.index <= rhs.index;
        //}

        //return (*lhs) <= (*rhs);
    }

    friend bool operator>= (const EdgeArrayWrapperIterator& lhs, const EdgeArrayWrapperIterator& rhs)
    {
        //if (rhs.index >= nedges)
        //{
            return lhs.index >= rhs.index;
        //}

        //return (*lhs) >= (*rhs);
    }

    friend bool operator< (const EdgeArrayWrapperIterator& lhs, const EdgeArrayWrapperIterator& rhs)
    {
        //if (rhs.index >= nedges)
        //{
            return lhs.index < rhs.index;
        //}

        //return (*lhs) < (*rhs);
    }

    friend bool operator> (const EdgeArrayWrapperIterator& lhs, const EdgeArrayWrapperIterator& rhs)
    {
        //if (rhs.index >= nedges)
        //{
            return lhs.index > rhs.index;
        //}

        //return (*lhs) < (*rhs);
    }

    using difference_type = std::ptrdiff_t;
    using value_type = edge_t;
    using pointer = edge_t*;
    using reference = edge_t&;
    using iterator_category = std::random_access_iterator_tag;
};

class EdgeArrayWrapper
{
private:
    edge_t * edge_array;
    size_t nedges;
    size_t edge_size;

public:

    typedef EdgeArrayWrapperIterator iterator;
    typedef const EdgeArrayWrapperIterator const_iterator;

    EdgeArrayWrapper(edge_t * ea_, size_t n_, size_t s_)
    {
        edge_array = ea_;
        nedges = n_;
        edge_size = s_;
    }

    size_t size()
    {
        return nedges;
    }

    EdgeArrayWrapper::iterator begin()
    {
        return EdgeArrayWrapperIterator(edge_array, nedges, 0);
    }

    EdgeArrayWrapper::iterator end()
    {
        return EdgeArrayWrapperIterator(edge_array, nedges, size());
    }

    EdgeArrayWrapper::const_iterator cbegin()
    {
        return EdgeArrayWrapperIterator(edge_array, nedges, 0);
    }

    EdgeArrayWrapper::const_iterator cend()
    {
        return EdgeArrayWrapperIterator(edge_array, nedges, size());
    }

    edge_t& operator[](int i)
    {
        return edge_array[i];
    }
};
*/

//std::vector<edge_t> v;
//edge_t * tmp_edge_arr;
/**
 * Edge comparison function for use with qsort
 * @param e1 first edge compared
 * @param e2 second edge compared
 * @return -1, 0, 1 if e1.weight <,=,> e2.weight, respectively
 */ 
static inline
bool cpp_edge_compare(const edge_t& e1, const edge_t& e2)
{
	return e1.weight < e2.weight;
}

//extern "C" void
//clear_vector(){
//	//free(tmp_edge_arr);
//	v.clear();
//        //delete v;
//}
extern "C"
//__attribute__((always_inline))
long int
cpp_partition_edge_arr(edge_t * t, size_t n, long int pivot_index) {

	assert(t);

        edge_t pivot = t[pivot_index];//*std::next(t, pivot_index);
#if __cplusplus >= 201703L
        //printf("using std library par (post C++17) partition!\n");
        auto it = partition(std::execution::par,
                                 t,
                                 t+n,
                                 [pivot](const edge_t& e)
                                 {
                                     return e.weight < pivot.weight;
                                     //return cpp_edge_compare(pivot,e);
                                 });
#else
        //printf("using std library serial (pre C++17) partition!\n");
        auto it = partition(t,
                            t+n,
                            [pivot](const edge_t& e)
                            {
                                return e.weight < pivot.weight;
                                //return cpp_edge_compare(pivot,e);
                            });
#endif

        return it-t;
        
}

extern "C"
//__attribute__((always_inline))
long int
cpp_partition_foo(edge_t * t, size_t n, edge_t pivot) {

	assert(t);

        //edge_t pivot = t[pivot_index];//*std::next(t, pivot_index);
#if __cplusplus >= 201703L
        //printf("using std library par (post C++17) partition!\n");
        auto it = partition(std::execution::par,
                                 t,
                                 t+n,
                                 [pivot](const edge_t& e)
                                 {
                                     return e.weight < pivot.weight;
                                     //return cpp_edge_compare(pivot,e);
                                 });
#else
        //printf("using std library serial (pre C++17) partition!\n");
        auto it = partition(t,
                            t+n,
                            [pivot](const edge_t& e)
                            {
                                return e.weight < pivot.weight;
                                //return cpp_edge_compare(pivot,e);
                            });
#endif

        return it-t;
        
}

std::vector<edge_t> v;

extern "C"
edge_t *
cpp_nth_element_edge_arr_v(edge_t *t, size_t n, long int k) {

        assert(t);

        v.assign(t,t+n);

#if __cplusplus >= 201703L
        printf("using std library par (post C++17) nth_element!\n");
	nth_element(std::execution::par_unseq, v.begin(),std::next(v.begin(),std::distance(v.begin(),v.end())/2), v.end(), cpp_edge_compare);
#else
        printf("using std library serial (pre C++17) nth_element!\n");
	nth_element(t, t+k, t+n, cpp_edge_compare);
#endif

        return v.data();
}

extern "C"
void
cpp_nth_element_edge_arr(edge_t *t, size_t n, long int k) {

        assert(t);

#if __cplusplus >= 201703L
        printf("using std library par (post C++17) nth_element!\n");
	nth_element(std::execution::par_unseq, t, t+k, t+n, cpp_edge_compare);
#else
        printf("using std library serial (pre C++17) nth_element!\n");
	nth_element(t, t+k, t+n, cpp_edge_compare);
#endif

}

extern "C"
long int
cpp_partition_double_arr(double * t, size_t n, int pivot_index) {

	assert(t);

        double pivot = *std::next(t, pivot_index);
#if __cplusplus >= 201703L
        printf("using std library par (post C++17) partition!\n");
        auto it = partition(std::execution::par_unseq,
                                 t,
                                 t+n,
                                 [pivot](const double& e)
                                 {
                                     return e < pivot;
                                 });
#else
        printf("using std library serial (pre C++17) partition!\n");
        auto it = partition(t,
                            t+n,
                            [pivot](const double& e)
                            {
                                return e < pivot;
                            });
#endif

        return it-t;
        
}

extern "C" //edge_t *
void
//extern "C" 
//template<class edge_t, size_t N> void
cpp_sort_edge_arr(edge_t * t, unsigned int as, int num_threads) {


	assert(t);
        //assert(el->edge_array);

	//el->edge_array = tmp_edge_arr; 
	
	//std::cout << t[0].weight << "\n";
	//fflush(stdout);

	//std::vector<edge_t> v (t, t + as);
        //std::cout << "edge_array size = " << as;
//#ifdef PROFILE
//    tsctimer_t tim;
//    timer_clear(&tim);
//    timer_start(&tim);
//#endif
//	v.assign(t, t + as);
//#ifdef PROFILE
//    timer_stop(&tim);
//    double hz = timer_read_hz();
//    fprintf(stdout, "vector assign     cycles:%lf seconds:%lf freq:%lf\n", 
//                    timer_total(&tim),
//                    timer_total(&tim) / hz,
//                    hz );
//#endif
        //EdgeArrayWrapper eaw(t, as, sizeof(edge_t));
        //v(t, t+as);
        //std::array(t);
        //std::cout << "(before sort) v size = " << v.size() << '\n';
        //for ( edge_t e: v )
        //    std::cout << e.weight << '\n';

	//std::cout << v[0].weight << "\n";
	//fflush(stdout);

	//std::vector<edge_t> v;
	//v.assign(el, el + el->nedges -1);
//	v.assign(tmp_edge_arr, tmp_edge_arr + el->nedges);
//	std::sort (v.begin(), v.end(), cpp_edge_compare);

//TODO consider moving printing of messages to the per case relevant executable
#ifdef USE_BOOST_PSORT
        printf("using boost library parallel_stable_sort with %d threads!\n", num_threads);
	//boost::sort::parallel_stable_sort(v.begin(), v.end(), cpp_edge_compare, num_threads);
	boost::sort::parallel_stable_sort(t, t+as, cpp_edge_compare, num_threads);
#elif defined USE_BOOST_SAMSORT
        printf("using boost library sample_sort with %d threads!\n", num_threads);
	//boost::sort::sample_sort(v.begin(), v.end(), cpp_edge_compare, num_threads);
	boost::sort::sample_sort(t, t+as, cpp_edge_compare, num_threads);
#elif defined USE_BOOST_BLISORT
        printf("using boost library block_indirect_sort with %d threads!\n", num_threads);
	//boost::sort::block_indirect_sort(v.begin(), v.end(), cpp_edge_compare, num_threads);
	boost::sort::block_indirect_sort(t, t+as, cpp_edge_compare, num_threads);
#else
#if __cplusplus >= 201703L 
        if ( num_threads > 1 ) {
            // note: you should actually specify, e.g. via 'taskset -c', how many threads to use!
            printf("using std library par (post C++17) sort!\n");
	    //sort(std::execution::par_unseq, v.begin(), v.end(), cpp_edge_compare);
	    sort(std::execution::par_unseq, t, t+as, cpp_edge_compare);
        } else {
            printf("using std library serial sort!\n");
	    //sort(v.begin(), v.end(), cpp_edge_compare);
	    sort(t, t+as, cpp_edge_compare);
        }
#else
        printf("using std library serial sort!\n");
	//sort(v.begin(), v.end(), cpp_edge_compare);
	sort(t, t+as, cpp_edge_compare);
#endif
#endif
	//std::cout << v[0].weight << "\n";
	//fflush(stdout);	
	//edge_t *tmp_edge_arr = v.data();
        //std::cout << "(after sort) v size = " << v.size() << '\n';
        //for ( edge_t e: v )
        //    std::cout << e.weight << '\n';

	//tmp_edge_arr=v.data();
	//std::cout << v[0].weight << "\n";
	//fflush(stdout);	
	//std::copy(v.begin(),v.end(),t);
	//t=v.data();
        //free(t);
	//return t;
	//return tmp_edge_arr;
}
