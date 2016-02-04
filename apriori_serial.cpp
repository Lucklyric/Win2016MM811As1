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
#include <algorithm>
using namespace std;

/*Define my own hasher*/
template < typename INTVECTOR > struct int_vector
{
	/*Self-defined hash function for vector<int> type*/
	std::size_t operator() ( const INTVECTOR& seq ) const{
		std::size_t seed = 0;
		for(auto& i : seq) {
			seed ^= i + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		}
		return seed;
	}
};

/**
 * Define variables
 */
int num_transactions = 0;
int currentLevel = 1;
float min_support = 0.001;
vector<vector<int>> DB; 										//database
typedef unordered_map<vector<int>,int,int_vector<vector<int>>> u_map_vector;
unordered_map <int,u_map_vector> candidates_k;
unordered_map <int,u_map_vector> large_itemsets_k;


/**
 * Declare all functions
 */
u_map_vector generateLargeItemsets(u_map_vector &candidates); 	//Generate large itemsets with given candidates
u_map_vector generateCandidates(u_map_vector &largeItemsets);
bool inputdata(const  char* filename); 							//Read data from file and initialize C1 candidates
void foreachDB(u_map_vector &candidates);
void nestCheckSubset(vector<int> &transaction,int level,vector<int>&pre,int idx);
void output(u_map_vector &map);

/**
 * Main application
 */
int main(int argc, char const *argv[]){
	clock_t begin = clock();
	/*Start loading data and initialize C1 and L1*/
	if (inputdata(argv[1])) {printf("Input file success!\n");}
	while(true){
		printf("Number of frequent %d_itemsets: %d\n",currentLevel,(int)large_itemsets_k[currentLevel].size());
		currentLevel++;
		u_map_vector candidates = generateCandidates(large_itemsets_k[currentLevel-1]);
		candidates_k[currentLevel] = candidates;
		printf("Number of candidates:%d\n",(int)candidates.size());
		foreachDB(candidates_k[currentLevel]);
		u_map_vector largeItemsets = generateLargeItemsets(candidates_k[currentLevel]);
		printf("Number of largeItemsets:%d\n",(int)largeItemsets.size());

		if (largeItemsets.size() == 0) {
			printf("No any more large item-sets! at level %d\n",currentLevel);
			break;
		}
		large_itemsets_k[currentLevel] = largeItemsets;
	}
	clock_t end = clock();
	double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
	printf("Total spend %f s\n",elapsed_secs);

	return 0;
}

bool inputdata(const char* filename){
	printf("Start loading data...\n");
	u_map_vector candidates;
	clock_t begin = clock();
	ifstream inputFile;
	inputFile.open(filename);
	if(!inputFile){
		printf("Input file error");
		return false;
	}
	string line ;
	while(getline(inputFile,line)){
		stringstream ss(line);
		vector<int> tmpLine;
		vector<int> itemsets;
		num_transactions++;
		int item;
		while (ss >> item){
			itemsets.clear();
			itemsets.push_back(item);
			tmpLine.push_back(item);
			if (candidates.count(itemsets) > 0){
				candidates[itemsets]++;

			}else{
				candidates[itemsets] = 1;
			}
		}
		DB.push_back(tmpLine);
		tmpLine.clear();
	}
	inputFile.close();
	clock_t end = clock();
	double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
	candidates_k[1] = candidates;
	large_itemsets_k[1] = generateLargeItemsets(candidates);
	printf("Spend %f s\n",elapsed_secs);
	return true;
}

u_map_vector generateLargeItemsets(u_map_vector &candidates){
	u_map_vector largeItemsets;

	for(u_map_vector::iterator itemset = candidates.begin();itemset!=candidates.end();++itemset){
		if(itemset->second >= min_support*num_transactions ){
			largeItemsets[itemset->first] = itemset->second;
		}
	}
	return largeItemsets;
}

