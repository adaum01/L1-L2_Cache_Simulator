#ifndef SIM_CACHE_H
#define SIM_CACHE_H
#include <stdlib.h>
#include <vector>

typedef 
struct {
   uint32_t BLOCKSIZE;
   uint32_t L1_SIZE;
   uint32_t L1_ASSOC;
   uint32_t L1_SETS = 0;
   uint32_t L1_INDEX = 0;
   uint32_t L1_BLOCKOFFSET = 0;
   uint32_t L2_SIZE;
   uint32_t L2_ASSOC;
   uint32_t L2_SETS = 0;
   uint32_t L2_INDEX = 0;
   uint32_t L2_BLOCKOFFSET = 0;
   uint32_t PREF_N;
   uint32_t PREF_M;
} cache_params_t;

typedef struct {
   uint32_t TAG = 0;
   uint32_t INDEX = 0;
} blockAddress;

int memTraffic = 0;

class block{
   public:
      bool valid = 0; 
      bool dirty = 0;
      uint32_t LRU = 0;
      uint32_t tag = 0;
};

class cache{
   public:
      uint32_t LEVEL = 0;
      uint32_t SETS = 0;
      uint32_t ASSOC = 0;
      uint32_t INDEX_BITS = 0;
      uint32_t BLOCK_OFFSET = 0;
      std::vector<std::vector <block>> *storage;
      cache* nextCache = nullptr;

      //measurement vals
      int read = 0;
      int readMiss = 0;
      int write = 0;
      int writeMiss = 0;
      int writeback = 0;
      
      cache(uint32_t lvlVal, uint32_t setsVal, uint32_t assocVal, uint32_t indexBitVal, uint32_t blockOffsetVal){
         LEVEL = lvlVal;
         SETS = setsVal;
         ASSOC = assocVal;
         INDEX_BITS = indexBitVal;
         BLOCK_OFFSET = blockOffsetVal;

         if(SETS != 0 && ASSOC != 0){
            storage = new std::vector<std::vector<block>>(SETS, std::vector<block>(ASSOC));

            //initialize LRU of all blocks in all sets
            for(uint32_t i=0; i<SETS; i++){
               for(uint32_t j=0; j<ASSOC; j++){
                  storage->at(i)[j].LRU = j;
               }
            }
         }
      }

      blockAddress decodeAddr(uint32_t addrVal){
         blockAddress retVal;
         uint32_t indexMask = 0;
      
         //index bits - create bit mask for index bits, AND with address
         for(uint32_t i=0; i<INDEX_BITS; i++){
            indexMask = indexMask << 1;
            indexMask += 1;
         }
         retVal.INDEX = (addrVal>>BLOCK_OFFSET) & indexMask;

         //tag bits - bit shift to remove index+blockoffset bits
         retVal.TAG = addrVal >> (INDEX_BITS + BLOCK_OFFSET);

         return retVal;
      }

      uint32_t recodeAddr(uint32_t tagVal, uint32_t indexVal){
         uint32_t retVal = 0;
         
         retVal = tagVal << INDEX_BITS;   //shift tag to make space for index
         retVal = retVal | indexVal;      //OR index with 0's
         retVal = retVal << BLOCK_OFFSET; //shift to include block offset bits (empty)

         return retVal;
      }

