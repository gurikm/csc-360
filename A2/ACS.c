
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

//info for customers
typedef struct customer_info {
    int id;
    int class_type;
    int arrival_time;
    int service_time;
    double start_time;
    double end_time;
} customer_info;
 
//info for clerks
typedef struct clerk {
    int id;
} clerk;

//Used to create linked list queue
typedef struct node_t {
    customer_info* customer_info;
    struct node_t* next;
    struct node_t* head;
    struct node_t* tail;
    int size;
} node_t;

void *customer_entry(void * cus_info);
void *clerk_entry(void * clerkNum);

/******************************************************Global Variables*******************************************************************************/
int B_counter = 0;
int E_counter = 0;
clerk clerk_info[4];
node_t* business_Queue;
node_t* economy_Queue;
struct timeval start_time; //start time used to count customer wait time.
double total_wait_time;
double business_wait_time;
double econ_wait_time;
int queue_stat[2] = {0};
//use for clerk lock and unlock and convar wait and signal
pthread_mutex_t clerk1_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t clerk2_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t clerk3_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t clerk4_mut = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t clerk1_con = PTHREAD_COND_INITIALIZER;
pthread_cond_t clerk2_con = PTHREAD_COND_INITIALIZER;
pthread_cond_t clerk3_con = PTHREAD_COND_INITIALIZER;
pthread_cond_t clerk4_con = PTHREAD_COND_INITIALIZER;

pthread_mutex_t Business_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t economy_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t Business_con = PTHREAD_COND_INITIALIZER;
pthread_cond_t economy_con = PTHREAD_COND_INITIALIZER;

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_con = PTHREAD_COND_INITIALIZER;
pthread_mutex_t total = PTHREAD_MUTEX_INITIALIZER;

/******************************************************Global Variables*******************************************************************************/

/*************************************************Basic linked list set up**********************************************************************************/
//creates nodes that will go into the queue
node_t* constructNode(customer_info* currCustomer) {
    node_t* node = (node_t*) malloc(sizeof (node_t));
    node->customer_info = currCustomer;
    node->next = NULL;
    return node;
}
//creates the queue
node_t* constructQueue() {
    node_t* queue = (node_t*) malloc(sizeof (node_t));
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    return queue;
}
//adds node to the queue
void add (node_t* add, node_t* newNode) {
    if(add->size == 0) {
        add->head = newNode;
        add->tail = newNode;
    }else {
        add->tail->next = newNode;
        add->tail = newNode;
    }
    add->size++;
}
//removes node from the queue
void remove_node(node_t* remove) {
    if(remove->size == 0) {
        exit(-1);
    }else if(remove->size == 1) {
        remove->head = NULL;
        remove->tail = NULL;
        remove->size--;
    }else {
        remove->head = (remove->head)->next;
        remove->size--;
    }
}
/*************************************************Basic linked list set up**********************************************************************************/

/****************************************************Helper Functions***************************************************************************************/
//used to get the current time in the simulations
double getCurrentSimulationTime() {
    struct timeval curr_time;
    double curr_secs,init_secs;

    init_secs = (start_time.tv_sec + (double) start_time.tv_usec / 1000000);
    gettimeofday(&curr_time, NULL);
    curr_secs = (curr_time.tv_sec + (double) curr_time.tv_usec / 1000000);

    return (curr_secs - init_secs);
}
//used to open text files for ACS
void readfile(char* file, char fileInfo[128][128]) {
    FILE *file_open = fopen(file, "r");
    if(file_open != NULL) {
        int count = 0;
        while(1) {
            fgets(fileInfo[count++], 128, file_open);
            if(feof(file_open)) break;
        }
    }else {
        perror("Error while reading file.\n");
    }
    fclose(file_open);
}

/****************************************************Helper Functions***************************************************************************************/

