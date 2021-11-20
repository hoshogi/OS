//
// Virual Memory Simulator Homework
// One-level page table system with FIFO and LRU
// Two-level page table system with LRU
// Inverted page table with a hashing system 
// Submission Year: 2021
// Student Name: Hoseok Lee
// Student Number: B611167
//
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define PAGESIZEBITS 12			// page size = 4Kbytes
#define VIRTUALADDRBITS 32		// virtual address space size = 4Gbytes

#define VIRTUALADDRESSNUM 1000000

int numProcess;
int nFrame;
int firstLevelBits;
int phyMemSizeBits;

struct pageTableEntry {
	int valid;
	int frameNum;
	int len;
	struct pageTableEntry *next;
	struct pageTableEntry *tail;
} fifoQueue;

struct procEntry {
	char *traceName;			// the memory trace name
	int pid;					// process (trace) id
	int ntraces;				// the number of memory traces
	int num2ndLevelPageTable;	// The 2nd level page created(allocated);
	int numIHTConflictAccess; 	// The number of Inverted Hash Table Conflict Accesses
	int numIHTNULLAccess;		// The number of Empty Inverted Hash Table Accesses
	int numIHTNonNULLAcess;		// The number of Non Empty Inverted Hash Table Accesses
	int numPageFault;			// The number of page faults
	int numPageHit;				// The number of page hits
	struct pageTableEntry *firstLevelPageTable;
	struct pageTableEntry *pageTable;
	FILE *tracefp;
} *procTable;

// struct physicalMemory {

// };

// struct frameEntry {
// 	int isMapped;
// 	int pid;
// 	int pageTableNum;
// };

// struct fifo {
// 	int len;
// 	struct pageTableEntry *head;
// 	struct pageTableEntry *tail;
// } fifoQueue;

void initProcTable() {
	int i;

	for (i = 0; i < numProcess; i++) {
		procTable[i].ntraces = 0;
		procTable[i].num2ndLevelPageTable = 0;
		procTable[i].numIHTConflictAccess = 0;
		procTable[i].numIHTNULLAccess = 0;
		procTable[i].numIHTNonNULLAcess = 0;
		procTable[i].numPageFault = 0;
		procTable[i].numPageHit = 0;			
		procTable[i].firstLevelPageTable = NULL;
		procTable[i].pageTable = NULL;
	}
}

void initPageTable() {
	int i;

	for (i = 0; i < numProcess; i++) {
		for (j = 0; j < (1L << 20); j++) {
			procTable[i].pageTable[j].valid = 0;
			procTable[i].pageTable[j].frameNum = 0;
			procTable[i].pageTable[j].len = 0;
			procTable[i].pageTable[j].next = NULL;
			procTable[i].pageTable[j].tail = NULL;
		}
	}
}

void pushFifoQueue(int procNum, int pageNum, int frameNum) {
	procTable[procNum].pageTable[pageNum].valid = 1;
	procTable[procNum].pageTable[pageNum].frameNum = frameNum;

	if (fifoQueue.len == 0) {
		fifoQueue.next = fifoQueue.tail = &procTable[procNum].pageTable[pageNum];
	}
	else {
		fifoQueue.tail->next = &procTable[procNum].pageTable[pageNum];
		fifoQueue.tail = &procTable[procNum].pageTable[pageNum];
	}
	fifoQueue.len++;
}

void popFifoQueue() {
	struct pageTableEntry* iter;

	iter = fifoQueue.next;
	fifoQueue.next = iter->next;
	iter->next = NULL;
	fifoQueue.len--;
}

