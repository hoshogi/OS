//
// 2021
// Operating Systems
// CPU Schedule Simulator Homework
// Student Number : B611167
// Name : Hoseok Lee
//
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <limits.h>

#define SEED 10

// process states
#define S_IDLE 0			
#define S_READY 1
#define S_BLOCKED 2
#define S_RUNNING 3
#define S_TERMINATE 4

int NPROC, NIOREQ, QUANTUM;

struct ioDoneEvent {
	int procid;
	int doneTime;
	int len;
	struct ioDoneEvent *prev;
	struct ioDoneEvent *next;
} ioDoneEventQueue, *ioDoneEvent;

struct process {
	int id;
	int len;		// for queue
	int targetServiceTime;
	int serviceTime;
	int startTime;
	int endTime;
	char state;
	int priority;
	int saveReg0, saveReg1;
	struct process *prev;
	struct process *next;
} *procTable;

struct process idleProc;
struct process readyQueue;
struct process blockedQueue;
struct process *runningProc;

int cpuReg0, cpuReg1;
int currentTime = 0;
int *procIntArrTime, *procServTime, *ioReqIntArrTime, *ioServTime;

void compute() {
	// DO NOT CHANGE THIS FUNCTION
	cpuReg0 = cpuReg0 + runningProc->id;
	cpuReg1 = cpuReg1 + runningProc->id;
	// printf("In computer proc %d cpuReg0 %d\n",runningProc->id,cpuReg0);
}

void initProcTable() {
	int i;
	
	for(i=0; i < NPROC; i++) {
		procTable[i].id = i;
		procTable[i].len = 0;
		procTable[i].targetServiceTime = procServTime[i];
		procTable[i].serviceTime = 0;
		procTable[i].startTime = 0;
		procTable[i].endTime = 0;
		procTable[i].state = S_IDLE;
		procTable[i].priority = 0;
		procTable[i].saveReg0 = 0;
		procTable[i].saveReg1 = 0;
		procTable[i].prev = NULL;
		procTable[i].next = NULL;
	}
}

void initIdleProc() {
	idleProc.id = -1;
	idleProc.len = 0;
	idleProc.targetServiceTime = -1;
	idleProc.serviceTime = 0;
	idleProc.startTime = 0;
	idleProc.endTime = 0;
	idleProc.state = S_IDLE;
	idleProc.priority = 0;
	idleProc.saveReg0 = 0;
	idleProc.saveReg1 = 0;
	idleProc.prev = NULL;
	idleProc.next = NULL;
} 

void initIoDoneEvent() {
	int i;
	
	for (i = 0; i < NIOREQ; i++) {
		ioDoneEvent[i].procid = -1;
		ioDoneEvent[i].doneTime = 0;
		ioDoneEvent[i].len = 0;
		ioDoneEvent[i].prev = NULL;
		ioDoneEvent[i].next = NULL;
	}
}

void pushReadyQueue(int pid) {
	procTable[pid].state = S_READY;

	readyQueue.prev->next = &procTable[pid];
	procTable[pid].prev = readyQueue.prev;
	procTable[pid].next = &readyQueue;
	readyQueue.prev = &procTable[pid];
	readyQueue.len++;
 }

struct process* popReadyQueue(int pid) {
	struct process* iter = readyQueue.next;

	while (iter != &readyQueue) {
		if (iter->id == pid) {
			iter->prev->next = iter->next;
			iter->next->prev = iter->prev;
			iter->prev = NULL;
			iter->next = NULL;
			readyQueue.len--;
			return iter;
		}
		iter = iter->next;
	}
}

void pushBlockedQueue(int pid) {
	procTable[pid].state = S_BLOCKED;

	procTable[pid].prev = blockedQueue.prev;
	blockedQueue.prev->next = &procTable[pid];
	blockedQueue.prev = &procTable[pid];
	procTable[pid].next = &blockedQueue;
	blockedQueue.len++;
}

struct process* popBlockedQueue(int pid) {
	struct process* iter = blockedQueue.next;