/*****************************************************Main Function*****************************************************************************************/
int main(int argc, char* argv[]) {
    char file[128][128];
    pthread_t customerId[128];
    pthread_t clerkId[4];
    readfile(argv[1], file);
    int clerk_info[4] = {1,2,3,4};
    business_Queue = constructQueue();
    economy_Queue = constructQueue();
    customer_info customer_information[128];
    int NCustomers = atoi(file[0]);
    //tokenizer
    for(int i = 0; i<=NCustomers; i++) {
        int j = 0;
        while(file[i][j] != '\0') {
            if(file[i][j] == ':' || file[i][j] == ',') {
                file[i][j] = ' ';
            }
            j++;
        }
        customer_info person = {atoi(&file[i][0]),atoi(&file[i][2]),atoi(&file[i][4]),atoi(&file[i][6])};
        customer_information[i-1] = person;
    }
    //create the clerk threads
    for(int i = 0; i < 4; i++) {
        if(pthread_create(&clerkId[i], NULL, clerk_entry,(void*)&clerk_info[i])) {
            printf("clerk threads failed\n");
        }
    }
    //initial the time
    gettimeofday(&start_time, NULL);
    //create the customer threads
    for(int i = 0; i < NCustomers; i++) {
        if(pthread_create(&customerId[i], NULL, customer_entry,(void*) &customer_information[i])) {
            printf("customer threads failed\n");
        }
    }
    //join the customer threads back to main thread or main function
    for(int x = 0; x < NCustomers; x++) {
        if(pthread_join(customerId[x], NULL)){
            printf("Failed to join threads to main\n");
        }
    }
    // calculate the average waiting time of all customers
    printf("The average waiting time for all customers in the system is: %.2f seconds. \n", total_wait_time/NCustomers);
    printf("The average waiting time for all business-class customers is: %.2f seconds. \n", business_wait_time/B_counter);
    printf("The average waiting time for all economy-class customers is: %.2f seconds. \n", econ_wait_time/E_counter);
    return 0;
}
/*****************************************************Main Function*****************************************************************************************/
/***********************************************function entry for customer threads*************************************************************************/
void* customer_entry(void* cus_info) {
    customer_info* p_myInfo = (customer_info*) cus_info;
    /* the arrival time of this customer */
    usleep(p_myInfo->arrival_time * 100000);
    printf("A customer arrives: customer ID %2d. \n", p_myInfo->id);
    /* Enqueue operation: get into either business queue or economy queue by using p_myInfo->class_type*/
    if(p_myInfo->class_type == 0) {
        B_counter++;
        pthread_mutex_lock(&Business_mutex);
        add(business_Queue, constructNode(p_myInfo)); //add the node into the queue
        pthread_cond_signal(&queue_con); //send signal to clerk the customer arrived
        printf("A customer enters a queue: the queue ID %1d, and length of the queue %2d. \n", p_myInfo->class_type, business_Queue->size);
        p_myInfo->start_time = getCurrentSimulationTime();
        //wait the clerk signal when the clerk is ready.
        pthread_cond_wait(&Business_con, &Business_mutex);
        remove_node(business_Queue); //remove the node from queue
        /* mutexLock of selected queue */
        pthread_mutex_unlock(&Business_mutex);
        usleep(20);
        int clerk_active = queue_stat[p_myInfo->class_type];
        queue_stat[p_myInfo->class_type] = 0;
        p_myInfo->end_time = getCurrentSimulationTime();
        pthread_mutex_lock(&total);
        total_wait_time += p_myInfo->end_time - p_myInfo->start_time;
        econ_wait_time += p_myInfo->end_time - p_myInfo->start_time;
        pthread_mutex_unlock(&total);
    
        printf("A clerk starts serving a customer: start time %.2f, the customer ID %2d, the clerk ID %1d. \n", p_myInfo->end_time, p_myInfo->id, clerk_active);
        usleep(p_myInfo->service_time * 100000);
        double end_serving_time = getCurrentSimulationTime();
        printf("A clerk finishes serving a customer: end time %.2f, the customer ID %2d, the clerk ID %1d. \n", end_serving_time, p_myInfo->id, clerk_active);
    
        if(clerk_active == 1) {
            pthread_cond_signal(&clerk1_con);
        }else if(clerk_active == 2) {
            pthread_cond_signal(&clerk2_con);
        }else if(clerk_active == 3) {
            pthread_cond_signal(&clerk3_con);
        }else if(clerk_active == 4){
            pthread_cond_signal(&clerk4_con);
        }
        //for economy class
    }else {
        E_counter++;
        pthread_mutex_lock(&economy_mutex);
         
        add(economy_Queue, constructNode(p_myInfo));//add the node into the queue
        pthread_cond_signal(&queue_con);//send signal to clerk the customer arrived
        printf("A customer enters a queue: the queue ID %1d, and length of the queue %2d. \n", p_myInfo->class_type, economy_Queue->size);
        p_myInfo->start_time = getCurrentSimulationTime();
        pthread_cond_wait(&economy_con, &economy_mutex);
        remove_node(economy_Queue);
        pthread_mutex_unlock(&economy_mutex);
        usleep(20);
        int clerk_active = queue_stat[p_myInfo->class_type];
        queue_stat[p_myInfo->class_type] = 0;
        p_myInfo->end_time = getCurrentSimulationTime();
        pthread_mutex_lock(&total);
        total_wait_time += p_myInfo->end_time - p_myInfo->start_time;
        business_wait_time += p_myInfo->end_time - p_myInfo->start_time;
        pthread_mutex_unlock(&total);
 
        printf("A clerk starts serving a customer: start time %.2f, the customer ID %2d, the clerk ID %1d. \n", p_myInfo->end_time, p_myInfo->id, clerk_active);
        usleep(p_myInfo->service_time * 100000);
        double end_serving_time = getCurrentSimulationTime();
        printf("A clerk finishes serving a customer: end time %.2f, the customer ID %2d, the clerk ID %1d. \n", end_serving_time, p_myInfo->id, clerk_active);
        if(clerk_active == 1) {
            pthread_cond_signal(&clerk1_con);
        }else if(clerk_active == 2) {
            pthread_cond_signal(&clerk2_con);
        }else if(clerk_active == 3) {
            pthread_cond_signal(&clerk3_con);
        }else if(clerk_active == 4){
            pthread_cond_signal(&clerk4_con);
        }
    }
    pthread_exit(NULL);
    return NULL;
}

