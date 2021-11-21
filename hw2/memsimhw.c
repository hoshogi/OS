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

struct pageTableEntry {
	int valid;
	int frameNum;
	int len;
	struct pageTableEntry *next;
	struct pageTableEntry *prev;
	struct pageTableEntry *secondLevelPageTable;
} fifoQueue, lruList;

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

void initProcTable(int numProcess) {
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
	}
}

void initFirstLevelPageTable(int firstLevelBits, int numProcess) {
	int i, j;

	for (i = 0; i < numProcess; i++) {
		for (j = 0; j < (1L << firstLevelBits); j++) {
			procTable[i].firstLevelPageTable[j].valid = 0;
			procTable[i].firstLevelPageTable[j].frameNum = -1;
			procTable[i].firstLevelPageTable[j].len = 0;
			procTable[i].firstLevelPageTable[j].next = NULL;
			procTable[i].firstLevelPageTable[j].prev = NULL;
			procTable[i].firstLevelPageTable[j].secondLevelPageTable = NULL;
		}
	}
}

void initSecondLevelPageTable(struct pageTableEntry* pageTable, int secondLevelBits) {
	int i;

	for (i = 0; i < (1L << secondLevelBits); i++) {
		pageTable[i].valid = 0;
		pageTable[i].frameNum = 0;
		pageTable[i].len = 0;
		pageTable[i].next = NULL;
		pageTable[i].prev = NULL;
		pageTable[i].secondLevelPageTable = NULL;
	}
}

