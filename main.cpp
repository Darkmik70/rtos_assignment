// compile with: g++ -lpthread <sourcename> -o <executablename>

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/types.h>

// code of periodic tasks
void task1_code();
void task2_code();
void task3_code();

// code of aperiodic tasks
void task4_code();

// characteristic function of the thread, only for timing and synchronization
// periodic tasks
void *task1(void *);
void *task2(void *);
void *task3(void *);

// aperiodic tasks
void *task4(void *);

// function to waste time
void waste_time();

// Function to write to a file
void write_to_file(int fd, const char *msg);

// Open the special file
int open_simple_driver();

// initialization of mutexes and conditions (only for aperiodic scheduling)
pthread_mutex_t mutex_task_4 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_task_4 = PTHREAD_COND_INITIALIZER;

#define INNERLOOP 1000
#define OUTERLOOP 100

#define NPERIODICTASKS 3
#define NAPERIODICTASKS 1
#define NTASKS NPERIODICTASKS + NAPERIODICTASKS

// declaration of mutex
pthread_mutex_t mutex_sem = PTHREAD_MUTEX_INITIALIZER;
// mutex attribute
pthread_mutexattr_t mutexattr_sem;

long int periods[NTASKS];
struct timespec next_arrival_time[NTASKS];
double WCET[NTASKS];
pthread_attr_t attributes[NTASKS];
pthread_t thread_id[NTASKS];
struct sched_param parameters[NTASKS];
int missed_deadlines[NTASKS];

