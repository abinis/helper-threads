#include <vector>
#include <iostream>
#include <stddef.h>
#include <stdio.h>
#include <algorithm>
#include <ratio>
#include <assert.h>
#include <array>
#include <thread>
#include <execution>
#include <boost/sort/sort.hpp>
#include <cstdlib>
#include <time.h>

using namespace std;

#define MAT_SIZE 10000000

/*typedef struct edge_st {
	
	unsigned int v1;
	unsigned int v2;
	unsigned int weight;

} edge_t;

bool cpp_edge_compare(const edge_t& e1, const edge_t& e2)
{
   // if ( e1.weight < e2.weight )
    //    return -1;
   // else if ( e1.weight > e2.weight )
    //    return 1;
   // else
   //     return 0;
	return e1.weight < e2.weight;
}

    
extern "C" void
cpp_sort_el(edge_t * t) {

	//edge_st *tmp_edge_arr;

	assert(t);
   // assert(el->edge_array);

	//el->edge_array = tmp_edge_arr; 
	
	std::cout << "sdfsdfsd\n";
	fflush(stdout);
	std::cout << t[0].weight << "\n";
	fflush(stdout);

	std::vector<edge_t> v (t, t + sizeof(t) / sizeof(t[0]));

//	cout <<v[0].weight << "\n";
	//for(const auto& w : v)
	//	std::cout << w.weight << "*" ;

	std::cout << sizeof(t) << "\n";
	for(int i=0; i<sizeof(t) * 13; i++)
		cout <<t[i].weight << " ";

	cout << "\n";

	//std::vector<edge_t> v;
	//v.assign(el, el + el->nedges -1);
//	v.assign(tmp_edge_arr, tmp_edge_arr + el->nedges);
//	std::sort (v.begin(), v.end(), cpp_edge_compare);
	//sort(std::execution::par, v.begin(), v.end(), cpp_edge_compare);
	boost::sort::parallel_stable_sort(v.begin(), v.end(), cpp_edge_compare,4);
	for(int i=0; i<MAT_SIZE; i++)
		cout <<t[i].weight << " ";

	cout << "\n";

	}

int 
main() {
	
	//edge_st mat_edge[MAT_SIZE];
	edge_t * mat_edge = (edge_t *)malloc(sizeof(edge_st) * MAT_SIZE);

	int lb = 1, ub = 100;

	srand(time(0));

	for(int i=0; i<MAT_SIZE; i++) 
		//mat_edge[i].weight = rand();  
		mat_edge[i].weight = (rand() % (ub - lb + 1)) + lb;

	//for(int i=0; i<MAT_SIZE; i++)
	//	cout << mat_edge[i].weight << " ";

	cout << "\n";

	//sort(mat_edge, mat_edge+MAT_SIZE, cpp_edge_compare);


	cpp_sort_el(mat_edge);
	
	//for(int i=0; i<MAT_SIZE; i++)
	//	cout << mat_edge[i].weight << " ";

	cout << "\n";

}*/
// -------------------------------------------------------------------------------------

/*
// Define a struct representing a person
struct Person {
    unsigned int id;
	unsigned int id2;
    unsigned int age;
};

// Custom comparison function for sorting by age
bool compareByAge(const Person& a, const Person& b) {
    return a.age < b.age;
}

struct Person * cpp_sort_el(struct Person *matper) {
	std::vector<Person> people(matper, matper + MAT_SIZE);
	boost::sort::parallel_stable_sort(people.begin(), people.end(), compareByAge,2);
	//for(int i=0; i<MAT_SIZE; i++)
	//	std::cout  << "ID: " << people[i].id << ", Age: " << people[i].age << std::endl;
	//matper = &people[0];
	
	struct Person *tmpmatper = people.data();
	//std::copy(people.begin(), people.end(), matper);
	return tmpmatper;
}

int main() {
    // Create an array of Person structs
//    std::vector<Person> people = {
//        {"Alice", 30},
//        {"Bob", 25},
//        {"Charlie", 35},
//        {"David", 22}
//    };

	struct Person * matPerson = (struct Person*)malloc(sizeof(struct Person) * MAT_SIZE);


	int lb = 1, ub = 50000;

	srand(time(0));

	for(int i=0; i<MAT_SIZE; i++) {
		//mat_edge[i].weight = rand();  
		matPerson[i].age = (rand() % (ub - lb + 1)) + lb;
		matPerson[i].id = i;
		matPerson[i].id = i * 10;

	}

	matPerson = cpp_sort_el(matPerson); 

	//for(int i=0; i<MAT_SIZE; i++)
	//	std::cout  << "ID: " << matPerson[i].id << ", Age: " << matPerson[i].age << std::endl;

	
	//std::vector<Person> people(matPerson, matPerson + MAT_SIZE);	
		

    // Sort the array of Person structs by age using std::sort
    //std::sort(people.begin(), people.end(), compareByAge);
//	sort(std::execution::par_unseq, people.begin(), people.end(), compareByAge);


	//boost::sort::parallel_stable_sort(people.begin(), people.end(), compareByAge,8);

    // Print the sorted array
//    for (const auto& person : people) {
 //     std::cout << "ID: " << person.id << ", Age: " << person.age << std::endl;
  //  }

//	cout << "\n";
	
//	people.clear();
//	free(matPerson);

    return 0;

}*/

//---------------------------------------------------------------------------------------------------------


// Define a struct representing a person
struct Person {
    unsigned int id;
    unsigned int age;
};

// Custom comparison function for sorting by age
bool compareByAge(const Person& a, const Person& b) {
    return a.age < b.age;
}

int main() {
    // Create an array of Person structs
//    std::vector<Person> people = {
//        {"Alice", 30},
//        {"Bob", 25},
//        {"Charlie", 35},
//        {"David", 22}
//    };

	struct Person * matPerson = (struct Person*)malloc(sizeof(struct Person) * MAT_SIZE);


	int lb = 1, ub = 100;

	srand(time(0));
        //srand(0);

	for(int i=0; i<MAT_SIZE; i++) {
		//mat_edge[i].weight = rand();  
		matPerson[i].age = (rand() % (ub - lb + 1)) + lb;
		matPerson[i].id = i; 
	}



	std::vector<Person> people(matPerson, matPerson + MAT_SIZE);	
		

    // Sort the array of Person structs by age using std::sort
    //std::sort(std::execution::par_unseq,people.begin(), people.end(), compareByAge);
	nth_element(std::execution::par_unseq, people.begin(), std::next(people.begin(),std::distance(people.begin(),people.end())/2), people.end(), compareByAge);

	//boost::sort::sample_sort(people.begin(), people.end(), compareByAge,8);

    // Print the sorted array
    //for (const auto& person : people) {
    //  std::cout << "ID: " << person.id << ", Age: " << person.age << std::endl;
    //}

//	cout << "\n";
	
	people.clear();
	free(matPerson);

    return 0;

}