	while (iter != &blockedQueue) {
		if (iter->id == pid) {
			iter->prev->next = iter->next;
			iter->next->prev = iter->prev;
			iter->prev = NULL;
			iter->next = NULL;
			blockedQueue.len--;
			return iter;
		}  
		iter = iter->next;
	}
}

void pushIoDoneEventQueue(int nioreq) {
	struct ioDoneEvent* iter = ioDoneEventQueue.next;

	if (ioDoneEventQueue.len == 0) {
		ioDoneEventQueue.prev = &ioDoneEvent[nioreq];
		ioDoneEventQueue.next = &ioDoneEvent[nioreq];
		ioDoneEvent[nioreq].prev = &ioDoneEventQueue;
		ioDoneEvent[nioreq].next = &ioDoneEventQueue;
	}
	else {
		if (ioDoneEvent[nioreq].doneTime >= ioDoneEventQueue.prev->doneTime) {
			ioDoneEventQueue.prev->next = &ioDoneEvent[nioreq];
			ioDoneEvent[nioreq].prev = ioDoneEventQueue.prev;
			ioDoneEventQueue.prev = &ioDoneEvent[nioreq];
			ioDoneEvent[nioreq].next = &ioDoneEventQueue;
		}
		else {
			while (iter != &ioDoneEventQueue) {
				if (ioDoneEvent[nioreq].doneTime < iter->doneTime) {
					iter->prev->next = &ioDoneEvent[nioreq];
					ioDoneEvent[nioreq].prev = iter->prev;
					ioDoneEvent[nioreq].next = iter;
					iter->prev = &ioDoneEvent[nioreq];				
					break;
				}
				iter = iter->next;
			}
		}
	}
	ioDoneEventQueue.len++;
}

void popIoDoneEventQueue() {
	struct ioDoneEvent* ioDoneEventIter = ioDoneEventQueue.next;

	ioDoneEventQueue.next = ioDoneEventIter->next;
	ioDoneEventIter->next->prev = &ioDoneEventQueue;
	ioDoneEventIter->prev = NULL;
	ioDoneEventIter->next = NULL;
	ioDoneEventQueue.len--;
}

void procExecSim(struct process *(*scheduler)()) {
	int pid, qTime=0, cpuUseTime = 0, nproc=0, termProc = 0, nioreq=0;
	char schedule = 0, nextState = S_IDLE;
	int nextForkTime, nextIOReqTime;
	
	nextForkTime = procIntArrTime[nproc];
	nextIOReqTime = ioReqIntArrTime[nioreq];

	runningProc = &idleProc;

	while(1) {
		currentTime++;
		qTime++;

		runningProc->serviceTime++;
		if (runningProc != &idleProc) cpuUseTime++;
		
		// MUST CALL compute() Inside While loop
		compute(); 
	
		if (currentTime == nextForkTime) { /* CASE 2 : a new process created */
			nextState = S_READY;	
			procTable[nproc].startTime = currentTime;

			pushReadyQueue(nproc);
			
			nproc++;
			nextForkTime += procIntArrTime[nproc];
			schedule = 1;
		}
		if (qTime == QUANTUM ) { /* CASE 1 : The quantum expires */
			nextState = S_READY;

			if (runningProc != &idleProc)
				runningProc->priority--;
			schedule = 1;
		}
		while (ioDoneEventQueue.next->doneTime == currentTime) { /* CASE 3 : IO Done Event */
			nextState = S_READY;
			pid = ioDoneEventQueue.next->procid;

			popIoDoneEventQueue();
			popBlockedQueue(pid);

			if (procTable[pid].state != S_TERMINATE) 
				pushReadyQueue(pid);
			schedule = 1;
		}
		if (cpuUseTime == nextIOReqTime) { /* CASE 5: reqest IO operations (only when the process does not terminate) */
			if (runningProc != &idleProc) {
				nextState = S_BLOCKED;
				ioDoneEvent[nioreq].procid = runningProc->id;
				ioDoneEvent[nioreq].doneTime = currentTime + ioServTime[nioreq];
				
				pushIoDoneEventQueue(nioreq);

				nioreq++;
				nextIOReqTime += ioReqIntArrTime[nioreq];
				if (qTime != QUANTUM )
					runningProc->priority++;
				schedule = 1;
			}
		}
		if (runningProc->serviceTime == runningProc->targetServiceTime) { /* CASE 4 : the process job done and terminates */
			nextState = S_TERMINATE;
			schedule = 1;
		}

		if (runningProc != &idleProc) {
			if (nextState == S_READY) {
				pushReadyQueue(runningProc->id);
			}
			else if (nextState == S_BLOCKED) {
				pushBlockedQueue(runningProc->id);
			}
			else if (nextState == S_TERMINATE) {
				runningProc->state = S_TERMINATE;
				runningProc->endTime = currentTime;
				termProc++;
			}
		}
		
		// call scheduler() if needed
		if(schedule) {
			runningProc->saveReg0 = cpuReg0;
			runningProc->saveReg1 = cpuReg1;
			
			runningProc = scheduler();

			if (runningProc == &idleProc) {
				nextState = S_IDLE;
			}
			else {
				runningProc->state = S_RUNNING;
				nextState = S_RUNNING;
				cpuReg0 = runningProc->saveReg0;
				cpuReg1 = runningProc->saveReg1;
			}
			schedule = 0;
			qTime = 0;
		}	

		if (termProc == NPROC) 
			break;
	} // while loop
}

