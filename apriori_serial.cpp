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

/**
 * Write my own hasher
 * Reference from the boost C++ library
 */
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
int largestLength = 0;
double min_support =0;
double min_confidence = 0.8;
vector<vector<int>> DB; 													//database
typedef unordered_map<vector<int>,double,int_vector<vector<int>>> u_map_vector;					//vector as key double as value for storing the items sets and support
typedef unordered_map<vector<int>,u_map_vector,int_vector<vector<int>>> u_map_vector_rules;		//vector as key u_map_vector as value for storing the strong rules head and body(with confidence)
unordered_map <int,u_map_vector> candidates_k;								//store candidates at each level
unordered_map <int,u_map_vector> large_itemsets_k;							//store large item sets at each level
u_map_vector_rules strong_rules;											//store all strong rules
clock_t lastTimeTick,mainstart;
int debug_flag = 0;
int argv_code = 0;															//0:just display number,1:display frequent,2:display strong rules,3:display all


/**
 * Declare all functions
 */
u_map_vector generateLargeItemsets(u_map_vector &candidates); 				//generate large itemsets with given candidates
u_map_vector generateCandidates(u_map_vector &largeItemsets);				//generate candidates with size k+1
vector<int> vectorRemoveOtherVector(vector<int> base, vector<int> sub);		//generate vector with sub/base
void findLargeItemsets();													//find frequent itemsets
void findStrongRules();														//find all strong rules
bool inputdata(const  char* filename); 										//read data from file and initialize C1 candidates
void foreachDB(u_map_vector &candidates);									//scan the database and update support
void nestCheckSubset(vector<int> &transaction,int level,vector<int>&pre,int idx);		//nestcheck function for fast update candidates' support

/*Util functions*/
void output();																//output to file
void output(u_map_vector &map);
void output(const vector<int> &trans);
void output(u_map_vector_rules &rules);
void timetick(clock_t since);

/**
 * Main
 */
int main(int argc, char const *argv[]){

	/*Parse arguments*/
	if ( argc<4){
		printf("Not enough parameters!\n");
		return 0;
	}else{
		min_support = atof(argv[2]);
		min_confidence = atof(argv[3]);
		printf("min support: %.2f%% ",min_support*100);
		printf("min confidence: %.2f%% \n",min_confidence*100);
		if (argc == 5){
			if (strcmp(argv[4], "f") == 0){
				argv_code = 1;
				printf("output all frequent itemset to 'output.txt.'\n");
			}else if(strcmp(argv[4], "a") == 0){
				argv_code = 3;
				printf("output all frequent itemset and strong rules to 'output.txt.'\n");

			}else if(strcmp(argv[4], "r") == 0){
				argv_code = 2;
				printf("output all strong rules to 'output.txt.'\n");

			}
		}
		printf("\n");
	}

	/*Tick start*/
	mainstart = lastTimeTick = clock();
	if (inputdata(argv[1])) {if (debug_flag) printf("Input file success!\n");}else{return 1;}
	/*find frequent large items sets*/
	findLargeItemsets();
	/*find all strong rules*/
	findStrongRules();
	/*finish apriori*/
	printf("All done ");
	timetick(mainstart);
	output();
	printf("Exit\n");
	return 0;
}

/**
 * Find all frequent itme sets
 */
void findLargeItemsets(){
	/*start loading data and initialize C1 and L1*/
	printf("Start finding frequent itemsets\n");
	/*input data and update min_support with num of transactions*/
	while(true){
		printf("Number of frequent %d_itemsets: %d\n",currentLevel,(int)large_itemsets_k[currentLevel].size());
		currentLevel++;
		/*generate candidates*/
		u_map_vector candidates = generateCandidates(large_itemsets_k[currentLevel-1]);
		if (candidates.size() == 0) {
			if (debug_flag) printf("No any more candidates item-sets! at level %d\n",currentLevel);
			break;
		}
		candidates_k.clear();
		candidates_k[currentLevel] = candidates;
		if (debug_flag)printf("Number of candidates:%d\n",(int)candidates.size());
		/*count support*/
		foreachDB(candidates_k[currentLevel]);
		/*generate frequent itemset*/
		u_map_vector largeItemsets = generateLargeItemsets(candidates_k[currentLevel]);
		if (debug_flag)printf("Number of largeItemsets:%d\n",(int)largeItemsets.size());
		if (largeItemsets.size() == 0) {
			if (debug_flag)printf("No any more large item-sets! at level %d\n",currentLevel);
			break;
		}
		/*save large item sets*/
		large_itemsets_k[currentLevel] = largeItemsets;
	}
	printf("End finding frequent itemsets ");
	timetick(lastTimeTick);
}

/**
 * Find all strong rules
 */