      void request(char RW, uint32_t addr){
         blockAddress decoded = decodeAddr(addr);
         uint32_t tagVal = decoded.TAG;
         uint32_t indexVal = decoded.INDEX;

         bool miss = 1;
         uint32_t hitLoc = 0;
         uint32_t LRULoc = 0;
         
         //loops through set to check for hit or record LRU
         for(uint32_t i=0; i<ASSOC; i++){
            if(storage->at(indexVal)[i].tag == tagVal){   //if tags match, no miss
               miss = 0;
               hitLoc = i;
            }
            if(storage->at(indexVal)[i].LRU == ASSOC-1){   //note LRU block location
               LRULoc = i;
            }
         }

         //HIT BRANCH: if write - set dirty bit to 1 and return, if read - just return
         if(!miss){
            if(RW == 'w'){
               storage->at(indexVal)[hitLoc].dirty = 1;
               write++;
            }
            else{
               read++;
            }
            
            //update LRU on hit
            for(uint32_t i=0; i<ASSOC; i++){
               if(storage->at(indexVal)[i].LRU < storage->at(indexVal)[hitLoc].LRU){  //if any block in set has lower LRU count than new block, increment
                  storage->at(indexVal)[i].LRU++;
               }
            }
            storage->at(indexVal)[hitLoc].LRU = 0;  //update hit block to MRU
            
            return;
         }

         //MISS BRANCH
         if(miss){
            if(RW == 'w'){
               write++;
               writeMiss++;
            }
            else{
               read++;
               readMiss++;
            }
         }
         //WRITEBACK BRANCH: if LRU block is dirty, write to next level
         if(storage->at(indexVal)[LRULoc].dirty){
            if(this->nextCache != nullptr){                                               //write request to next level
               uint32_t wbAddr = recodeAddr(storage->at(indexVal)[LRULoc].tag, indexVal);
               this->nextCache->request('w', wbAddr);                     
               writeback++;
            }
            else{                      //if null, then main memory (pretend write)
               writeback++;
               memTraffic++;
            }
            storage->at(indexVal)[LRULoc].dirty = 0;  //reset dirty bit, matches next level
         }
         
         //need to read in block from next level
         if(this->nextCache != nullptr){           //if null, then main memory (pretend read)
            this->nextCache->request('r', addr);   //read request to next level
         }
         else{
            memTraffic++;
         }
         
         //install new block in set
         storage->at(indexVal)[LRULoc].valid = 1;
         storage->at(indexVal)[LRULoc].dirty = 0;
         storage->at(indexVal)[LRULoc].tag = tagVal;

         //update LRU
         for(uint32_t i=0; i<ASSOC; i++){
            if(storage->at(indexVal)[i].LRU < storage->at(indexVal)[LRULoc].LRU){  //if any block in set has lower LRU count than new block, increment
               storage->at(indexVal)[i].LRU++;
            }
         }
         storage->at(indexVal)[LRULoc].LRU = 0;  //update new block to MRU

         //perform read/write on new block
         if(RW == 'w'){
            storage->at(indexVal)[LRULoc].dirty = 1;
         }

         return;
      }

      void printCache(){
         std::vector<uint32_t> tagArr;
         std::vector<bool> dirtyArr;
         tagArr.resize(ASSOC);
         dirtyArr.resize(ASSOC);

         printf("===== L%d contents =====\n", LEVEL);
         for(uint32_t i=0; i<SETS; i++){                                      //loop through all sets in cache
            for(uint32_t j=0; j<ASSOC; j++){                                  //loop through every way in set
               tagArr.at(storage->at(i)[j].LRU) = storage->at(i)[j].tag;      //store tag+dirty bit in MRU->LRU order
               dirtyArr.at(storage->at(i)[j].LRU) = storage->at(i)[j].dirty;
            }
            
            printf("set\t%d:", i);
            for(uint32_t j=0; j<ASSOC; j++){
               printf("\t%x", tagArr[j]);
               if(dirtyArr[j] == 1){
                  printf(" D");
               }
            }
            printf("\n");
         }

         if(nextCache != nullptr){
            printf("\n");
            nextCache->printCache();
         }

         return;
      }

      void printMeasurements(){
         //print measurements
         printf("\n===== Measurements =====\n");
         printf("a. L1 reads:                   %d\n", read);
         printf("b. L1 read misses:             %d\n", readMiss);
         printf("c. L1 writes:                  %d\n", write);
         printf("d. L1 write misses:            %d\n", writeMiss);
         printf("e. L1 miss rate:               %0.4f\n", ((double)readMiss+(double)writeMiss)/((double)read+(double)write));
         printf("f. L1 writebacks:              %d\n", writeback);
         printf("g. L1 prefetches:              %d\n", 0);
         if(nextCache != nullptr){
            printf("h. L2 reads (demand):          %d\n", nextCache->read);
            printf("i. L2 read misses (demand):    %d\n", nextCache->readMiss);
            printf("j. L2 reads (prefetch):        %d\n", 0);
            printf("k. L2 read misses (prefetch):  %d\n", 0);
            printf("l. L2 writes:                  %d\n", nextCache->write);
            printf("m. L2 write misses:            %d\n", nextCache->writeMiss);
            printf("n. L2 miss rate:               %0.4f\n", ((double)nextCache->readMiss/(double)nextCache->read));
            printf("o. L2 writebacks:              %d\n", nextCache->writeback);
            printf("p. L2 prefetches:              %d\n", 0);
         }
         else{
            printf("h. L2 reads (demand):          0\n");
            printf("i. L2 read misses (demand):    0\n");
            printf("j. L2 reads (prefetch):        0\n");
            printf("k. L2 read misses (prefetch):  0\n");
            printf("l. L2 writes:                  0\n");
            printf("m. L2 write misses:            0\n");
            printf("n. L2 miss rate:               0.0000\n");
            printf("o. L2 writebacks:              0\n");
            printf("p. L2 prefetches:              0\n");
         }
         printf("q. memory traffic:             %d\n", memTraffic);
      }
};

#endif