void oneLevelVMSim(int simType) {
	unsigned int addr, pageNum, frameNum;
	char rw;
	int i, j;
	
	for (i = 0; i < numProcess; i++) {
		procTable[i].pageTable = (struct pageTableEntry *)malloc(sizeof(struct pageTableEntry) * (1L << 20));
		initPageTable();
	}
	

	if (simType == 0) { // FIFO

		for (i = 0; i < VIRTUALADDRESSNUM; i++) {
			for (j = 0; j < numProcess; j++) {
				fscanf(procTable[j].tracefp, "%x, %c", &addr, &rw);
				pageNum = addr >> 12;
				procTable[j].ntraces++;

				if (procTable[j].pageTable[pageNum].valid == 1) {
					procTable[j].numPageHit++;
					continue;
				}
				
				if (fifoQueue.len == nFrame) {
					frameNum = fifoQueue.next->frameNum;
					fifoQueue.next->valid = 0;

					popFifoQueue();
					pushFifoQueue(j, pageNum, frameNum);
				}
				else {
					pushFifoQueue(j, pageNum, fifoQueue.len);
				}
				procTable[j].numPageFault++;
			}
		}
	}
	else if (simType == 1) { // LRU

	}

	for(i = 0; i < numProcess; i++) {
		printf("**** %s *****\n",procTable[i].traceName);
		printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
		printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
		printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
		assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
	}
}

// void twoLevelVMSim(...) {
	
// 	for(i=0; i < numProcess; i++) {
// 		printf("**** %s *****\n",procTable[i].traceName);
// 		printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
// 		printf("Proc %d Num of second level page tables allocated %d\n",i,procTable[i].num2ndLevelPageTable);
// 		printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
// 		printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
// 		assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
// 	}
// }

// void invertedPageVMSim(...) {

// 	for(i=0; i < numProcess; i++) {
// 		printf("**** %s *****\n",procTable[i].traceName);
// 		printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
// 		printf("Proc %d Num of Inverted Hash Table Access Conflicts %d\n",i,procTable[i].numIHTConflictAccess);
// 		printf("Proc %d Num of Empty Inverted Hash Table Access %d\n",i,procTable[i].numIHTNULLAccess);
// 		printf("Proc %d Num of Non-Empty Inverted Hash Table Access %d\n",i,procTable[i].numIHTNonNULLAcess);
// 		printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
// 		printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
// 		assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
// 		assert(procTable[i].numIHTNULLAccess + procTable[i].numIHTNonNULLAcess == procTable[i].ntraces);
// 	}
// }

int main(int argc, char *argv[]) {
	int i, c, simType;
	int optind;

	simType = atoi(argv[1]);
	firstLevelBits = atoi(argv[2]);
	phyMemSizeBits = atoi(argv[3]);
	optind = 1;
	numProcess = argc - 4;
	nFrame = 1 << (phyMemSizeBits - 12);
	
	procTable = (struct procEntry *)malloc(sizeof(struct procEntry) * numProcess);

	// initialize procTable for Memory Simulations
	for(i = 0; i < numProcess; i++) {
		// opening a tracefile for the process
		printf("process %d opening %s\n", i, argv[i + optind + 3]);

		procTable[i].traceName = argv[i + optind + 3];
		procTable[i].pid = i;
		procTable[i].tracefp = fopen(argv[i + optind + 3], "r");
		
		initProcTable();

		if (procTable[i].tracefp == NULL) {
			printf("ERROR: can't open %s file; exiting...", argv[i + optind + 3]);
			exit(1);
		}
	}

	fifoQueue.len = fifoQueue.valid = fifoQueue.frameNum = 0;
	fifoQueue.tail = fifoQueue.next = NULL;

	printf("Num of Frames %d Physical Memory Size %ld bytes\n", nFrame, (1L << phyMemSizeBits));
	
	if (simType == 0) {
		printf("=============================================================\n");
		printf("The One-Level Page Table with FIFO Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		oneLevelVMSim(simType);
	}
	
	if (simType == 1) {
		printf("=============================================================\n");
		printf("The One-Level Page Table with LRU Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		// oneLevelVMSim(simType);
	}
	
	if (simType == 2) {
		printf("=============================================================\n");
		printf("The Two-Level Page Table Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		// twoLevelVMSim(...);
	}
	
	if (simType == 3) {
		printf("=============================================================\n");
		printf("The Inverted Page Table Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		// invertedPageVMSim(...);
	}

	return(0);
}