void findStrongRules(){
	printf("Start finding strong rules\n");
	largestLength = currentLevel-1;
	int numofStrongRules = 0;
	for(int length = 2;length <= largestLength;length++){
		/*Foe each frequent Itemset at currentLevel*/
		u_map_vector::iterator largeItemsets_end = large_itemsets_k[length].end();
		for(u_map_vector::iterator itemset = large_itemsets_k[length].begin();itemset!=largeItemsets_end;++itemset){
			vector<int> frequentSet = itemset->first;
			u_map_vector ruleHeaders;
			/*rule head with one one item*/
			for(int i = 0 ; i < frequentSet.size();i++){
				vector<int> tmp;
				tmp.push_back(frequentSet[i]);
				ruleHeaders[tmp] = 0;
			}
			int m = 1;
			/*check possible rules*/
			while (!ruleHeaders.empty() && m < frequentSet.size() ){
				u_map_vector::iterator ruleHeader_end = ruleHeaders.end();
				/*traverse the possible header*/
				for(u_map_vector::iterator ruleHeader = ruleHeaders.begin();ruleHeader!=ruleHeader_end;++ruleHeader){
					/*check the confidence of the rule*/
					vector<int> leftItems = vectorRemoveOtherVector( frequentSet,ruleHeader->first);
					double confidence = (double)large_itemsets_k[length][frequentSet]/(double)large_itemsets_k[m][ruleHeader->first];
					/*save the strong rules*/
					if (confidence >= min_confidence){
						numofStrongRules++;
						if(strong_rules.find(ruleHeader->first) == strong_rules.end()){
							u_map_vector ruleBodies;
							ruleBodies[leftItems] = confidence;
							strong_rules[ruleHeader->first] = ruleBodies;
						}else{
							strong_rules[ruleHeader->first][leftItems] = confidence;
						}
					}
				}
				m++;
				currentLevel = m;
				/*generate rule head with length + 1*/
				ruleHeaders = generateCandidates(ruleHeaders);
			}
		}
	}
	printf("Num of associations rules:%d\n",numofStrongRules);
	printf("End finding strong rules ");
	timetick(lastTimeTick);
}

/**
 * Input the data and store them to the memory
 */
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
	/*parse the file line by line and initialize the C1*/
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
	/*clsoe file*/
	inputFile.close();
	num_transactions = DB.size();
	printf("There are total %d transactions.\n",num_transactions);
	candidates_k[1] = candidates;
	large_itemsets_k[1] = generateLargeItemsets(candidates);
	if (debug_flag) {
		printf("Finshed");
		timetick(lastTimeTick);
	}
	return true;
}

/**
 * Generate large frequent itemsets based on the threshold and candidates
 */
u_map_vector generateLargeItemsets(u_map_vector &candidates){
	if (debug_flag)printf("Start generating (%d)largeItemsets...\n",currentLevel);
	u_map_vector largeItemsets;
	/*check the the support and min_tresh value*/
	for(u_map_vector::iterator itemset = candidates.begin();itemset!=candidates.end();++itemset){
		if(((double)(itemset->second/num_transactions)) >= min_support ){
			largeItemsets[itemset->first] = itemset->second;
		}
	}
	if (debug_flag)printf("End generating (%d)largeItemsets!\n",currentLevel);

	return largeItemsets;
}

/**
 * Generage candidates with lenght k+1
 */
u_map_vector generateCandidates(u_map_vector &largeItemsets){
	if (debug_flag)printf("Start generating (k=%d) candidates...\n",currentLevel);
	u_map_vector candidates;
	u_map_vector::iterator largeItemsets_end = largeItemsets.end();
	/*check each pair of the candidates*/
	for(u_map_vector::iterator itemsetA = largeItemsets.begin();itemsetA!=largeItemsets_end;++itemsetA){
		for(u_map_vector::iterator itemsetB = itemsetA;itemsetB!=largeItemsets_end;++itemsetB){
			if (itemsetA!=itemsetB){
				bool flag = true;
				/*check first k-1 item*/
				for (int i = 0; i < currentLevel-2;i++){
					if (itemsetA->first[i]!=itemsetB->first[i]){
						flag = false;
						break;
					}
				}
				if (flag){
					/*construct new candidate*/
					vector<int> candidate = itemsetA->first;
					candidate.push_back(itemsetB->first.back());
					if (candidate[currentLevel-2] > candidate[currentLevel-1]){
						int buffer = candidate[currentLevel-2];
						candidate[currentLevel-2]= candidate[currentLevel-1];
						candidate[currentLevel-1] = buffer;

					}
					/*new prune with check last two itemset*/
					if (currentLevel > 2){
						vector<int> two_tmp; two_tmp.push_back(candidate[currentLevel-2]);two_tmp.push_back(candidate[currentLevel-1]);
						if (large_itemsets_k[2].find(two_tmp)!=large_itemsets_k[2].end()) {candidates[candidate] = 0;}
					}else{
						candidates[candidate] = 0;
					}
					/*Old Prune Part in paper
					sort(candidate.begin(),candidate.end());
					candidates[candidate] = 0;
					Prune
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
					Old Prune Part End*/
				}
			}
		}
	}
	if (debug_flag)printf("End generating (k=%d) - (%d)candidates!\n",currentLevel,(int)candidates.size());
	return candidates;
}

/**
 * Scan database to and count the support
 */
