/*
Nikola Kilibarda and Matthew Mabrey
CSC-345-01 Operating Systems
Dr. Yoon
Project 3
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#define PAGE_SIZE 256
#define BUFFER_SIZE 10
#define NUMBER_OF_FRAMES 256
#define PAGE_TABLE_SIZE 256
#define BLOCK 256
#define TLB_SIZE 16
#define FRAME_SIZE 128

struct TlbEntry
{
	int frame_num;
	int pg_num;
        int isValid;
};
	
/*file declaration*/
FILE *backing_store_bin;
FILE *address_file;
FILE *out1;
FILE *out2;
FILE *out3;
	
typedef struct TlbEntry TlbEntry_t;

signed char physical_memory[FRAME_SIZE][PAGE_SIZE];
/* a char array  read into  logical addresses from the file*/
char input_address[BUFFER_SIZE];

int phys_addr;
int logc_addr;
int frame_num;
int pg_num;
int offset;
int val = 0;
int TlbHits = 0;	
int pg_faults = 0;
int nxt_open_tlb_indx = 1;
int nxt_open_frame = 0;
int frame_cnt = 0;
int nxt_frame = 0;
bool foundFlg = false;
int pg_no_indx = 0;

int page_table[PAGE_TABLE_SIZE];
int frame_table[128];

TlbEntry_t table[TLB_SIZE];

/* an array to read the frame from the backing store */
signed char frame_tmp[PAGE_SIZE];
	
int get_page_number(int logc_addr);
int get_offset(int logc_addr); 
void initialize_page_table(); 
void initTlb(); 
int checkTlb(int pg_num); 
int check_page_table(int pg_num); 
	
int main(int argc, char *argv[])
{
	/* Check for user input */
        if(argc < 2){
                printf("You must enter the name of an input file!\n");
                return 1;
        }

        /*open the address file for reading*/	
	address_file = fopen(argv[1], "rt"); 
	/*open the backing store  binary file*/
	backing_store_bin = fopen("BACKING_STORE.bin", "rb"); 
	/*open the file write the logical addressess*/
	out1 = fopen("out1.txt", "wt"); 
	/*open the file to write the physical addressess*/
	out2 = fopen("out2.txt", "wt"); 
	/*open the file to write the vals*/
	out3 = fopen("out3.txt", "wt"); 

       if (address_file == NULL)
    {
        printf("Cannot open file addresses.txt");
        return -1;
    }


	if (backing_store_bin == NULL) 
    {
        printf("Cannot open file BACKING_STORE.bin");
        return -1;
    }


	/*reset TLB*/
        initTlb();

	/*reset/init  the page table*/
	initialize_page_table();
	
	
	while (fgets(input_address, BUFFER_SIZE, address_file) != NULL) 
    {
	    logc_addr = atoi(input_address);
	
	    pg_num = get_page_number(logc_addr); 
	    /*First check TLB*/
	    frame_num = checkTlb(pg_num);
	    offset = get_offset(logc_addr);
       /*then check the page table*/	
	    if (frame_num == -1) 
        { 
	        frame_num = check_page_table(pg_num);
	    }
              /*construct the physical address for the frame number and offset*/
		phys_addr = (frame_num << 8) | offset;
	        val = physical_memory[frame_num][offset];
		/*write to the files*/
                fprintf(out1, "%d\n", logc_addr);
		fprintf(out2,"%d\n", phys_addr);
                fprintf(out3, "%d\n", val);
	}
	
	printf("Page faults = %d / 1000, %.3f\n", pg_faults, ((double)pg_faults/1000));
        printf("TLB hits = %d / 1000, %.3f\n", TlbHits, ((double)TlbHits/1000));

	
	/*closing the files after finished writting*/
        fclose(out1);
        fclose(out2);
        fclose(out3);
	fclose(address_file);
	fclose(backing_store_bin);
	
	return 0;
}



void initialize_page_table()
{
    int k=0;
    while ( k < PAGE_TABLE_SIZE)
    {
            page_table[k] = -1;
	    k++;
    }
}

int get_offset(int logc_addr)
{
        return (logc_addr & 255);
}


int get_page_number(int logc_addr)
{
        return (logc_addr & 65280) >> 8;
}



int checkTlb(int pg_num)
{
	int frame_num = -1;
	int k = 0;
	while ( k < TLB_SIZE) {
	if ((table[k].pg_num == pg_num) && (table[k].isValid == 1)){
	frame_num = table[k].frame_num;
	TlbHits++;
                break;
		}
	k++;
	}
	return frame_num;
}


void initTlb()
{
       int k=0;
       while (k < TLB_SIZE)
    {
            table[k].frame_num = -1;
            table[k].pg_num = -1;
            table[k].isValid = -1;
	    k++;
        }
}

int check_page_table(int pg_num)
{
    foundFlg=false;
    frame_num = page_table[pg_num];
    int k = 0;

    // if frame  not in  page table get from the  backing store 
    if (frame_num == -1)
    {
	    //count page faults
            ++pg_faults;
	    //get the info from the backing store
        fseek(backing_store_bin, pg_num * PAGE_SIZE, SEEK_SET);
        fread(frame_tmp, sizeof(signed char), PAGE_SIZE, backing_store_bin);

       while (k < PAGE_SIZE)
        {
            physical_memory[nxt_frame][k] = frame_tmp[k];
	    k++;
        }

        //  page replacement due to  max_frame = 128
        if (!(FRAME_SIZE > frame_cnt))
        {
	    //since the frame will be overwritten, search for page in page table and set to -1
            for (k = 0; k < PAGE_TABLE_SIZE; k++)
            {
                if (page_table[k] == nxt_frame)
                {
                    page_table[k] = -1;
                    pg_no_indx = k;
                    break;
                }
            }
        }

            //get frame from the page_table[page_num] FIFO
        page_table[pg_num] = nxt_frame;
        frame_num = page_table[pg_num];
        nxt_frame = (nxt_frame + 1) % FRAME_SIZE;
        frame_cnt++;
        }

    // in case that overwritten page exist in the tlb, find the frame in tlb and update the page. 
    for (k = 0; k < TLB_SIZE; k++)
    {
        if (frame_num == table[k].frame_num)
        {
            table[k].pg_num = pg_num;
	    foundFlg = true;
        }
    }

    //if frame not in tlb in the TLB update it.
    if (!foundFlg)
    {
        table[nxt_open_tlb_indx].frame_num = frame_num;
        table[nxt_open_tlb_indx].pg_num = pg_num;
        table[nxt_open_tlb_indx].isValid = true;
        /*FIFO*/
        nxt_open_tlb_indx = (nxt_open_tlb_indx + 1) % TLB_SIZE;

    }

        return frame_num;
}