//RR,SJF(Modified),SRTN,Guaranteed Scheduling(modified),Simple Feed Back Scheduling
struct process* RRschedule() {
	if (readyQueue.len == 0) 
		return &idleProc;
	else 
		return popReadyQueue(readyQueue.next->id);
}

struct process* SJFschedule() {
	int minTargetServiceTime = INT_MAX;
	int pid = -1;
	struct process* iter = readyQueue.next;

	if (readyQueue.len == 0) {
		return &idleProc;
	}
	else {
		while(iter != &readyQueue) {
			if (iter->targetServiceTime < minTargetServiceTime) {
				minTargetServiceTime = iter->targetServiceTime;
				pid = iter->id;
			}
			iter = iter->next;
		}
		return popReadyQueue(pid);
	}
}

struct process* SRTNschedule() {
	int minRemainingTime = INT_MAX;
	int pid = -1;
	struct process* iter = readyQueue.next;

	if (readyQueue.len == 0) {
		return &idleProc;
	}
	else {
		while(iter != &readyQueue) {
			if ((iter->targetServiceTime - iter->serviceTime) < minRemainingTime) {
				minRemainingTime = iter->targetServiceTime - iter->serviceTime;
				pid = iter->id;
			}
			iter = iter->next;
		}
		return popReadyQueue(pid);
	}
}

struct process* GSschedule() {
	double minRatio = 2.0;
	int pid = -1;
	struct process* iter = readyQueue.next;

	if (readyQueue.len == 0) {
		return &idleProc;
	}
	else {
		while(iter != &readyQueue) {
			if (((double)iter->serviceTime / iter->targetServiceTime) < minRatio) {
				minRatio = (double)iter->serviceTime / iter->targetServiceTime;
				pid = iter->id;
			}
			iter = iter->next;
		}
		return popReadyQueue(pid);
	}
}

struct process* SFSschedule() {
	int maxPriority = INT_MIN;
	int pid = -1;
	struct process* iter = readyQueue.next;

	if (readyQueue.len == 0) {
		return &idleProc;
	}
	else {
		while(iter != &readyQueue) {
			if (iter->priority > maxPriority) {
				maxPriority = iter->priority;
				pid = iter->id;
			}
			iter = iter->next;
		}
		return popReadyQueue(pid);
	}
}

