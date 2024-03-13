#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>
#include <vector>
#include "sim.h"

// Input format
// ./sim 16 4096 4 65536 8 traceFile.txt
// ./sim <blocksize> <L1_size> <L1_associativity> <L2_size> <L2_associativity> <trace_file>

// Starter code -------------------------------------------------------------------------------------------------------------------------
int main (int argc, char *argv[]) {
   FILE *fp;			// File pointer.
   char *trace_file;		// This variable holds the trace file name.
   cache_params_t params;	// Look at the sim.h header file for the definition of struct cache_params_t.
   char rw;			// This variable holds the request's type (read or write) obtained from the trace.
   uint32_t addr;		// This variable holds the request's address obtained from the trace.
				// The header file <inttypes.h> above defines signed and unsigned integers of various sizes in a machine-agnostic way.  "uint32_t" is an unsigned integer of 32 bits.

   // Exit with an error if the number of command-line arguments is incorrect.
   if (argc != 7) {
      printf("Error: Expected 6 command-line arguments but was provided %d.\n", (argc - 1));
      exit(EXIT_FAILURE);
   }
    
   params.BLOCKSIZE = (uint32_t) atoi(argv[1]);
   params.L1_SIZE   = (uint32_t) atoi(argv[2]);
   params.L1_ASSOC  = (uint32_t) atoi(argv[3]);
   params.L2_SIZE   = (uint32_t) atoi(argv[4]);
   params.L2_ASSOC  = (uint32_t) atoi(argv[5]);
   trace_file       = argv[6];

   params.PREF_N    = 1;
   params.PREF_M    = 1;

   // Open the trace file for reading.
   fp = fopen(trace_file, "r");
   if (fp == (FILE *) NULL) {
      // Exit with an error if file open failed.
      printf("Error: Unable to open file %s\n", trace_file);
      exit(EXIT_FAILURE);
   }
    
   // Print simulator configuration.
   printf("===== Simulator configuration =====\n");
   printf("BLOCKSIZE:  %u\n", params.BLOCKSIZE);
   printf("L1_SIZE:    %u\n", params.L1_SIZE);
   printf("L1_ASSOC:   %u\n", params.L1_ASSOC);
   printf("L2_SIZE:    %u\n", params.L2_SIZE);
   printf("L2_ASSOC:   %u\n", params.L2_ASSOC);
   printf("trace_file: %s\n", trace_file);
   //printf("===================================\n");
   printf("\n");
// Starter Code -------------------------------------------------------------------------------------------------------------------------

   // Calculate L1/L2 cache sets, index bits, and block offset
   if(params.L1_SIZE != 0 && params.L1_ASSOC != 0){
      params.L1_SETS = params.L1_SIZE/(params.L1_ASSOC*params.BLOCKSIZE);
      params.L1_INDEX = log2(params.L1_SETS);
      params.L1_BLOCKOFFSET = log2(params.BLOCKSIZE);
   }

   if(params.L2_SIZE != 0 && params.L2_ASSOC != 0){
      params.L2_SETS = params.L2_SIZE/(params.L2_ASSOC*params.BLOCKSIZE);
      params.L2_INDEX = log2(params.L2_SETS);
      params.L2_BLOCKOFFSET = log2(params.BLOCKSIZE);
   }

   // Instantiate L1, L2 caches, link L1 to L2
   cache L1 = cache(1, params.L1_SETS, params.L1_ASSOC, params.L1_INDEX, params.L1_BLOCKOFFSET);
   cache L2 = cache(2, params.L2_SETS, params.L2_ASSOC, params.L2_INDEX, params.L2_BLOCKOFFSET);
   if(params.L2_SETS != 0 && params.L2_ASSOC != 0){
      L1.nextCache = &L2;
   }

// Starter Code -------------------------------------------------------------------------------------------------------------------------
   // Read requests from the trace file and echo them back.
   while (fscanf(fp, "%c %x\n", &rw, &addr) == 2) {	// Stay in the loop if fscanf() successfully parsed two tokens as specified.
      if (rw != 'r' && rw != 'w'){
         printf("Error: Unknown request type %c.\n", rw);
	      exit(EXIT_FAILURE);
      }
// Starter Code -------------------------------------------------------------------------------------------------------------------------

      L1.request(rw, addr);   // Issue request to the L1 cache
    }
   
//Print after processing
   L1.printCache();
   L1.printMeasurements();
   return(0);
}
