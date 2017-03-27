#include <linux/module.h>  
#include <linux/kernel.h>    
#include <linux/init.h>
#include <linux/sched.h> 
#include <linux/tty.h> 
#include <linux/slab.h> 
#include <linux/string.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>      
#include <linux/moduleparam.h>
#include <linux/semaphore.h>

//#define THRESHOLD 10

extern struct semaphore gatekeeper;

struct task_struct *kernelThread = NULL;

struct IDlist{
	pid_t ID;
	struct IDlist* next;
};
typedef struct IDlist list_ID;

struct node{
	struct task_struct* task;
	struct node* next;
};
typedef struct node list_node;

static int THRESHOLD = 10;
module_param(THRESHOLD,int,0000);

/**************	LINKED LIST FUNCTIONS FOR PGID list_ID *********************/

/*	Adding a node to the beginning of the list	*/
void pushToListID(list_ID** head ,pid_t toAdd ){
	list_ID* new_node;
	list_ID* temp;

	new_node = NULL;							//Preparing new node
	new_node = kmalloc(sizeof(list_ID), GFP_KERNEL);
	new_node->ID = toAdd;

	if(*head == NULL){
		//printk(KERN_ALERT "Head Null");
		new_node->next = NULL;
		*head = new_node;
	}
	else{
		//printk(KERN_ALERT "Head not Null");
		temp = *head;
		*head = new_node;
		new_node->next = temp;
	}
}

/*	Appending a new node to the end of the list_node	*/
void addnewList_IDNode(list_ID** head, pid_t toAdd){
	list_ID* crawler;
	list_ID* new_node;
	if(*head == NULL){
		//printk(KERN_ALERT"head == NULL in function %s at line %d\n", __FUNCTION__, __LINE__);
		pushToListID(head,toAdd);
		return;
	}
	crawler = *head;
	while(crawler->next != NULL){						//traversing list till the location of new node is found !
		if(crawler->ID == toAdd)						//maintains a non-duplicate list !
			return;
		crawler = crawler->next;
	}

	new_node = NULL;							//Preparing new node
	new_node = kmalloc(sizeof(list_ID), GFP_KERNEL);
	new_node->ID = toAdd;
	new_node->next = NULL;

	crawler->next = new_node;							//Adding new node
}

void printList_ID(list_ID* crawler){
	//printk("Printing ID List: \n");
	while(crawler != NULL){								//traversing list till NULL
		//printk(KERN_INFO"%d -> ",crawler->ID);
		crawler = crawler->next;
	}
	//printk("|NULL|\n");
}

void deletelist_ID(list_ID* crawler){
	int i;
	i=0;
	while(crawler != NULL){								//traversing list till NULL
		list_ID* temp = crawler->next;
		kfree(crawler);
		crawler = temp;
		i++;
	}
	//printk("Deleted List with %d members \n" , i);
}

/*******************************************************************/


/**************	LINKED LIST FUNCTIONS FOR list_node *********************/

/*	Adding a node to the beginning of the list	*/
void pushToTaskList(list_node** head, struct task_struct* toAdd, list_ID** ID_Head){
	list_node* new_node;
	list_node* temp;

	new_node = NULL;							//Preparing new node
	new_node = kmalloc(sizeof(list_node), GFP_KERNEL);
	new_node->task = toAdd;
	//addnewList_IDNode(ID_Head, pid_vnr(task_pgrp(new_node->task)));
	addnewList_IDNode(ID_Head, new_node->task->pid);
	//printk(KERN_ALERT "Pushed : %d \t %s with PGID: %d\n",new_node->task->pid, new_node->task->comm, pid_vnr(task_pgrp(new_node->task)));
	if(*head == NULL){
		//printk(KERN_ALERT "Head Null in function %s() \n",__FUNCTION__);
		new_node->next = NULL;
		*head = new_node;
	}
	else{
		//printk(KERN_ALERT "Head not Null in function %s() \n", __FUNCTION__);
		temp = *head;
		*head = new_node;
		new_node->next = temp;
	}
}

/*	Appending a new node to the end of the list_node	*/
void addNodeToTaskList(list_node* head, struct task_struct* toAdd ,list_ID** ID_Head){
	list_node* crawler;
	list_node* new_node;
	if(head == NULL){
		//printk(KERN_ALERT"head == NULL in function %s at line %d\n", __FUNCTION__, __LINE__);
		return;
	}
	crawler = head;
	while(crawler->next != NULL){						//traversing list till the location of new node is found
		crawler = crawler->next;
	}

	new_node = NULL;									//Preparing new node
	new_node = kmalloc(sizeof(list_node), GFP_KERNEL);
	new_node->task = toAdd;
	new_node->next = NULL;
	//addnewList_IDNode(ID_Head, pid_vnr(task_pgrp(new_node->task)));
	addnewList_IDNode(ID_Head, new_node->task->pid);
	crawler->next = new_node;							//Adding new node	
}

