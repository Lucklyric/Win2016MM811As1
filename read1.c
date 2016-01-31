#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct database_t{
  int sizeOfDatabase;
  int maxEntry;
  int *transaction;
};

int main(int argc, char const *argv[]){
  FILE *dataFile;
  dataFile = fopen(argv[1],"r");
  int item;
  database_t* dataBase = (database_t*)malloc(sizeof(databse_t)*100000*100);
  while(fscanf(dataFile,"%d",&item) != EOF){
    dataBase.sizeOfDatabase++;
    dataBbase_*;
  }

}