int main()
{
	//(1) Design an application with 3 threads J1, J2, J3, whose periods are 300ms, 500ms, and 800ms,
	// plus an aperiodic thread J4 in background which is triggered by J2.

	// the first task has period 300 millisecond
	periods[0] = 300000000; // nanoseconds
							// the second task has period 500 millisecond
	periods[1] = 500000000; // nanoseconds
							// the third task has period 800 millisecond
	periods[2] = 800000000; // nanoseconds

	// for aperiodic tasks we set the period equals to 0
	periods[3] = 0;

	// this is not strictly necessary, but it is convenient to
	// assign a name to the maximum and the minimum priotity in the
	// system. We call them priomin and priomax.
	struct sched_param priomax;
	priomax.sched_priority = sched_get_priority_max(SCHED_FIFO);
	struct sched_param priomin;
	priomin.sched_priority = sched_get_priority_min(SCHED_FIFO);

	// set the maximum priority to the current thread (you are required to be
	// superuser). Check that the main thread is executed with superuser privileges
	// before doing anything else.
	if (getuid() == 0)
		pthread_setschedparam(pthread_self(), SCHED_FIFO, &priomax);

	// Open the file for error handlings
	int fd = open_simple_driver();

	// initialize mutex attributes
	pthread_mutexattr_init(&mutexattr_sem);

	// set mutex protocol to priority ceiling
	pthread_mutexattr_setprotocol(&mutexattr_sem, PTHREAD_PRIO_PROTECT);

	// set the priority ceiling of the mutex to the maximum priority
	pthread_mutexattr_setprioceiling(&mutexattr_sem, priomin.sched_priority + NTASKS);

	// initialize the mutex
	pthread_mutex_init(&mutex_sem, &mutexattr_sem);

	// execute all tasks in standalone modality in order to measure execution times
	// (use gettimeofday). Use the computed values to update the worst case execution
	// time of each task.
	for (int i = 0; i < NTASKS; i++)
	{

		// initializa time_1 and time_2 required to read the clock
		struct timespec time_1, time_2;
		clock_gettime(CLOCK_REALTIME, &time_1);

		// we should execute each task more than one for computing the WCET
		// periodic tasks
		if (i == 0)
			task1_code();
		if (i == 1)
			task2_code();
		if (i == 2)
			task3_code();

		// aperiodic tasks
		if (i == 3)
			task4_code();

		clock_gettime(CLOCK_REALTIME, &time_2);

		// compute the Worst Case Execution Time (in a real case, we should repeat this many times under
		// different conditions, in order to have reliable values

		WCET[i] = 1000000000 * (time_2.tv_sec - time_1.tv_sec) + (time_2.tv_nsec - time_1.tv_nsec);
		printf("\nWorst Case Execution Time %d=%f \n", i, WCET[i]);
		fflush(stdout);
		char msg[100];
		sprintf(msg, "\nWorst Case Execution Time %d=%f \n", i, WCET[i]);
		write_to_file(fd, msg);
	}

	// compute U
	double U = WCET[0] / periods[0] + WCET[1] / periods[1] + WCET[2] / periods[2];

	// compute Ulub by considering the fact that we have harmonic relationships between periods
	double Ulub = 1;

	// if there are no harmonic relationships, use the following formula instead
	// double Ulub = NPERIODICTASKS*(pow(2.0,(1.0/NPERIODICTASKS)) -1);

	// check the sufficient conditions: if they are not satisfied, exit
	if (U > Ulub)
	{
		printf("\n U=%lf Ulub=%lf Non schedulable Task Set \n", U, Ulub);
		return (-1);
	}
	printf("\n U=%lf Ulub=%lf Scheduable Task Set \n", U, Ulub);
	fflush(stdout);

	char msg[100];
	sprintf(msg, "\n U=%lf Ulub=%lf Scheduable Task Set \n", U, Ulub);
	write_to_file(fd, msg);
	close(fd);
	sleep(5);

	// set the minimum priority to the current thread: this is now required because
	// we will assign higher priorities to periodic threads to be soon created
	// pthread_setschedparam

	if (getuid() == 0)
		pthread_setschedparam(pthread_self(), SCHED_FIFO, &priomin);

	// set the attributes of each task, including scheduling policy and priority
	for (int i = 0; i < NPERIODICTASKS; i++)
	{
		// initializa the attribute structure of task i
		pthread_attr_init(&(attributes[i]));

		// set the attributes to tell the kernel that the priorities and policies are explicitly chosen,
		// not inherited from the main thread (pthread_attr_setinheritsched)
		pthread_attr_setinheritsched(&(attributes[i]), PTHREAD_EXPLICIT_SCHED);

		// set the attributes to set the SCHED_FIFO policy (pthread_attr_setschedpolicy)
		pthread_attr_setschedpolicy(&(attributes[i]), SCHED_FIFO);

		// properly set the parameters to assign the priority inversely proportional
		// to the period
		// parameters[i].sched_priority = priomin.sched_priority+NTASKS - i;
		parameters[i].sched_priority = sched_get_priority_max(SCHED_FIFO) - i;

		// set the attributes and the parameters of the current thread (pthread_attr_setschedparam)
		pthread_attr_setschedparam(&(attributes[i]), &(parameters[i]));
	}

	// aperiodic tasks
	for (int i = NPERIODICTASKS; i < NTASKS; i++)
	{
		pthread_attr_init(&(attributes[i]));
		pthread_attr_setschedpolicy(&(attributes[i]), SCHED_FIFO);

		// set minimum priority (background scheduling)
		parameters[i].sched_priority = 0;
		pthread_attr_setschedparam(&(attributes[i]), &(parameters[i]));
	}

	// declare the variable to contain the return values of pthread_create
	int iret[NTASKS];

	// declare variables to read the current time
	struct timespec time_1;
	clock_gettime(CLOCK_REALTIME, &time_1);

	// set the next arrival time for each task. This is not the beginning of the first
	// period, but the end of the first period and beginning of the next one.
	for (int i = 0; i < NPERIODICTASKS; i++)
	{
		long int next_arrival_nanoseconds = time_1.tv_nsec + periods[i];
		// then we compute the end of the first period and beginning of the next one
		next_arrival_time[i].tv_nsec = next_arrival_nanoseconds % 1000000000;
		next_arrival_time[i].tv_sec = time_1.tv_sec + next_arrival_nanoseconds / 1000000000;
		missed_deadlines[i] = 0;
	}

	// create all threads(pthread_create)
	iret[0] = pthread_create(&(thread_id[0]), &(attributes[0]), task1, NULL);
	iret[1] = pthread_create(&(thread_id[1]), &(attributes[1]), task2, NULL);
	iret[2] = pthread_create(&(thread_id[2]), &(attributes[2]), task3, NULL);
	iret[3] = pthread_create(&(thread_id[3]), &(attributes[3]), task4, NULL);

	// join all threads (pthread_join)
	pthread_join(thread_id[0], NULL);
	pthread_join(thread_id[1], NULL);
	pthread_join(thread_id[2], NULL);

	// set the next arrival time for each task. This is not the beginning of the first
	// period, but the end of the first period and beginning of the next one.
	fd = open_simple_driver();

	for (int i = 0; i < NTASKS; i++)
	{
		if (i == 0)
			printf("\n");
		printf("Missed Deadlines Task %d = %d \n", i, missed_deadlines[i]);
		char msg[100];
		sprintf(msg, "Missed Deadlines Task %d = %d \n", i, missed_deadlines[i]);
		write_to_file(fd, msg);
		fflush(stdout);
	}
	close(fd);
	exit(0);
}

// application specific task1_code
void task1_code()
{
	const char *str_begin = " [1 ";
	const char *str_end = " 1] ";

	int fd = open_simple_driver();
	write_to_file(fd, str_begin);
	close(fd);

	waste_time();

	// repeat
	fd = open_simple_driver();
	write_to_file(fd, str_end);
	close(fd);
}

