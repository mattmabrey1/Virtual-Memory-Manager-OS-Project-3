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
#define TLB_SIZE 16		// Size of the Translation Look-Aside Buffer

typedef enum {false, true} bool;

// !!! TEMP !!! struct to test FIFO with my page replacement solution
typedef struct Node Node;
typedef struct Node{
	int page_num;
	int frame_num;
	Node *next;
	Node *prev;
} Node;

int get_frame_num(int page_num, Node *TLB_head);

int page_fault(int page_num, FILE** backing_store);

void get_page(int page_num, int frame_num, FILE** backing_store);

void add_node(Node **head, Node **tail, int page_num, int frame_num, bool is_tlb);

void remove_node(Node **node, bool is_tlb);

signed char physical_memory[FRAME_COUNT][FRAME_SIZE];
int page_table[PAGE_TABLE_SIZE];

bool frame_taken[FRAME_COUNT];
bool valid[PAGE_TABLE_SIZE];

int page_faults = 0, tlb_hits = 0, tlb_count = 0;

FILE *out1;
FILE *out2;
FILE *out3;

int main(int argc, char *argv[])
{
	int address, page_num, offset, frame_num;

	char address_buffer[7];

	Node *TLB_head, *TLB_tail, *page_head, *page_tail, *curr, *temp;
	
	memset(&TLB_head, 0, sizeof(Node));
	memset(&TLB_tail, 0, sizeof(Node));
	memset(&page_head, 0, sizeof(Node));
	memset(&page_tail, 0, sizeof(Node));

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

	if(input_file == NULL || backing_store == NULL){
		fprintf(stderr, "Error opening input file or backing store!\n");
		return 1;
	}
			
	// Read in logical addresses until End-of-File is reached
	while(fgets(address_buffer, 7, input_file)){

		address = atoi(address_buffer);

		// Bit AND mask to get the highest 8 bits
		page_num = address & 65280;

		// Bit AND mask to get the lowest 8 bits
		offset = address & 255;

		// Shift the page num removing the lowest 8 bits which are zero after bit AND mask
		page_num = page_num >> 8;
			
		frame_num = get_frame_num(page_num, TLB_tail);

		// PAGE FAULT if no frame num was found in TLD or page table
		if(frame_num == -1){

			frame_num = page_fault(page_num, &backing_store);

			// PAGE REPLACEMENT if no free frames could be found in page fault
			if(frame_num == -1){
				printf("No available frame found!\n");

				valid[page_head->page_num] = false;

				valid[page_num] = true;

				frame_num = page_table[page_num] = page_head->frame_num;

				get_page(page_num, frame_num, &backing_store);

				remove_node(&page_head, false);
				
			}	
			
			add_node(&page_head, &page_tail, page_num, frame_num, false);
			
		}
		
		
		add_node(&TLB_head, &TLB_tail, page_num, frame_num, true);

		if(tlb_count > TLB_SIZE)
			remove_node(&TLB_head, true);
			
		// !! CHANGE THIS TO PRINT TO FILE <-----------------------!!!
		//	printf("Virtual address: %d Physical address: %d ", address, ((frame_num * 256) + offset));
		//	printf("Value: %d\n", physical_memory[frame_num][offset]);

		/*write to the files*/
                fprintf(out1, "%d\n", address);
		fprintf(out2,"%d\n", ((frame_num * 256) + offset));
                fprintf(out3, "%d\n", physical_memory[frame_num][offset]);

	}


	printf("\n(without page replacement)\n");
	printf("Page faults = %d / 1000, %.3f\n", page_faults, ((double)page_faults/1000));
	printf("TLB hits = %d / 1000, %.3f\n", tlb_hits, ((double)tlb_hits/1000));

	curr = page_head;

	while(curr != NULL){
		temp = curr;
		curr = curr->next;
		free(temp);
	}

	curr = TLB_head;

	while(curr != NULL){
		temp = curr;
		curr = curr->next;
		free(temp);
	}

	fclose(input_file);
	fclose(backing_store);
	fclose(out1);
	fclose(out2);
	fclose(out3);

	return 0;
}


int get_frame_num(int page_num, Node *TLB_tail){

	// ----CONSULT TLB------
	// Check each TLB object starting from youngest to see if the page number exists for fast access
	Node *curr = TLB_tail;

	while(curr != NULL){

		if(curr->page_num == page_num){
			tlb_hits++;
			return curr->frame_num;
		}
			

		curr = curr->prev;
	}

	// ----CONSULT PAGE TABLE------
	if(valid[page_num])
		return page_table[page_num];

	return -1;
}


int page_fault(int page_num, FILE** backing_store){

	page_faults++;

	// ----PAGE FAULT------
	// Look for an empty frame in memory to fill with a page
	for(int frame_num = 0; frame_num < FRAME_COUNT; ++frame_num){
	
		if(frame_taken[frame_num] == false){
		
			frame_taken[frame_num] = valid[page_num] = true;

			page_table[page_num] = frame_num;

			get_page(page_num, frame_num, backing_store);

			return frame_num;
		}
	}

	return -1;
}

void get_page(int page_num, int frame_num, FILE** backing_store){
	
	// The buffer will hold an entire page + 1 for null character
	char buffer[FRAME_SIZE + 1];
	
	fseek(*backing_store, page_num * 256, SEEK_SET);

	fread(buffer, FRAME_SIZE, 1, *backing_store);

	buffer[256] = '\0';

	// Copy buffer one by one since some data is 0 which will be considered null and stop strdup()
	// and other functions from copying the data
	for(int i = 0; i < FRAME_SIZE; i++){
		physical_memory[frame_num][i] = buffer[i];
	}
}

void add_node(Node **head, Node **tail, int page_num, int frame_num, bool is_tlb){

	if(*head == NULL){
		*head = (Node *)malloc(sizeof(Node));
		*tail = *head;
		(*head)->prev = NULL;
	}
	else{
		if(is_tlb){
			// Check through entire queue, if it already exists, remove and add to tail
			Node *curr = *head;
				
			while(curr != NULL){

				if(curr->page_num == page_num){

					if(*head == curr)
						remove_node(head, is_tlb);
					else if(*tail == curr)
						return;		// NOTE: if it already is tail, do nothing
					else
						remove_node(&curr, is_tlb);
					

					break;
				}

				curr = curr->next;
			}
		}

		(*tail)->next = (Node *)malloc(sizeof(Node));
		(*tail)->next->prev = *tail;
		*tail = (*tail)->next;
		
	}

	(*tail)->frame_num = frame_num;
	(*tail)->page_num = page_num;
		
	(*tail)->next = NULL;	

	if(is_tlb)
		tlb_count++;

}

void remove_node(Node **node, bool is_tlb){

	Node *deleted_node = *node;

	*node = deleted_node->next;
	
	(*node)->prev = deleted_node->prev;
	
	if(deleted_node->prev != NULL)
		deleted_node->prev->next = *node;

	free(deleted_node);
	
	if(is_tlb)
		tlb_count--;
}