void printResult() {
	// DO NOT CHANGE THIS FUNCTION
	int i;
	long totalProcIntArrTime=0,totalProcServTime=0,totalIOReqIntArrTime=0,totalIOServTime=0;
	long totalWallTime=0, totalRegValue=0;
	for(i=0; i < NPROC; i++) {
		totalWallTime += procTable[i].endTime - procTable[i].startTime;
		/*
		printf("proc %d serviceTime %d targetServiceTime %d saveReg0 %d\n",
			i,procTable[i].serviceTime,procTable[i].targetServiceTime, procTable[i].saveReg0);
		*/
		totalRegValue += procTable[i].saveReg0+procTable[i].saveReg1;
		/* printf("reg0 %d reg1 %d totalRegValue %d\n",procTable[i].saveReg0,procTable[i].saveReg1,totalRegValue);*/
	}
	for(i = 0; i < NPROC; i++ ) { 
		totalProcIntArrTime += procIntArrTime[i];
		totalProcServTime += procServTime[i];
	}
	for(i = 0; i < NIOREQ; i++ ) { 
		totalIOReqIntArrTime += ioReqIntArrTime[i];
		totalIOServTime += ioServTime[i];
	}
	
	printf("Avg Proc Inter Arrival Time : %g \tAverage Proc Service Time : %g\n", (float) totalProcIntArrTime/NPROC, (float) totalProcServTime/NPROC);
	printf("Avg IOReq Inter Arrival Time : %g \tAverage IOReq Service Time : %g\n", (float) totalIOReqIntArrTime/NIOREQ, (float) totalIOServTime/NIOREQ);
	printf("%d Process processed with %d IO requests\n", NPROC,NIOREQ);
	printf("Average Wall Clock Service Time : %g \tAverage Two Register Sum Value %g\n", (float) totalWallTime/NPROC, (float) totalRegValue/NPROC);
}