// thread code for task_1 (used only for temporization)
void *task1(void *ptr)
{
	// set thread affinity, that is the processor on which threads shall run
	cpu_set_t cset;
	CPU_ZERO(&cset);
	CPU_SET(0, &cset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

	// execute the task one hundred times... it should be an infinite loop (too dangerous)
	for (int i = 0; i < 100; i++)
	{
		// lock mutex
		pthread_mutex_lock(&mutex_sem);
		// execute application specific code
		task1_code();

		// unlock mutex
		pthread_mutex_unlock(&mutex_sem);

		// check if we didnt miss the deadline
		clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_arrival_time[0], NULL);

		// the thread is ready and can compute the end of the current period for
		// the next iteration
		long int next_arrival_nanoseconds = next_arrival_time[0].tv_nsec + periods[0];
		next_arrival_time[0].tv_nsec = next_arrival_nanoseconds % 1000000000;
		next_arrival_time[0].tv_sec = next_arrival_time[0].tv_sec + next_arrival_nanoseconds / 1000000000;
	}
}

// Task 2 signals aperiodic task 4
void task2_code()
{
	const char *str_begin = " [2 ";
	const char *str_end = " 2] ";

	int fd = open_simple_driver();
	write_to_file(fd, str_begin);
	close(fd);

	// Task 4 should block task 2
	pthread_cond_signal(&cond_task_4);


	waste_time();


	// repeat
	fd = open_simple_driver();
	write_to_file(fd, str_end);
	close(fd);
}

void *task2(void *ptr)
{
	// set thread affinity, that is the processor on which threads shall run
	cpu_set_t cset;
	CPU_ZERO(&cset);
	CPU_SET(0, &cset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

	for (int i = 0; i < 100; i++)
	{
		// lock mutex
		pthread_mutex_lock(&mutex_sem);

		task2_code();

		// unlock mutex
		pthread_mutex_unlock(&mutex_sem);

		clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_arrival_time[1], NULL);
		long int next_arrival_nanoseconds = next_arrival_time[1].tv_nsec + periods[1];
		next_arrival_time[1].tv_nsec = next_arrival_nanoseconds % 1000000000;
		next_arrival_time[1].tv_sec = next_arrival_time[1].tv_sec + next_arrival_nanoseconds / 1000000000;
	}
}

void task3_code()
{
	const char *str_begin = " [3 ";
	const char *str_end = " 3] ";

	int fd = open_simple_driver();
	write_to_file(fd, str_begin);
	close(fd);

	waste_time();

	// repeat
	fd = open_simple_driver();
	write_to_file(fd, str_end);
	close(fd);
}

void *task3(void *ptr)
{
	// set thread affinity, that is the processor on which threads shall run
	cpu_set_t cset;
	CPU_ZERO(&cset);
	CPU_SET(0, &cset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

	for (int i = 0; i < 100; i++)
	{
		// lock mutex
		pthread_mutex_lock(&mutex_sem);

		task3_code();

		// unlock mutex
		pthread_mutex_unlock(&mutex_sem);

		clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_arrival_time[2], NULL);
		long int next_arrival_nanoseconds = next_arrival_time[2].tv_nsec + periods[2];
		next_arrival_time[2].tv_nsec = next_arrival_nanoseconds % 1000000000;
		next_arrival_time[2].tv_sec = next_arrival_time[2].tv_sec + next_arrival_nanoseconds / 1000000000;
	}
}

void task4_code()
{
	const char *str_begin = " [4 ";
	const char *str_end = " 4] ";

	int fd = open_simple_driver();
	write_to_file(fd, str_begin);
	close(fd);

	waste_time();

	// repeat
	fd = open_simple_driver();
	write_to_file(fd, str_end);
	close(fd);
}

void *task4(void *)
{
	// set thread affinity, that is the processor on which threads shall run
	cpu_set_t cset;
	CPU_ZERO(&cset);
	CPU_SET(0, &cset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

	// add an infinite loop
	while (1)
	{
		// this guy waits for the proper condition
		pthread_cond_wait(&cond_task_4, &mutex_task_4);

		// lock the mutex
		pthread_mutex_lock(&mutex_sem);

		// execute the task code
		task4_code();

		pthread_mutex_unlock(&mutex_sem);
	}
}

// Function to do something to waste the time
void waste_time()
{
	double wt;
	for (int i = 0; i < OUTERLOOP; i++)
	{
		for (int j = 0; j < INNERLOOP; j++)
		{
			wt = rand() * rand() % 10;
		}
	}
}

/* Opens the /dev/simple_driver module
 *
 *  @return int - The file descriptor from the module
 */
int open_simple_driver()
{
	int fd;
	if ((fd = open("/dev/simple_driver", O_RDWR)) == -1)
	{
		perror("open failed");
		exit(-1);
	}
	return fd;
}

// Function that writes message to a file
void write_to_file(int fd, const char *msg)
{
	int bytes_written = write(fd, msg, sizeof(msg));
	printf("%s", msg);
	if (bytes_written != sizeof(msg))
	{
		perror("write failed");
		exit(1);
	}

}