u_map_vector generateCandidates(u_map_vector &largeItemsets){
	u_map_vector candidates;
	u_map_vector::iterator largeItemsets_end;
	for(u_map_vector::iterator itemsetA = largeItemsets.begin();itemsetA!=largeItemsets_end;++itemsetA){
		for(u_map_vector::iterator itemsetB = itemsetA;itemsetB!=largeItemsets_end;++itemsetB){
			if (itemsetA!=itemsetB){
				bool flag = true;
				for (int i = 0; i < currentLevel-2;i++){
					if (itemsetA->first[i]!=itemsetB->first[i]){
						flag = false;
						break;
					}
				}
				if (flag){
					vector<int> candidate = itemsetA->first;
					candidate.push_back(itemsetB->first.back());
					sort(candidate.begin(),candidate.end());
					//candidates[candidate] = 0;
					//Prune
					for (int j = 0;j<currentLevel;j++){
						vector<int> tmpSubset;
						for (int k = 0 ; k < currentLevel;k++){
							if (j!=k){
								tmpSubset.push_back(candidate[k]);
							}
						}
						if (largeItemsets.find(tmpSubset) != largeItemsets_end){
							//the sub-itemset in L-k-1 then it is the valid candidate
							candidates[candidate] = 0;
						}
					}
				}
			}
		}
	}
	return candidates;
}

void foreachDB(u_map_vector &candidates){
//	vector<vector<int> >::iterator DB_begin = DB.begin();
//	vector<vector<int> >::iterator DB_end = DB.end();
//	int num = 0;
//	for(vector<vector<int> >::iterator transaction = DB_begin;transaction!=DB_end;++transaction){
//		u_map_vector::iterator candidates_end = candidates.end();
//		vector<int>::iterator transaction_end = (*transaction).end();
//		for(u_map_vector::iterator itemset = candidates.begin();itemset!=candidates_end;++itemset){
//			int matchItem = 0;
//			if (itemset->first.size() <= (*transaction).size()){
//				vector<int> tmp = itemset->first;
//				vector<int>::iterator itemset_end = tmp.end();
//				for (vector<int>::iterator item = tmp.begin();item!= itemset_end;++item){
//					for (vector<int>::iterator tran_item = (*transaction).begin();tran_item!= transaction_end;++tran_item){
//						if (*item == *tran_item) {matchItem++;break;}
//					}
//				}
//			}
//			if (matchItem == itemset->first.size()){
//				candidates[itemset->first]++;
//			}
//		}
//	}
		int num = 0;
		for(int i = 0 ; i < num_transactions;i++){
			u_map_vector::iterator candidates_end = candidates.end();
			vector<int> transaction = DB[i];
			if (transaction.size()<currentLevel) continue;
			for (int j = 0 ; j <  transaction.size()-currentLevel+1;j++){
				vector<int> tmp;tmp.push_back(DB[i][j]);
				nestCheckSubset(transaction,1,tmp,j);
			}
		}
	printf("EndScan\n");
}


void nestCheckSubset(vector<int> &transaction,int level,vector<int>&pre,int idx){
	if (level < currentLevel){
		if (large_itemsets_k[currentLevel-1].find(pre) == large_itemsets_k[currentLevel-1].end()) return;
			for (int j = idx+1;j<transaction.size()-currentLevel+level+1;j++){
				vector<int> tmp = pre;
				tmp.push_back(transaction[j]);
				nestCheckSubset(transaction,level+1,tmp,j);
			}
	}else{

		if(candidates_k[currentLevel].find(pre)!=candidates_k[currentLevel].end()){
			candidates_k[currentLevel][pre]++;
		}
	}
}

/**
 * Util functions for testing
 */
void output(u_map_vector &map){
	for(u_map_vector::iterator itemset = map.begin();itemset!=map.end();++itemset){
		printf("Candidates:");
		for(int i = 0; i < (itemset->first).size();i++){
			printf("%d ",itemset->first[i]);
		}
		printf(": sup=%d\n",itemset->second);
		}
}
