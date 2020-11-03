/*
Nikola Kilibarda and Matthew Mabrey
CSC-345-01 Operating Systems
Dr. Yoon
Project 3
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define FRAME_COUNT 128		// How many frames in physical memory
#define FRAME_SIZE 256          // The size of each frame in physical memory
#define PAGE_TABLE_SIZE 256	// Size of the logical memory page table
#define TLB_SIZE 16		// Size of the Translation Look-Aside Buffer

int get_frame_num(int page_num);

int page_fault(int page_num, FILE** backing_store);

signed char physical_memory[FRAME_COUNT][FRAME_SIZE];

int page_table[PAGE_TABLE_SIZE];		// Each index stores the frame# the page is loaded into
int frames_taken[FRAME_COUNT];			// Each index stores the page# that is in that frame
int valid[PAGE_TABLE_SIZE];			// Used to tell if that paged is currently in physical memory

int TLB_pages[TLB_SIZE];
int TLB_frames[TLB_SIZE];

char buffer[FRAME_SIZE];			// Buffer for reading in entire pages

int page_faults = 0, tlb_hits = 0, tlb_hit = 0, tlb_idx = 0, replace_idx = 0;

FILE *out1;
FILE *out2;
FILE *out3;

int main(int argc, char *argv[])
{
	int address, page_num, offset, frame_num;

	char address_buffer[7];

	for(int i = 0; i < PAGE_TABLE_SIZE; i++){
		page_table[i] = -1;
		frames_taken[i] = -1;
		valid[i] = 0;
	}

	for(int i = 0; i < TLB_SIZE; i++){
		TLB_pages[i] = -1;
		TLB_frames[i] = -1;
	}

	/* Check for user input */
	if(argc < 2){
		printf("You must enter the name of an input file!\n");
		return 1;
	}
	
	// Read logical addresses from input file
	FILE* input_file = fopen(argv[1], "r");

	// Read memory images from BACKING_STORE
	FILE* backing_store = fopen("BACKING_STORE.bin", "r");

	/*open the file write the logical addressess*/
	out1 = fopen("out1.txt", "wt"); 
	/*open the file to write the physical addressess*/
	out2 = fopen("out2.txt", "wt"); 
	/*open the file to write the vals*/
	out3 = fopen("out3.txt", "wt");

	if(input_file == NULL ){
		fprintf(stderr, "Error opening input file!\n");
		return -1;
	}

	if(backing_store == NULL){
		fprintf(stderr, "Error opening backing store!\n");
		return -1;
	}

	// Read in logical addresses until End-of-File is reached
	while(fgets(address_buffer, 7, input_file) != NULL){

		address = atoi(address_buffer);

		// Bit AND mask to get the highest 8 bits then shift to remove lowest 8 bits
		page_num = (address & 65280) >> 8;

		// Bit AND mask to get the lowest 8 bits
		offset = address & 255;
			
		frame_num = get_frame_num(page_num);

		// PAGE FAULT if no frame num was found in TLB or page table
		if(frame_num == -1){
			frame_num = page_fault(page_num, &backing_store);
		}
			
		// !! CHANGE THIS TO PRINT TO FILE <-----------------------!!!
		//	printf("Virtual address: %d Physical address: %d ", address, ((frame_num * 256) + offset));
		//	printf("Value: %d\n", physical_memory[frame_num][offset]);
		fprintf(out1, "%d\n", address);
		fprintf(out2,"%d\n", ((frame_num * 256) + offset));
		fprintf(out3, "%d\n", physical_memory[frame_num][offset]);

		// If page wasn't in TLB add it using FIFO
		if(tlb_hit == 0){

			TLB_pages[tlb_idx] = page_num;
			TLB_frames[tlb_idx] = frame_num;
			
			if(tlb_idx < (TLB_SIZE - 1)){
				tlb_idx++;
			}
			else{
				tlb_idx = 0;
			}
			
		}
	}


	printf("\n(with page replacement, using 128 frames)\n");
	printf("Page faults = %d / 1000, %.3f\n", page_faults, ((double)page_faults/1000));
	printf("TLB hits = %d / 1000, %.3f\n", tlb_hits, ((double)tlb_hits/1000));

	fclose(input_file);
	fclose(backing_store);
	fclose(out1);
	fclose(out2);
	fclose(out3);
	 
	
	return 0;
}


// Consult TLB and page table to try to find if this page has a frame allocated already
int get_frame_num(int page_num){

	tlb_hit = 0;

	// If this page isn't in physical memory don't look for it in TLB or page table
	if(valid[page_num] == 0){
		return -1;
	}

	// ----CONSULT TLB------
	// Check each TLB object starting from youngest to see if the page number exists for fast access
	for(int i = 0; i < TLB_SIZE; i++){

		if(TLB_pages[i] == page_num){
			tlb_hits++;
			tlb_hit = 1;
			return TLB_frames[i];
		}
	}

	
	// ----CONSULT PAGE TABLE------
	return page_table[page_num];

}


// If no frame was found for the current page, look for an empty one or replace using FIFO
int page_fault(int page_num, FILE** backing_store){

	page_faults++;

	int frame_num = replace_idx;

	// If frame already associated with page, make victim page invalid
	if(frames_taken[frame_num] != -1){
		valid[frames_taken[frame_num]] = 0;
	}

	page_table[page_num] = frame_num;
	frames_taken[frame_num] = page_num;
	
	// Read in page from memory image
	fseek(*backing_store, page_num * FRAME_SIZE, SEEK_SET);

	fread(buffer, FRAME_SIZE, 1, *backing_store);

	// Copy buffer one by one since some data is 0 which will be considered null and stop strdup()/strcpy()
	for(int i = 0; i < FRAME_SIZE; i++){
		physical_memory[frame_num][i] = buffer[i];
	}

	valid[page_num] = 1;

	if(replace_idx < (FRAME_COUNT - 1)){
		replace_idx++;
	}
	else{
		replace_idx = 0;
	}
	
	return frame_num;
}

