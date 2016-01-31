#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <ctime>
using namespace std;
//Define my own hasher
template < typename INTVECTOR > struct int_vector
{
	//Self-defined hash function for vector<int> type
	std::size_t operator() ( const INTVECTOR& seq ) const{
		std::size_t seed = 0;
		for(auto& i : seq) {
			seed ^= i + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		}
		return seed;
	}
};

vector<vector<int>> DB; /*The global database*/

unordered_map <vector<int>,int,int_vector<vector<int>>> candidates;
//map <vector<int>,int> candidates;
bool inputdata(const  char* filename);/*Read data from file and initialize C1 candidates*/


int main(int argc, char const *argv[]){
	//Start loading data;
	if (inputdata(argv[1])) {printf("Input file success!\n");}
	printf("%d\n",(int)DB.size());
	vector<int> a;a.push_back(77);
	printf("%d\n",(int)candidates[a]);

}

bool inputdata(const char* filename){
	printf("Start loading data...\n");
	clock_t begin = clock();
	ifstream inputFile;
	inputFile.open(filename);
	if(!inputFile){
		printf("Input file error");
		return false;
	}
	string line;
	while(getline(inputFile,line)){
		stringstream ss(line);
		vector<int> tmpLine;
		vector<int> itemsets;
		int item;
		while (ss >> item){
			itemsets.clear();
			itemsets.push_back(item);
			tmpLine.push_back(item);
			if (candidates.count(itemsets) > 0){
				candidates[itemsets]++;
			}else{
				candidates[itemsets] = 0;
			}
		}
		DB.push_back(tmpLine);
		tmpLine.clear();
	}
	inputFile.close();
	clock_t end = clock();
	double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
	printf("Spend %f s\n",elapsed_secs);
	return true;
}
