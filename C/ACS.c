#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/time.h>

typedef struct customer_info {
    int user_id;
	int class_type; // 0 = economy and 1 = business
	int service_time;
	int arrival_time;
} customer_info;

/*
typedef struct clerk_info {
	int clerk_id;
	int status;
} clerk_info; */

/* ---- Global Variables ---- */

#define TRUE 1
#define FALSE 0
#define BUFFER 1024
#define MAX_CUSTOMERS 100
#define DECI_TO_MICRO_SECS 100000
#define NClerks 5	// Max number of clerks

customer_info customers_info[BUFFER];
customer_info* queue[BUFFER];
int clerkID[5] = {0, 1, 2, 3, 4};

pthread_t clerk_threads[NClerks];
pthread_mutex_t clerk_mutex;
pthread_cond_t clerk_available;

pthread_t customer_threads[MAX_CUSTOMERS];
pthread_mutex_t customer_mutex;
pthread_cond_t customer_convar;

struct timeval init_time; // use this variable to record the simulation start time; No need to use mutex_lock when reading this variable since the value would not be changed by thread once the initial time was set.
double overall_waiting_time; //A global variable to add up the overall waiting time for all customers, every customer add their own waiting time to this variable, mutex_lock is necessary.
int queue_size = 0;
int NCustomers;

/* ---- Globals to Calculate Final Wait Times ---- */
double business_wait = 0.0;
float business_customers = 0;
double economy_wait = 0.0;
float economy_customers = 0;
float avg_wait = 0;


/* ---- Time Functions ---- */

/* Returns the time difference in microseconds between a passed start time to function call */
double getTimeDifference(struct timeval startTime)
{
    struct timeval nowTime;
    gettimeofday(&nowTime, NULL);
    long nowMicroseconds = (nowTime.tv_sec * 10 * DECI_TO_MICRO_SECS) + nowTime.tv_usec;
    long startMicroseconds = (startTime.tv_sec * 10 * DECI_TO_MICRO_SECS) + startTime.tv_usec;
    return (double)(nowMicroseconds - startMicroseconds) / (10 * DECI_TO_MICRO_SECS);
}

/* Returns the time in terms of microseconds */
double getCurSystemTime()
{
    struct timeval nowTime;
    gettimeofday(&nowTime, NULL);
    long nowMicroseconds = (nowTime.tv_sec * 10 * DECI_TO_MICRO_SECS) + nowTime.tv_usec;
    long startMicroseconds = (init_time.tv_sec * 10 * DECI_TO_MICRO_SECS) + init_time.tv_usec;
    return (double)(nowMicroseconds - startMicroseconds) / (10 * DECI_TO_MICRO_SECS);
}

/* ---- Queue Functions ---- */

// for the qsort() function, returns 1 if the curr customer has higher priorty based on class_type and arrival_time
int cmp_clients(customer_info* curr, customer_info* next) {
	if (curr->class_type > next->class_type) {
		return 1;
	} else if (curr->arrival_time > next->arrival_time) {
		return 1;
	} else {
		return -1;
	}
	return 0;
}

void enQueue(customer_info* curr_customer) {
	queue[queue_size] = curr_customer;
	queue_size++;
}

void deQueue() {
	int x = 0;
	while (x < queue_size -1) {
		queue[x] = queue[x + 1];
		x++;
	}
	queue_size--;
}	


/* ---- File Functions ---- */

void read_file_contents(char *input_file, char customer_file_info[BUFFER][BUFFER]) {
	FILE *fp = fopen(input_file, "r");
	if (fp != NULL) {
		int i = 0;
		while (fgets(customer_file_info[i], MAX_CUSTOMERS, fp)) {
			i++;
		}
		fclose(fp);
	} else {
		perror("Unable to read file contents\n");
		exit(1);
	}
} 

void replace_colon(char s[]) {
	int i = 0;
	while (s[i] != '\0') {
		if (s[i] == ':') {
			s[i] = ',';
		}
		i++;
	}
}