void printTaskList(list_node* head){
	list_node* crawler;
	crawler = head;
	//printk("Printing List: \n");
	while(crawler != NULL){								//traversing list till NULL
		//printk(KERN_INFO"children name : %d \t %s  with PGID %d\n",crawler->task->pid,
		//	crawler->task->comm, pid_vnr(task_pgrp(crawler->task)));
		crawler = crawler->next;
	}
}

void killTaskList(list_node* head){
	struct siginfo info;

	list_node* crawler;
	list_node* temp;
	crawler = head;

	memset(&info, 0, sizeof(struct siginfo));
	info.si_signo = SIGKILL;
	info.si_code = SI_KERNEL;
	printk("Killing Lists: \n");
	printk(" PID \t CMD\n");
	crawler = crawler->next;
	//crawler = crawler->next;
	while(crawler != NULL){
		printk(" %d \t %s\n",crawler->task->pid, crawler->task->comm);			//traversing list till NULL
		temp = crawler->next;
		send_sig_info(SIGKILL, &info, crawler->task);    		//send the signal
		crawler = temp;
	}
}

void deleteTaskList(list_node* head){
	list_node* crawler;
	int i;
	i=0;
	crawler = head;
	while(crawler != NULL){								//traversing list till NULL
		list_node* temp = crawler->next;
		kfree(crawler);
		crawler = temp;
		i++;
	}
	//printk("Deleted List with %d members \n" , i);
}

/*******************************************************************/

void BFS(struct task_struct* task){					//TODO: delete the head and adjust it so that the list takes less space.

	int process_count;
	struct list_head *list;
	list_node* crawler_head ;
	list_node* head = NULL;
	list_ID* ID_Head = NULL;
	pushToTaskList(&head, task, &ID_Head);
	if(head==NULL){
		//printk(KERN_ALERT"Head Null in function %s at line %d\n", __FUNCTION__, __LINE__);
		return;
	}
	//printk("checking children of %s\n" , task->comm);
	crawler_head = head;
	process_count = 1;
	while(crawler_head!=NULL){
		list_for_each(list,&(crawler_head->task->children))
		{
			task = list_entry(list, struct task_struct, sibling);
			addNodeToTaskList(crawler_head,task, &ID_Head);
			//printk(KERN_ALERT "Adding : %d \t %s \n",task->pid, task->comm);
			process_count++;
		}
		// insert code to adjust head here
		crawler_head = crawler_head->next;
	}
	if(process_count > THRESHOLD){
		printk(KERN_ALERT"\n\n FORK BOMB FOUND with PGID %d and having %d processes\n\n",
			pid_vnr(task_pgrp(task)), process_count);
		killTaskList(head);
	}
	//printTaskList(head);
	printList_ID(ID_Head);
	deleteTaskList(head);
	deletelist_ID(ID_Head);
}

static int my_name(void* data)
{
    struct task_struct *task, *taskBash=NULL;
	struct list_head *list; 
    //struct task_struct *curr=get_current();
	pid_t bashPId;
	rwlock_t rd_lock;
	
	rwlock_init(&rd_lock);
	
	bashPId = *((pid_t*)data);
    printk(KERN_ALERT "--------Thread Function Starts Here-----%d\n",bashPId);

	read_lock(&rd_lock);
	for_each_process(task)
	{
		if(task->pid == bashPId)
		{	taskBash = task; }
	}
	read_unlock(&rd_lock);
	
	if(taskBash == NULL)
	{	return -1;	}
	
	printk(KERN_INFO "bash PID: %d" , bashPId);
	
	while(!kthread_should_stop())	
	{
//		printk(KERN_ALERT "******* START DETECTION ******\n");

		for_each_process(task)
		{
			if(!strcmp(task->comm , "gnome-terminal"))
			{	taskBash = task; }
		}
		read_unlock(&rd_lock);
	
		if(taskBash == NULL)
		{	return -1;	}
	
		printk(KERN_INFO "bash PID: %d" , bashPId);

		read_lock(&rd_lock);	
		list_for_each(list,&taskBash->children)
		{
			task = list_entry(list, struct task_struct, sibling);
			BFS(task);
		}
		read_unlock(&rd_lock);

		//msleep(20000);
		down_interruptible(&gatekeeper);
		printk("Received up signal\n");
	}
	kfree(((pid_t*)data));
	printk("Exiting Thread\n");
	return 0;
}

static int __init init_func(void)
{
    pid_t* pointer_bashPID;
	pointer_bashPID = kmalloc(sizeof(pid_t),GFP_KERNEL);
	*pointer_bashPID = current->parent->parent->parent->pid;
	
	printk("\n\n-------------------------------------------------\n");

	printk(KERN_ALERT"bashPid: %d\nCalling Kernel thread\n", *pointer_bashPID);
	
	down_interruptible(&gatekeeper);
    kernelThread = kthread_run(&my_name,pointer_bashPID,"myfunction");

    return 0;   
}

static void __exit exit_func(void)
{
	up(&gatekeeper);
	kthread_stop(kernelThread);
    return;
}

module_init(init_func);
module_exit(exit_func);

MODULE_LICENSE("GPL");