void pushFifoQueue(int procNum, int pageNum, int frameNum) {
	procTable[procNum].firstLevelPageTable[pageNum].valid = 1;
	procTable[procNum].firstLevelPageTable[pageNum].frameNum = frameNum;

	if (fifoQueue.len == 0) {
		fifoQueue.next = fifoQueue.prev = &procTable[procNum].firstLevelPageTable[pageNum];
	}
	else {
		fifoQueue.prev->next = &procTable[procNum].firstLevelPageTable[pageNum];
		fifoQueue.prev = &procTable[procNum].firstLevelPageTable[pageNum];
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

void pushLruList(struct pageTableEntry* pageTable, int frameNum) {
	int valid;

	valid = pageTable->valid;
	pageTable->frameNum = frameNum;

	if (valid == 0) {
		if (lruList.len == 0) {
			lruList.next = lruList.prev = pageTable;
			pageTable->prev = pageTable->next = &lruList;
		}
		else {
			lruList.prev->next = pageTable;
			pageTable->prev = lruList.prev;
			lruList.prev = pageTable;
			pageTable->next = &lruList;
		}

		pageTable->valid = 1;
		lruList.len++;
	} 
	else if (valid == 1) {
		pageTable->prev->next = pageTable->next;
		pageTable->next->prev = pageTable->prev;
		lruList.prev->next = pageTable;
		pageTable->prev = lruList.prev;
		lruList.prev = pageTable;
		pageTable->next = &lruList;
	}
}

void popLruList() {
	struct pageTableEntry* iter;

	iter = lruList.next;
	lruList.next = iter->next;
	iter->next = NULL;
	lruList.len--;
}

void oneLevelVMSim(int simType, int nFrame, int numProcess) {
	unsigned int addr, pageNum, frameNum;
	char rw;
	int i;
	
	for (i = 0; i < numProcess; i++) {
		procTable[i].firstLevelPageTable = (struct pageTableEntry *)malloc(sizeof(struct pageTableEntry) * (1L << VIRTUALADDRBITS - PAGESIZEBITS));
		initFirstLevelPageTable(VIRTUALADDRBITS - PAGESIZEBITS, numProcess);
	}

	i = 0;
	while (fscanf(procTable[i].tracefp, "%x %c", &addr, &rw) != EOF) {
		pageNum = addr >> 12;
		procTable[i].ntraces++;

		if (simType == 0) { // FIFO
			if (procTable[i].firstLevelPageTable[pageNum].valid == 1) {
				procTable[i].numPageHit++;
				i = (i + 1) % numProcess;
				continue;
			}
			
			if (fifoQueue.len == nFrame) {
				frameNum = fifoQueue.next->frameNum;
				fifoQueue.next->valid = 0;

				popFifoQueue();
				pushFifoQueue(i, pageNum, frameNum);
			}
			else {
				pushFifoQueue(i, pageNum, fifoQueue.len);
			}
			procTable[i].numPageFault++;
		} 
		else if (simType == 1) { // LRU
			if (procTable[i].firstLevelPageTable[pageNum].valid == 1) {
				procTable[i].numPageHit++;
				pushLruList(&procTable[i].firstLevelPageTable[pageNum], procTable[i].firstLevelPageTable[pageNum].frameNum);
			}
			else {
				if (lruList.len == nFrame) {
					frameNum = lruList.next->frameNum;
					lruList.next->valid = 0;

					popLruList();
					pushLruList(&procTable[i].firstLevelPageTable[pageNum], frameNum);
				}
				else {	
					pushLruList(&procTable[i].firstLevelPageTable[pageNum], lruList.len);
				}
				procTable[i].numPageFault++;
			}
		}
		i = (i + 1) % numProcess;
	}

	for (i = 0; i < numProcess; i++) {
		printf("**** %s *****\n",procTable[i].traceName);
		printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
		printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
		printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
		assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
	}
}

void twoLevelVMSim(int firstLevelBits, int nFrame, int numProcess, int phyMemSizeBits) {
	unsigned int addr, pageNum, frameNum, firstPageNum, secondPageNum;
	int i, secondLevelBits;
	char rw;
	
	secondLevelBits = 20 - firstLevelBits;
	for (i = 0; i < numProcess; i++) {
		procTable[i].firstLevelPageTable = (struct pageTableEntry *)malloc(sizeof(struct pageTableEntry) * (1L << firstLevelBits));
		initFirstLevelPageTable(firstLevelBits, numProcess);
	}

	i = 0;
	while (fscanf(procTable[i].tracefp, "%x %c", &addr, &rw) != EOF) {
		pageNum = addr >> 12;
		procTable[i].ntraces++;

		firstPageNum = addr >> 32 - firstLevelBits;
		secondPageNum = addr << firstLevelBits;
		secondPageNum >>= firstLevelBits + 12;

		if (procTable[i].firstLevelPageTable[firstPageNum].valid == 0) { // firstLevelPage의 valid == 0
			procTable[i].numPageFault++;
			procTable[i].num2ndLevelPageTable++;
			procTable[i].firstLevelPageTable[firstPageNum].valid = 1;
			procTable[i].firstLevelPageTable[firstPageNum].secondLevelPageTable = (struct pageTableEntry *)malloc(sizeof(struct pageTableEntry) * (1L << secondLevelBits));
			initSecondLevelPageTable(procTable[i].firstLevelPageTable[firstPageNum].secondLevelPageTable, secondLevelBits);

			if (lruList.len == numProcess) { // lrulist가 가득 찼을때 -> replacement
				frameNum = lruList.next->frameNum;
				lruList.next->valid = 0;

				popLruList();
				pushLruList(&procTable[i].firstLevelPageTable[firstPageNum].secondLevelPageTable[secondPageNum], frameNum);
			}
			else { // lru가 가득 안찼을때 -> 제일 작은 번호대로 채운다
				pushLruList(&procTable[i].firstLevelPageTable[firstPageNum].secondLevelPageTable[secondPageNum], lruList.len);
			}
		} 
		else { // firstLevelPage의 valid == 1
			if (procTable[i].firstLevelPageTable[firstPageNum].secondLevelPageTable == NULL) { // second table이 없을때 second table 생성
				procTable[i].numPageFault++;
				procTable[i].num2ndLevelPageTable++;
				procTable[i].firstLevelPageTable[firstPageNum].secondLevelPageTable = (struct pageTableEntry *)malloc(sizeof(struct pageTableEntry) * (1L << secondLevelBits));
				initSecondLevelPageTable(procTable[i].firstLevelPageTable[firstPageNum].secondLevelPageTable, secondLevelBits);

				if (lruList.len == numProcess) { // lrulist가 가득 찼을때 -> replacement
					frameNum = lruList.next->frameNum;
					lruList.next->valid = 0;

					popLruList();
					pushLruList(&procTable[i].firstLevelPageTable[firstPageNum].secondLevelPageTable[secondPageNum], frameNum);
				}
				else { // lru가 가득 안찼을때 -> 제일 작은 번호대로 채운다
					pushLruList(&procTable[i].firstLevelPageTable[firstPageNum].secondLevelPageTable[secondPageNum], lruList.len);
				}
			}
			else { // second table이 있을때 second page 에 접근해서 valid면 hit -> push
				if (procTable[i].firstLevelPageTable[firstPageNum].secondLevelPageTable[secondPageNum].valid == 0) { // valid 가 0 인경우
					procTable[i].numPageFault++;

					if (lruList.len == numProcess) { // lrulist가 가득 찼을때 -> replacement
						frameNum = lruList.next->frameNum;
						lruList.next->valid = 0;

						popLruList();
						pushLruList(&procTable[i].firstLevelPageTable[firstPageNum].secondLevelPageTable[secondPageNum], frameNum);
					}
					else { // lru가 가득 안찼을때 -> 제일 작은 번호대로 채운다
						pushLruList(&procTable[i].firstLevelPageTable[firstPageNum].secondLevelPageTable[secondPageNum], lruList.len);
					}
				}
				else {
					procTable[i].numPageHit++;
					pushLruList(&procTable[i].firstLevelPageTable[firstPageNum].secondLevelPageTable[secondPageNum], lruList.len);
				}
			}
		}

		i = (i + 1) % numProcess;
	}



	for(i=0; i < numProcess; i++) {
		printf("**** %s *****\n",procTable[i].traceName);
		printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
		printf("Proc %d Num of second level page tables allocated %d\n",i,procTable[i].num2ndLevelPageTable);
		printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
		printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
		assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
	}
}

// void invertedPageVMSim() {

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
	int phyMemSizeBits, nFrame, firstLevelBits, numProcess;
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
		initProcTable(numProcess);

		if (procTable[i].tracefp == NULL) {
			printf("ERROR: can't open %s file; exiting...", argv[i + optind + 3]);
			exit(1);
		}
	}

	fifoQueue.len = fifoQueue.valid = fifoQueue.frameNum = 0;
	fifoQueue.prev = fifoQueue.next = fifoQueue.secondLevelPageTable = NULL;
	lruList.len = lruList.valid = lruList.frameNum = 0;
	lruList.prev = lruList.next = lruList.secondLevelPageTable = NULL;

	printf("Num of Frames %d Physical Memory Size %ld bytes\n", nFrame, (1L << phyMemSizeBits));
	
	if (simType == 0) {
		printf("=============================================================\n");
		printf("The One-Level Page Table with FIFO Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		oneLevelVMSim(simType, nFrame, numProcess);
	}
	
	if (simType == 1) {
		printf("=============================================================\n");
		printf("The One-Level Page Table with LRU Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		oneLevelVMSim(simType, nFrame, numProcess);
	}
	
	if (simType == 2) {
		printf("=============================================================\n");
		printf("The Two-Level Page Table Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		twoLevelVMSim(firstLevelBits, nFrame, numProcess, phyMemSizeBits);
	}
	
	if (simType == 3) {
		printf("=============================================================\n");
		printf("The Inverted Page Table Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		// invertedPageVMSim(...);
	}

	return(0);
}