void tokenize_file(char customer_file_info[BUFFER][BUFFER], int NCustomers) {
	int i;
	for (i = 1; i < NCustomers; i++) {
		replace_colon(customer_file_info[i]);
		
		int j = 0;
		int customer_details[4];
		
		char *t = strtok(customer_file_info[i], ",");
		while (t != NULL) {
			customer_details[j] = atoi(t);
			t = strtok(NULL, ",");
			j++;
		}

		customer_info curr = {
			customer_details[0],	// ID
			customer_details[1],	// Class
			customer_details[2],	// Arrival time
			customer_details[3]		// Service time
		};

		customers_info[i - 1] = curr;
	}
}

/* ---- Thread Functions ---- */

// function entry for customer threads
void *customer_entry(void *cus_info) {
	
	customer_info *curr_customer = (customer_info *)cus_info;
	struct timeval wait_time;
	
	usleep(curr_customer->arrival_time * DECI_TO_MICRO_SECS);
	
	fprintf(stdout, "A customer arrives: customer ID %2d. \n", curr_customer->user_id);

	gettimeofday(&wait_time, NULL);
	
	
	pthread_mutex_lock(&customer_mutex); {	
		enQueue(curr_customer);
		qsort(queue, NCustomers, sizeof(customer_info), cmp_clients);
		double queue_enter_time = getCurSystemTime();

		fprintf(stdout, "A customer enters a queue: the queue ID %1d, and length of the queue %2d. \n", curr_customer->user_id, queue_size);
		
		while (TRUE) {
			if (pthread_cond_wait(&clerk_available, &customer_mutex) != 0) {
				perror("Failed to execute convar\n");
				exit(1);
			}
			if (curr_customer->user_id == queue[0]) {
				
				if (pthread_mutex_lock(&clerk_mutex) != 0) {
					perror("Failed to lock clerk mutex\n");
					exit(1);
				}

				if (pthread_mutex_unlock(&clerk_available) != 0) {
					perror("Failed to unlock clerk mutex\n");
					exit(1);
				}
				
			} else {
				break;
			}
			break;
		}
		deQueue();		
			
	}
	pthread_mutex_unlock(&customer_mutex); //unlock mutex_lock such that other customers can enter into the queue
	
	/* Try to figure out which clerk awoken me, because you need to print the clerk Id information */
	overall_waiting_time = getTimeDifference(wait_time);
	usleep(10); // Add a usleep here to make sure that all the other waiting threads have already got back to call pthread_cond_wait. 10 us will not harm your simulation time.
	
	
	/* get the current machine time; updates the overall_waiting_time*/
	double start_time = getCurSystemTime();
	fprintf(stdout, "A clerk starts serving a customer: start time %.2f, the customer ID %2d, the clerk ID %1d. \n", start_time, curr_customer->user_id, clerkID);
	
	usleep(curr_customer->service_time * DECI_TO_MICRO_SECS);
	
	double end_time = getCurSystemTime();

	fprintf(stdout, "A clerk finishes serving a customer: end time %.2f, the customer ID %2d, the clerk ID %1d. \n", end_time, curr_customer->user_id, clerkID);\
	
	pthread_cond_signal(&clerk_available); // Notify the clerk that service is finished, it can serve another customer
	
	pthread_exit(NULL);
	
	return NULL;
}

// function entry for clerk threads
void *clerk_entry(void * clerkNum) {
	
	int *clerkID = (int *)clerkNum;

	while(TRUE){
		
		pthread_mutex_lock(&clerk_mutex);
		
		*queue = clerkID; // The current clerk (clerkID) is signaling this queue
		
		pthread_cond_broadcast(&customer_convar); // Awake the customer (the one enter into the queue first) from the selected queue
		
		
		pthread_mutex_unlock(&clerk_mutex);
		
		pthread_cond_wait(&clerk_available, &clerk_mutex); // wait for the customer to finish its service
	}
	
	pthread_exit(NULL);
	
	return NULL;
}