int main(int argc, char *argv[]) {
	// DO NOT CHANGE THIS FUNCTION
	int i;
	int totalProcServTime = 0, ioReqAvgIntArrTime;
	int SCHEDULING_METHOD, MIN_INT_ARRTIME, MAX_INT_ARRTIME, MIN_SERVTIME, MAX_SERVTIME, MIN_IO_SERVTIME, MAX_IO_SERVTIME, MIN_IOREQ_INT_ARRTIME;
	
	if (argc < 12) {
		printf("%s: SCHEDULING_METHOD NPROC NIOREQ QUANTUM MIN_INT_ARRTIME MAX_INT_ARRTIME MIN_SERVTIME MAX_SERVTIME MIN_IO_SERVTIME MAX_IO_SERVTIME MIN_IOREQ_INT_ARRTIME\n",argv[0]);
		exit(1);
	}
	
	SCHEDULING_METHOD = atoi(argv[1]);
	NPROC = atoi(argv[2]);
	NIOREQ = atoi(argv[3]);
	QUANTUM = atoi(argv[4]);
	MIN_INT_ARRTIME = atoi(argv[5]);
	MAX_INT_ARRTIME = atoi(argv[6]);
	MIN_SERVTIME = atoi(argv[7]);
	MAX_SERVTIME = atoi(argv[8]);
	MIN_IO_SERVTIME = atoi(argv[9]);
	MAX_IO_SERVTIME = atoi(argv[10]);
	MIN_IOREQ_INT_ARRTIME = atoi(argv[11]);
	
	printf("SIMULATION PARAMETERS : SCHEDULING_METHOD %d NPROC %d NIOREQ %d QUANTUM %d \n", SCHEDULING_METHOD, NPROC, NIOREQ, QUANTUM);
	printf("MIN_INT_ARRTIME %d MAX_INT_ARRTIME %d MIN_SERVTIME %d MAX_SERVTIME %d\n", MIN_INT_ARRTIME, MAX_INT_ARRTIME, MIN_SERVTIME, MAX_SERVTIME);
	printf("MIN_IO_SERVTIME %d MAX_IO_SERVTIME %d MIN_IOREQ_INT_ARRTIME %d\n", MIN_IO_SERVTIME, MAX_IO_SERVTIME, MIN_IOREQ_INT_ARRTIME);
	
	srandom(SEED);
	
	// allocate array structures
	procTable = (struct process *) malloc(sizeof(struct process) * NPROC);
	ioDoneEvent = (struct ioDoneEvent *) malloc(sizeof(struct ioDoneEvent) * NIOREQ);
	procIntArrTime = (int *) malloc(sizeof(int) * NPROC);
	procServTime = (int *) malloc(sizeof(int) * NPROC);
	ioReqIntArrTime = (int *) malloc(sizeof(int) * NIOREQ);
	ioServTime = (int *) malloc(sizeof(int) * NIOREQ);

	// initialize queues
	readyQueue.next = readyQueue.prev = &readyQueue;
	
	blockedQueue.next = blockedQueue.prev = &blockedQueue;
	ioDoneEventQueue.next = ioDoneEventQueue.prev = &ioDoneEventQueue;
	ioDoneEventQueue.doneTime = INT_MAX;
	ioDoneEventQueue.procid = -1;
	ioDoneEventQueue.len  = readyQueue.len = blockedQueue.len = 0;
	
	// generate process interarrival times
	for(i = 0; i < NPROC; i++ ) { 
		procIntArrTime[i] = random()%(MAX_INT_ARRTIME - MIN_INT_ARRTIME+1) + MIN_INT_ARRTIME;
	}
	
	// assign service time for each process
	for(i=0; i < NPROC; i++) {
		procServTime[i] = random()% (MAX_SERVTIME - MIN_SERVTIME + 1) + MIN_SERVTIME;
		totalProcServTime += procServTime[i];	
	}
	
	ioReqAvgIntArrTime = totalProcServTime/(NIOREQ+1);
	assert(ioReqAvgIntArrTime > MIN_IOREQ_INT_ARRTIME);
	
	// generate io request interarrival time
	for(i = 0; i < NIOREQ; i++ ) { 
		ioReqIntArrTime[i] = random()%((ioReqAvgIntArrTime - MIN_IOREQ_INT_ARRTIME)*2+1) + MIN_IOREQ_INT_ARRTIME;
	}
	
	// generate io request service time
	for(i = 0; i < NIOREQ; i++ ) { 
		ioServTime[i] = random()%(MAX_IO_SERVTIME - MIN_IO_SERVTIME+1) + MIN_IO_SERVTIME;
	}
	
#ifdef DEBUG
	// printing process interarrival time and service time
	printf("Process Interarrival Time :\n");
	for(i = 0; i < NPROC; i++ ) { 
		printf("%d ",procIntArrTime[i]);
	}
	printf("\n");
	printf("Process Target Service Time :\n");
	for(i = 0; i < NPROC; i++ ) { 
		printf("%d ",procTable[i].targetServiceTime);
	}
	printf("\n");
#endif
	
	// printing io request interarrival time and io request service time
	printf("IO Req Average InterArrival Time %d\n", ioReqAvgIntArrTime);
	printf("IO Req InterArrival Time range : %d ~ %d\n",MIN_IOREQ_INT_ARRTIME,
			(ioReqAvgIntArrTime - MIN_IOREQ_INT_ARRTIME)*2+ MIN_IOREQ_INT_ARRTIME);
			
#ifdef DEBUG		
	printf("IO Req Interarrival Time :\n");
	for(i = 0; i < NIOREQ; i++ ) { 
		printf("%d ",ioReqIntArrTime[i]);
	}
	printf("\n");
	printf("IO Req Service Time :\n");
	for(i = 0; i < NIOREQ; i++ ) { 
		printf("%d ",ioServTime[i]);
	}
	printf("\n");
#endif

	struct process* (*schFunc)();
	switch(SCHEDULING_METHOD) {
		case 1 : schFunc = RRschedule; break;
		case 2 : schFunc = SJFschedule; break;
		case 3 : schFunc = SRTNschedule; break;
		case 4 : schFunc = GSschedule; break;
		case 5 : schFunc = SFSschedule; break;
		default : printf("ERROR : Unknown Scheduling Method\n"); exit(1);
	}

	initProcTable();
	initIdleProc();
	initIoDoneEvent();
	procExecSim(schFunc);
	printResult();
}