/***********************************************function entry for customer threads*************************************************************************/
/************************************************function entry for clerk threads***************************************************************************/
void* clerk_entry(void* clerkNum) {
    while(1) {
        //selected_queue_ID = Select the queue based on the priority and current customers number
        int selected_queue_ID = 0;
        int* clerk_ID = (int *) clerkNum;
       /* mutexLock of the selected queue */
        pthread_mutex_lock(&queue_mutex);
        while (business_Queue->size == 0 && economy_Queue->size == 0) {
        pthread_cond_wait(&queue_con, &queue_mutex);
        }
        pthread_mutex_unlock(&queue_mutex);
        if(business_Queue->size != 0) {
            pthread_mutex_lock(&Business_mutex);
            queue_stat[selected_queue_ID] = *clerk_ID;;
            // Awake the customer (the one enter into the queue first) from the selected queue
            pthread_cond_broadcast(&Business_con);
            pthread_mutex_unlock(&Business_mutex);
            if(*clerk_ID == 1 ) {
                pthread_mutex_lock(&clerk1_mut);
                pthread_cond_wait(&clerk1_con, &clerk1_mut);
                pthread_mutex_unlock(&clerk1_mut);
            }else if(*clerk_ID == 2 ) {
                pthread_mutex_lock(&clerk2_mut);
                pthread_cond_wait(&clerk2_con, &clerk2_mut);
                pthread_mutex_unlock(&clerk2_mut);
            }else if(*clerk_ID == 3 ) {
                pthread_mutex_lock(&clerk3_mut);
                pthread_cond_wait(&clerk3_con, &clerk3_mut);
                pthread_mutex_unlock(&clerk3_mut);
            }else{
                pthread_mutex_lock(&clerk4_mut);
                pthread_cond_wait(&clerk4_con, &clerk4_mut);
                pthread_mutex_unlock(&clerk4_mut);
            }
        }else{
            selected_queue_ID = 1;
            pthread_mutex_lock(&economy_mutex);
            queue_stat[selected_queue_ID] = *clerk_ID;
            pthread_cond_broadcast(&economy_con); 
            pthread_mutex_unlock(&economy_mutex);
            if(*clerk_ID == 1 ) {
                pthread_mutex_lock(&clerk1_mut);
                pthread_cond_wait(&clerk1_con, &clerk1_mut);
                pthread_mutex_unlock(&clerk1_mut);
            }else if(*clerk_ID == 2 ) {
                pthread_mutex_lock(&clerk2_mut);
                pthread_cond_wait(&clerk2_con, &clerk2_mut);
                pthread_mutex_unlock(&clerk2_mut);
            }else if(*clerk_ID == 3 ) {
                pthread_mutex_lock(&clerk3_mut);
                pthread_cond_wait(&clerk3_con, &clerk3_mut);
                pthread_mutex_unlock(&clerk3_mut);
            }else{
                pthread_mutex_lock(&clerk4_mut);
                pthread_cond_wait(&clerk4_con, &clerk4_mut);
                pthread_mutex_unlock(&clerk4_mut);
            }
        }
    }
    return NULL;
}
/************************************************function entry for clerk threads***************************************************************************/