int main(int argc, char *argv[]) {

	if (argc < 2) {
		perror("You need two arguments\n");
		exit(1);
	}

	char customer_file_info[BUFFER][BUFFER];
	read_file_contents(argv[1], customer_file_info);	// Get the file contents

	int NCustomers = atoi(customer_file_info[0]);	// Number of customers from file
	tokenize_file(customer_file_info, NCustomers);	// Parse file

	// Initialize clerk mutex and convar
	if (pthread_mutex_init(&clerk_mutex, NULL) != 0) {
		perror("Failed to initiate clerk mutex\n");
		exit(1);
	}
	if (pthread_cond_init(&clerk_available, NULL) != 0) {
		perror("Failed to initiate clerk conditional variable\n");
		exit(1);
	}
	// Initialize customer mutex and convar
	if (pthread_mutex_init(&customer_mutex, NULL) != 0) {
		perror("Failed to initiate customer mutex\n");
		exit(1);
	}
	if (pthread_cond_init(&customer_convar, NULL) != 0) {
		perror("Failed to initiate customer conditional variable\n");
		exit(1);
	}
	// Initialize attr and create threads in joinable state
	pthread_attr_t attr;
	if (pthread_attr_init(&attr) != 0) {
		perror("Failed to initialize attr\n");
		exit(1);
	}

	if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE) != 0) {
		perror("Failed to set detatchstate\n");
		exit(1);
	}

	gettimeofday(&init_time, NULL);

	int i;
	//create clerk thread (optional)
	for (i = 0; i < NClerks; i++) { // number of clerks
		if (pthread_create(&clerk_threads[i], &attr, clerk_entry, (void *)&clerkID[i]) != 0) {
			perror("Failed to create clerk pthread\n");
		}; // clerk_info: passing the clerk information (e.g., clerk ID) to clerk thread
	}
	
	//create customer thread
	for (i = 0; i < NCustomers; i++) { // number of customers
		if (pthread_create(&customer_threads[i], NULL, customer_entry, (void *)&customers_info[i]) != 0) {
			perror("Failed to create customer pthread\n");
		} //custom_info: passing the customer information (e.g., customer ID, arrival time, service time, etc.) to customer thread
	}
	// wait for all clerk and customer threads to terminate
	for (i = 0; i < NCustomers; i++) {
		if (pthread_join(clerk_threads[i], NULL) != 0) {
			perror("Failed to join clerk pthread\n");
			exit(1);
		}
	}
	
	for (i = 0; i < NCustomers; i++) {
		if (pthread_join(customer_threads[i], NULL) != 0) {
			perror("Failed to join customer pthread\n");
			exit(1);
		}
	}
	// destroy mutex & condition variable (optional)
	if (pthread_attr_destroy(&attr) != 0) {
		perror("Failed to destroy attr\n");
		exit(1);
	}
	
	if (pthread_mutex_destroy(&clerk_mutex) != 0) {
		perror("Failed to destroy clerk mutex\n");
		exit(1);
	}
	if (pthread_cond_destroy(&clerk_available) != 0) {
		perror("Failed to destroy clerk convar");
		exit(1);
	}

	if (pthread_mutex_destroy(&customer_mutex) != 0) {
		perror("Failed to destroy customer mutex\n");
		exit(1);
	}
	if (pthread_cond_destroy(&customer_convar) != 0) {
		perror("Failed to destroy customer convar");
		exit(1);
	}
	
	// calculate the average waiting time of all customers

	avg_wait = overall_waiting_time / NCustomers;
	business_wait = business_wait / business_customers;
	economy_wait = economy_wait / economy_customers;

	printf("The average waiting time for all %d customers in the system is: %.2f seconds. \n", NCustomers, avg_wait);
    printf("The average waiting time for all %d business-class customers is: %.2f seconds. \n", (int)business_customers, business_wait);
    printf("The average waiting time for all %d economy-class customers is: %.2f seconds. \n", (int)economy_customers, economy_wait);

	pthread_exit(NULL);
	return 0;
}