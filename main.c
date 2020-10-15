/*
Nikola Kilibarda and Matthew Mabrey
CSC-345-01 Operating Systems
Dr. Yoon
Project 3
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>

#define FRAME_COUNT 256		// How many frames in physical memory
#define FRAME_SIZE 256		// The size of each frame in physical memory
#define PAGE_TABLE_SIZE 256	// Size of the logical memory page table
#define TLB_SIZE 15		// Size of the Translation Look-Aside Buffer

// !!! TEMP !!! structure to contain the page number to translate to frame number
typedef struct _TLB_entry{
	int pageNum;
	int frameNum;

} TLB_entry;

// CONSULT_TLB  NOT YET IMPLEMENTED
// Consult the Translation Look-Aside Buffer before consulting page table to increase speed
short consult_TLB(int pageNum, TLB_entry **TLB);

// Consult page table for frame number in physical memory
short consult_page_table(int pageNum, int *pageTable);

// Get and store page in physical memory if the page doesn't currently have a frame in memory
short page_fault(int pageNum, FILE** backingStore, int *framesOpen, signed char **physicalMemory);


int main(int argc, char *argv[])
{
	// Buffer to hold address that is read in
	char logicalAddress[7];
	
	// Initialize array of char pointers to simulate physical memory
	signed char *physicalMemory[FRAME_COUNT];

	// Keep track of which frames in physical memory have been filled
	int openFrames[FRAME_COUNT];

	// Page table initialized with -1 to signify if no frame number is available for that page
	int pageTable[PAGE_TABLE_SIZE] = { [0 ... 255] = -1};

	TLB_entry* TLB = (TLB_entry *)malloc(sizeof(TLB_entry) * TLB_SIZE);

	int frameNum, address, pageFaults = 0;

	unsigned int pageNum, offset;

	/* Check for use input */
	if(argc < 2){
		printf("You must enter the name of an input file!\n");
		return 1;
	}
	
	// Read logical addresses from input file
	FILE* inputFile = fopen(argv[1], "r");

	// Read memory images from BACKING_STORE
	FILE* backingStore = fopen("BACKING_STORE.bin", "r");


	// Check input file opening for error
	if(inputFile == NULL){

		fprintf(stderr, "Error opening input file!\n");
		return 1;
	}
	// Check backing store file opening for error
	else if(backingStore == NULL){
		fprintf(stderr, "Error opening backing store binary file!\n");
		return 1;
	}
	else{
			
		// Read in logical addresses until End-of-File is reached
		while(fgets(logicalAddress, 7, inputFile)){

			// Convert string to logical address integer
			address = atoi(logicalAddress);

			// Bit AND mask (1111 1111 0000 0000) to get the highest 8 bits
			pageNum = address & 65280;

			// Bit AND mask (0000 0000 1111 1111) to get the lowest 8 bits
			offset = address & 255;

			// Shift the page num removing the lowest 8 bits which are zero after bit AND mask
			pageNum = pageNum >> 8;
			
			/**
			*   NOT YET IMPLEMENTED ---- CONSULT TLB BEFORE CONSULTING PAGE TABLE
			*
			*/

			frameNum = consult_page_table(pageNum, pageTable);

			// We get a page fault if no memory frame was found for the given page
			if(frameNum == -1){

				// Load in page from memory image in BACKING_STORE
				frameNum = page_fault(pageNum, &backingStore, openFrames, physicalMemory);

				// Store the frame number generated
				pageTable[pageNum] = frameNum;
			}

			// If no free memory could be found in page_fault, we must do page replacement
			if(frameNum == -1){
				printf("No available frame found!\n");

				/**
				*   NOT YET IMPLEMENTED ---- PAGE REPLACEMENT
				*
				*/
			}
			else{
		
				printf("Virtual address: %d Physical address: %d ", address, ((frameNum * 256) + offset));
				printf("Value: %d\n", physicalMemory[frameNum][offset]);
				//printf("Frame Num: %d   Offset: %d\n", frameNum, offset);
			}
		}

		

		

	}
	
	
	fclose(inputFile);
	fclose(backingStore);

	return 0;
}


short consult_TLB(int pageNum, TLB_entry **TLB){

	// Check each TLB object to see if the page number exists for fast access
	for(int i = 0; i < TLB_SIZE; ++i){

		if(TLB[i]->pageNum == pageNum){
			return TLB[i]->frameNum;
		}
	}

	
	return -1;
}


// Consult page table for frame number in physical memory (slower than TLB)
short consult_page_table(int pageNum, int *pageTable){


	if(pageTable[pageNum] != -1){
		return pageTable[pageNum];
	}

	return -1;

}


// Load page into physical memory from memory image in BACKING_STORE if it doesn't yet exist there
short page_fault(int pageNum, FILE** backingStore, int *openFrames, signed char **physicalMemory){

	// The buffer will hold an entire page + 1 for null character
	char buffer[FRAME_SIZE + 1];

	// Set file position to the beginning of that page's addresses (each page 256 bytes)
	fseek(*backingStore, pageNum * 256, SEEK_SET);

	// Read in a 256 byte page of memory from BACKING_STORE
	fread(buffer, FRAME_SIZE, 1, *backingStore);

	// Make sure string is null terminated
	buffer[256] = '\0';

	// Look for an empty frame in memory to fill with a page
	for(int frameNum = 0; frameNum < FRAME_COUNT; ++frameNum){
	
		// If empty frame, load with current page data
		if(openFrames[frameNum] == 0){
		
			// Set the frame to occupied
			openFrames[frameNum] = 1;

			// Allocate space in memory for the string
			physicalMemory[frameNum] = (char *)malloc(sizeof(char) * FRAME_SIZE + 1);


			// Copy buffer one by one since some data is 0 which will be considered null and stop strdup()
			// and other functions from copying the data
			for(int i = 0; i < FRAME_SIZE; i++){
				physicalMemory[frameNum][i] = buffer[i];
			}

			// Return the frame number where the page is now stored
			return frameNum;
		}
	}

	// An empty frame was unable to be found in physical memory, page replacement must be done !!!!
	printf("\nPhysical Memory FULL!\n");

	return -1;
}







