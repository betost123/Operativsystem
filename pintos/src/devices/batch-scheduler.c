/* Tests cetegorical mutual exclusion with different numbers of threads.
 * Automatic checks only catch severe problems like crashes.
 */
#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "lib/random.h" //generate random numbers
//#include "devices/timer.h"

#define BUS_CAPACITY 3
#define SENDER 0
#define RECEIVER 1
#define NORMAL 0
#define HIGH 1

//#define OPEN 2

//define OPEN 2 or define TASKDIR task.direction, TASKPRIO task.priority

/*
 *	initialize task with direction and priority
 *	call o
 * */
typedef struct {
	int direction;
	int priority;
} task_t;


void batchScheduler(unsigned int num_tasks_send, unsigned int num_task_receive,
        unsigned int num_priority_send, unsigned int num_priority_receive);

void senderTask(void *);
void receiverTask(void *);
void senderPriorityTask(void *);
void receiverPriorityTask(void *);


void oneTask(task_t task);/*Task requires to use the bus and executes methods below*/
	void getSlot(task_t task); /* task tries to use slot on the bus */
	void transferData(task_t task); /* task processes data on the bus either sending or receiving based on the direction*/
	void leaveSlot(task_t task); /* task release the slot */

struct semaphore mutex, sender, prioritySender, reciever, priorityReciever;
//struct lock mutex;
int direction = -1;
int waitHighSender = 0;
int waitHighReciever = 0;
int freeSlots = BUS_CAPACITY;

/* initializes semaphores */
void init_bus(void){

    random_init((unsigned int)123456789);

    msg("NOT IMPLEMENTED");
    /* FIXME implement */

		sema_init(&mutex, 1);
		sema_init(&prioritySender, 0);
		sema_init(&sender, 0);
		sema_init(&priorityReciever, 0);
		sema_init(&reciever, 0);

}

/*
 *  Creates a memory bus sub-system  with num_tasks_send + num_priority_send
 *  sending data to the accelerator and num_task_receive + num_priority_receive tasks
 *  reading data/results from the accelerator.
 *
 *  Every task is represented by its own thread.
 *  Task requires and gets slot on bus system (1)
 *  process data and the bus (2)
 *  Leave the bus (3).
 */

void batchScheduler(unsigned int num_tasks_send, unsigned int num_task_receive,
        unsigned int num_priority_send, unsigned int num_priority_receive)
{
    msg("NOT IMPLEMENTED");
    /* FIXME implement */

		unsigned int i;
		for(i = 0; i < num_tasks_send; i++) {
			thread_create("senderTask", 0, senderTask, 0);
		}
		for(i = 0; i < num_task_receive; i++) {
			thread_create("recieverTask", 0, receiverTask, 0);
		}
		for(i = 0; i < num_priority_send; i++) {
			thread_create("senderPriorityTask", 0, senderPriorityTask, 0);
		}
		for(i = 0; i < num_priority_receive; i++) {
			thread_create("recieverPriorityTask", 0, receiverPriorityTask, 0);
		}

}

/* Normal task,  sending data to the accelerator */
void senderTask(void *aux UNUSED){
        task_t task = {SENDER, NORMAL};
        oneTask(task);
}

/* High priority task, sending data to the accelerator */
void senderPriorityTask(void *aux UNUSED){
        task_t task = {SENDER, HIGH};
        oneTask(task);
}

/* Normal task, reading data from the accelerator */
void receiverTask(void *aux UNUSED){
        task_t task = {RECEIVER, NORMAL};
        oneTask(task);
}

/* High priority task, reading data from the accelerator */
void receiverPriorityTask(void *aux UNUSED){
        task_t task = {RECEIVER, HIGH};
        oneTask(task);
}

/* abstract task execution*/
void oneTask(task_t task) {
  getSlot(task);
  transferData(task);
  leaveSlot(task);
}


/* task tries to get slot on the bus subsystem */
void getSlot(task_t task)
{
    msg("NOT IMPLEMENTED");
    /* FIXME implement */

		sema_down(&mutex);
		if(task.direction == SENDER) {
			if(task.priority == HIGH) {
				waitHighSender++;
				while(!freeSlots || direction == RECEIVER) {
					//sema_wait() ??
					sema_up(&mutex);
					sema_down(&prioritySender);
					sema_down(&mutex);
				}
				waitHighSender--;
				direction = task.direction;
				freeSlots--;
			} else {
				while(!freeSlots || direction == RECEIVER || waitHighSender > 0 || waitHighReciever > 0) {
					sema_up(&mutex);
					sema_down(&sender);
					sema_down(&mutex);
				}
				direction = task.direction;
				freeSlots--;
			}
		} else {
			if(task.priority == HIGH) {
				waitHighReciever++;
				while(!freeSlots || direction == SENDER) {
					sema_up(&mutex);
					sema_down(&priorityReciever);
					sema_down(&mutex);
				}
				waitHighReciever--;
				direction = task.direction;
				freeSlots--;
			} else {
				while(!freeSlots || direction == SENDER || waitHighReciever > 0 || waitHighSender > 0) {
					sema_up(&mutex);
					sema_down(&reciever);
					sema_down(&mutex);
				}
				direction = task.direction;
				freeSlots--;
			}
		}
		ASSERT(freeSlots >= 0);
		if (task.priority == NORMAL) {
			ASSERT(waitHighReciever == 0 && waitHighSender == 0);
		}
		//lock_release(&mutex)
		//lock_release(&mutex);
		sema_up(&mutex);

}

/* task processes data on the bus send/receive */
void transferData(task_t task)
{
    msg("NOT IMPLEMENTED");
    /* FIXME implement */

    //timer_msleep(100 + random_ulong() % 100);
    timer_msleep(random_ulong() % 2000);
}

/* task releases the slot */
void leaveSlot(task_t task)
{
    msg("NOT IMPLEMENTED");
    /* FIXME implement */

    sema_down(&mutex);
    freeSlots++;
    //other one
    ASSERT(freeSlots <= BUS_CAPACITY); //check number of slots on bus correct
    ASSERT(direction == task.direction); //check direction has not changed since task entered
    if (freeSlots >= BUS_CAPACITY) { direction = -1; }

    //other one
    if (direction == SENDER) {
        if (waitHighSender > 0) {
            sema_up(&prioritySender;
        } else {
            sema_up(&sender);
        }
    } else if (direction == RECEIVER) {
        if(waitHighReceiver > 0) {
            sema_up(&priorityReciever);
        } else {
            sema_up(&reciever);
        }
    } else {
        int i;

        for (i = 0; i<BUS_CAPACITY; i++) {
						sema_up(&priorityReciever);
						sema_up(&prioritySender);
						sema_up(&reciever);
						sema_up(&sender);
        }
    }
    sema_up(&mutex);
}
