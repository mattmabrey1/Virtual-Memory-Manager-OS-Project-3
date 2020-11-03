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
typedef struct TLB_entry{
	int page_num;
	int frame_num;

} TLB_entry;

// !!! TEMP !!! struct to test FIFO with my page replacement solution
typedef struct Node Node;
 
typedef struct Node{

	int frame_num;
	Node *next;
} Node;

// CONSULT_TLB  NOT YET IMPLEMENTED
// Consult the Translation Look-Aside Buffer before consulting page table to increase speed
short consult_TLB(int page_num, TLB_entry **TLB);

// Consult page table for frame number in physical memory
short consult_page_table(int page_num, int *page_table);

// Get and store page in physical memory if the page doesn't currently have a frame in memory
short page_fault(int page_num, FILE** backing_store, int *open_frames, signed char **physical_memory);


int main(int argc, char *argv[])
{
	// Buffer to hold address that is read in
	char address_buffer[7];
	
	// Initialize array of char pointers to simulate physical memory
	signed char *physical_memory[FRAME_COUNT];

	// Keep track of which frames in physical memory have been filled
	int open_frames[FRAME_COUNT];

	// Page table initialized with -1 to signify if no frame number is available for that page
	int page_table[PAGE_TABLE_SIZE] = { [0 ... 255] = -1};

	TLB_entry* TLB = (TLB_entry *)malloc(sizeof(TLB_entry) * TLB_SIZE);

	int frame_num, page_faults = 0, tlb_hits = 0 ;

	unsigned int address, page_num, offset;

	/* Check for user input */
	if(argc < 2){
		printf("You must enter the name of an input file!\n");
		return 1;
	}
	
	// Read logical addresses from input file
	FILE* input_file = fopen(argv[1], "r");

	// Read memory images from BACKING_STORE
	FILE* backing_store = fopen("BACKING_STORE.bin", "r");


	// Check input file opening for error
	if(input_file == NULL){

		fprintf(stderr, "Error opening input file!\n");
		return 1;
	}
	// Check backing store file opening for error
	else if(backing_store == NULL){
		fprintf(stderr, "Error opening backing store file!\n");
		return 1;
	}
	else{
			
		// Read in logical addresses until End-of-File is reached
		while(fgets(address_buffer, 7, input_file)){

			// Convert string to logical address integer
			address = atoi(address_buffer);

			// Bit AND mask (1111 1111 0000 0000) to get the highest 8 bits
			page_num = address & 65280;

			// Bit AND mask (0000 0000 1111 1111) to get the lowest 8 bits
			offset = address & 255;

			// Shift the page num removing the lowest 8 bits which are zero after bit AND mask
			page_num = page_num >> 8;
			
			/**
			*   NOT YET IMPLEMENTED ---- CONSULT TLB BEFORE CONSULTING PAGE TABLE
			*
			*/

			frame_num = consult_page_table(page_num, page_table);

			// We get a page fault if no memory frame was found for the given page
			if(frame_num == -1){

				// Load in page from memory image in BACKING_STORE
				frame_num = page_fault(page_num, &backing_store, open_frames, physical_memory);

				// Store the frame number generated
				page_table[page_num] = frame_num;
			}

			// If no free memory could be found in page_fault, we must do page replacement
			if(frame_num == -1){
				printf("No available frame found!\n");

				/**
				*   NOT YET IMPLEMENTED ---- PAGE REPLACEMENT
				*
				*/
			}
			else{
		
				printf("Virtual address: %d Physical address: %d ", address, ((frame_num * 256) + offset));
				printf("Value: %d\n", physical_memory[frame_num][offset]);
				//printf("Frame Num: %d   Offset: %d\n", frameNum, offset);
			}
		}

		

		

	}
	
	
	fclose(input_file);
	fclose(backing_store);

	return 0;
}


short consult_TLB(int page_num, TLB_entry **TLB){

	// Check each TLB object to see if the page number exists for fast access
	for(int i = 0; i < TLB_SIZE; ++i){

		if(TLB[i]->page_num == page_num){
			return TLB[i]->frame_num;
		}
	}

	
	return -1;
}


// Consult page table for frame number in physical memory (slower than TLB)
short consult_page_table(int page_num, int *page_table){


	if(page_table[page_num] != -1){
		return page_table[page_num];
	}

	return -1;

}


// When we get a page fault we load a page into physical memory from memory image in BACKING_STORE
// by setting file position to the beginning of that page's addresses (each page 256 bytes) and
// then reading in the page then adding on a null terminator
short page_fault(int page_num, FILE** backing_store, int *open_frames, signed char **physical_memory){

	// The buffer will hold an entire page + 1 for null character
	char buffer[FRAME_SIZE + 1];
	
	fseek(*backing_store, page_num * 256, SEEK_SET);

	fread(buffer, FRAME_SIZE, 1, *backing_store);

	buffer[256] = '\0';

	// Look for an empty frame in memory to fill with a page
	for(int frame_num = 0; frame_num < FRAME_COUNT; ++frame_num){
	
		if(open_frames[frame_num] == 0){
		
			// Set the frame to occupied
			open_frames[frame_num] = 1;

			physical_memory[frame_num] = (char *)malloc(sizeof(char) * FRAME_SIZE + 1);

			// Copy buffer one by one since some data is 0 which will be considered null and stop strdup()
			// and other functions from copying the data
			for(int i = 0; i < FRAME_SIZE; i++){
				physical_memory[frame_num][i] = buffer[i];
			}

			return frame_num;
		}
	}

	// An empty frame was unable to be found in physical memory, page replacement must be done !!!!
	printf("\nPhysical Memory FULL!\n");

	return -1;
}