void foreachDB(u_map_vector &candidates){
	if (debug_flag)printf("Start scaning database...");
	int num = 0;
	for(int i = 0 ; i < num_transactions;i++){
		vector<int> transaction = DB[i];
		if (transaction.size()<currentLevel) continue;
		for (int j = 0 ; j <  transaction.size()-currentLevel+1;j++){
			vector<int> tmp;tmp.push_back(DB[i][j]);
			/*fast nest check*/
			nestCheckSubset(transaction,1,tmp,j);
		}
	}
	if (debug_flag)printf("End caning database!\n");
}

/**
 * Nest check subset with given transaction
 */
void nestCheckSubset(vector<int> &transaction,int level,vector<int>&pre,int idx){
	/*from lower level to higher level*/
	if (level < currentLevel){
		if (large_itemsets_k[level].find(pre) == large_itemsets_k[level].end()) return;
		for (int j = idx+1;j<transaction.size()-currentLevel+level+1;j++){
			vector<int> tmp = pre;
			tmp.push_back(transaction[j]);
			nestCheckSubset(transaction,level+1,tmp,j);
		}
	}else{
		/*if in candidates then add count*/
		if(candidates_k[currentLevel].find(pre)!=candidates_k[currentLevel].end()){
			candidates_k[currentLevel][pre]++;
		}
	}
}

/**
 * Return the vector with sub/base
 */
vector<int> vectorRemoveOtherVector(vector<int> base, vector<int> sub){
	vector<int> newVector;
	int idx = 0;
	for (int i = 0 ; i < sub.size(); i++){
		for(;idx<base.size();idx++){
			if (sub[i] != base[idx]){
				newVector.push_back(base[idx]);
			}else{
				idx++;
				break;
			}
		}
	}
	for(;idx<base.size();idx++){
		newVector.push_back(base[idx]);
	}
	return newVector;
}


/**
 * Util functions for testing
 */
void timetick(clock_t since){
	clock_t current = clock();
	double elapsed_secs = double(current - since) / CLOCKS_PER_SEC;
	lastTimeTick = current;
	printf("spent %f s\n\n",elapsed_secs);
}

void output(u_map_vector &map){
	for(u_map_vector::iterator itemset = map.begin();itemset!=map.end();++itemset){
		output(itemset->first);
		printf("(%.2f)\n",(double)itemset->second/num_transactions);
	}
}

void output(const vector<int> &trans){
	for(int i = 0 ; i < trans.size();i++){
		printf("%d",trans[i]);
		if (i !=  (trans.size()-1)){
			printf(",");
		}
	}
}

void output(FILE * pFile,const vector<int> &trans){
	for(int i = 0 ; i < trans.size();i++){
		fprintf(pFile,"%d",trans[i]);
		if (i !=  (trans.size()-1)){
			fprintf(pFile,", ");
		}
	}
}

void output(u_map_vector_rules &rules){
	u_map_vector_rules::iterator rules_end = rules.end();
	for (u_map_vector_rules::iterator head=rules.begin();head!=rules_end;++head){
		vector<int> headVector = head->first;
		u_map_vector::iterator bodies_end = head->second.end();
		for(u_map_vector::iterator body = head->second.begin();body!=bodies_end;++body){
			vector<int> bodyVector = body->first;
			output(headVector);printf("->");output(bodyVector);printf("(%f)\n",body->second);
		}
	}
}


/**
 * Output to file
 */
void output(){
	FILE * pFile;
	pFile = fopen ("output(in%).txt","w");
	if (argv_code == 3){printf("Output all large frequent itemsets and strong rules to output.txt\n");}
	if (argv_code == 2){printf("Output all strong rules to output.txt\n");}
	if (argv_code == 1){printf("Output all large frequent itemsets to output.txt\n");}

	if(argv_code == 1 || argv_code == 3){
		for(int i = 1 ; i <= largestLength ; i ++){
			u_map_vector current = large_itemsets_k[i];
			u_map_vector::iterator current_end = current.end();
			for(u_map_vector::iterator itemset = current.begin();itemset!=current_end;++itemset){
				output(pFile,itemset->first);
				fprintf(pFile,"(%.2f)\n",(double)itemset->second/num_transactions*100);
			}
		}
	}else if  (argv_code == 2 || argv_code == 3){
		u_map_vector_rules::iterator rules_end = strong_rules.end();
			for (u_map_vector_rules::iterator head=strong_rules.begin();head!=rules_end;++head){
				vector<int> headVector = head->first;
				double head_sup = large_itemsets_k[(int)headVector.size()][headVector]/num_transactions;
				u_map_vector::iterator bodies_end = head->second.end();
				for(u_map_vector::iterator body = head->second.begin();body!=bodies_end;++body){
					vector<int> bodyVector = body->first;
					output(pFile,headVector);fprintf(pFile," -> ");output(pFile,bodyVector);fprintf(pFile," (%.2f,%.2f)\n",body->second*head_sup*100,body->second*100);
				}
			}
	}else{
		return;
	}
	fclose(pFile);
	printf("Success\n\n");
}
