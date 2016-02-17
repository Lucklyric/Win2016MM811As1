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
float min_support = 10/10000.0f;
vector<vector<int>> DB; 										//database
typedef unordered_map<vector<int>,int,int_vector<vector<int>>> u_map_vector;
unordered_map <int,u_map_vector> candidates_k;
unordered_map <int,u_map_vector> large_itemsets_k;
clock_t lastTimeTick,mainstart;
int debug_flag = 0;


/**
 * Declare all functions
 */
u_map_vector generateLargeItemsets(u_map_vector &candidates); 	//Generate large itemsets with given candidates
u_map_vector generateCandidates(u_map_vector &largeItemsets);
bool inputdata(const  char* filename); 							//Read data from file and initialize C1 candidates
void foreachDB(u_map_vector &candidates);
void nestCheckSubset(vector<int> &transaction,int level,vector<int>&pre,int idx);
void output(u_map_vector &map);
void output(vector<int> &trans);
void timetick(clock_t since);
/**
 * Main application
 */
int main(int argc, char const *argv[]){
	mainstart = lastTimeTick = clock();
	/*Start loading data and initialize C1 and L1*/
	printf("Start finding frequent itemsets\n");
	if (inputdata(argv[1])) {if (debug_flag) printf("Input file success!\n");}
	while(true){
		printf("Number of frequent %d_itemsets: %d\n",currentLevel,(int)large_itemsets_k[currentLevel].size());
		currentLevel++;
		u_map_vector candidates = generateCandidates(large_itemsets_k[currentLevel-1]);
		if (candidates.size() == 0) {
			if (debug_flag) printf("No any more candidates item-sets! at level %d\n",currentLevel);
			break;
		}
		candidates_k[currentLevel] = candidates;
		if (debug_flag)printf("Number of candidates:%d\n",(int)candidates.size());
		foreachDB(candidates_k[currentLevel]);
		//output(candidates_k[currentLevel]);
		u_map_vector largeItemsets = generateLargeItemsets(candidates_k[currentLevel]);
		if (debug_flag)printf("Number of largeItemsets:%d\n",(int)largeItemsets.size());
		if (largeItemsets.size() == 0) {
			if (debug_flag)printf("No any more large item-sets! at level %d\n",currentLevel);
			break;
		}
		large_itemsets_k[currentLevel] = largeItemsets;
	}

	printf("End finding frequent itemsets ");
	timetick(mainstart);

	return 0;
}

bool inputdata(const char* filename){
	if (debug_flag) printf("Input data...");
	u_map_vector candidates;
	ifstream inputFile;
	inputFile.open(filename);
	if(!inputFile){
		printf("Input file error\n");
		return false;
	}
	string line ;
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
				candidates[itemsets] = 1;
			}
		}
		if (!tmpLine.empty()) DB.push_back(tmpLine);
		tmpLine.clear();
	}
	inputFile.close();
	num_transactions = DB.size();
	candidates_k[1] = candidates;
	large_itemsets_k[1] = generateLargeItemsets(candidates);
	if (debug_flag) {
		printf("Finshed");
		timetick(lastTimeTick);
	}
	return true;
}

u_map_vector generateLargeItemsets(u_map_vector &candidates){
	if (debug_flag)printf("Start generating (%d)largeItemsets...\n",currentLevel);
	int tresh = min_support*num_transactions;
	u_map_vector largeItemsets;

	for(u_map_vector::iterator itemset = candidates.begin();itemset!=candidates.end();++itemset){
		if(itemset->second > tresh ){
			largeItemsets[itemset->first] = itemset->second;
		}
	}
	if (debug_flag)printf("End generating (%d)largeItemsets!\n",currentLevel);

	return largeItemsets;
}

u_map_vector generateCandidates(u_map_vector &largeItemsets){
	if (debug_flag)printf("Start generating (k=%d) candidates...\n",currentLevel);
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
					if (candidate[currentLevel-2] > candidate[currentLevel-1]){
						int buffer = candidate[currentLevel-2];
						candidate[currentLevel-2]= candidate[currentLevel-1];
						candidate[currentLevel-1] = buffer;

					}
					if (currentLevel > 2){
						vector<int> two_tmp; two_tmp.push_back(candidate[currentLevel-2]);two_tmp.push_back(candidate[currentLevel-1]);
						if (large_itemsets_k[2].find(two_tmp)!=large_itemsets_k[2].end()) {candidates[candidate] = 0;}
					}else{
						candidates[candidate] = 0;
					}
					//Old Prune Part
					//sort(candidate.begin(),candidate.end());
//					candidates[candidate] = 0;
//					Prune
//					for (int j = 0;j<currentLevel;j++){
//						vector<int> tmpSubset;
//						for (int k = 0 ; k < currentLevel;k++){
//							if (j!=k){
//								tmpSubset.push_back(candidate[k]);
//							}
//						}
//						if (largeItemsets.find(tmpSubset) != largeItemsets_end){
//							//the sub-itemset in L-k-1 then it is the valid candidate
//							candidates[candidate] = 0;
//						}
//					}
					//Old Prune Part End
				}
			}
		}
	}
	if (debug_flag)printf("End generating (k=%d) - (%d)candidates!\n",currentLevel,(int)candidates.size());
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
	if (debug_flag)printf("Start scaning database...");
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
		if (debug_flag)printf("End caning database!\n");
}


void nestCheckSubset(vector<int> &transaction,int level,vector<int>&pre,int idx){
	if (level < currentLevel){

		if (large_itemsets_k[level].find(pre) == large_itemsets_k[level].end()) return;
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
void timetick(clock_t since){
	clock_t current = clock();
	double elapsed_secs = double(current - since) / CLOCKS_PER_SEC;
	lastTimeTick = current;
	printf("spent %f s\n",elapsed_secs);
}

void output(u_map_vector &map){
	for(u_map_vector::iterator itemset = map.begin();itemset!=map.end();++itemset){
		printf("Candidates:");
		for(int i = 0; i < (itemset->first).size();i++){
			printf("%d ",itemset->first[i]);
		}
		printf(": sup=%d\n",itemset->second);
		}
}
void output(vector<int> &trans){
	for(int i = 0 ; i < trans.size();i++){
		printf("%d",trans[i]);
	}
	printf("\n");
}
