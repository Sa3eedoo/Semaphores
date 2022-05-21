#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#define no_of_counters 5
#define size_of_buffer 1

sem_t s1, s2, n, e;

int no_of_messages = 0;

// for coloring
void print_blue()
{
    printf("\033[0;34m");
}

void print_green()
{
    printf("\033[0;32m");
}

void print_red()
{
    printf("\033[1;31m");
}
//------------------------------------------------------------------------------

// Queue implementation
struct Queue
{
    int front, rear, size;
    unsigned capacity;
    int *array;
};

struct Queue *createQueue(unsigned capacity)
{
    struct Queue *queue = (struct Queue *)malloc(
        sizeof(struct Queue));
    queue->capacity = capacity;
    queue->front = queue->size = 0;

    queue->rear = capacity - 1;
    queue->array = (int *)malloc(
        queue->capacity * sizeof(int));
    return queue;
}

int isFull(struct Queue *queue)
{
    return (queue->size == queue->capacity);
}

int isEmpty(struct Queue *queue)
{
    return (queue->size == 0);
}

void enqueue(struct Queue *queue, int item)
{
    if (isFull(queue))
        return;
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->array[queue->rear] = item;
    queue->size = queue->size + 1;
}

int dequeue(struct Queue *queue)
{
    if (isEmpty(queue))
        return -1;
    int item = queue->array[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    queue->size = queue->size - 1;
    return item;
}

int front(struct Queue *queue)
{
    if (isEmpty(queue))
        return 0;
    return queue->front;
}

int rear(struct Queue *queue)
{
    if (isEmpty(queue))
        return 0;
    return queue->rear;
}

struct Queue *buffer;
//------------------------------------------------------------------------------

void *counter(void *arg);

void *monitor(void *arg);

void *collector(void *arg);

int main()
{
    buffer = createQueue(size_of_buffer);

    sem_init(&s1, 0, 1);
    sem_init(&s2, 0, 1);
    sem_init(&n, 0, 0);
    sem_init(&e, 0, size_of_buffer);

    pthread_t mCounter[no_of_counters], mMonitor, mCollector;

    int args[no_of_counters];

    for (int i = 0; i < no_of_counters; i++)
    {
        args[i] = i + 1;
        pthread_create(&mCounter[i], NULL, &counter, &args[i]);
    }
    pthread_create(&mMonitor, NULL, &monitor, NULL);
    pthread_create(&mCollector, NULL, &collector, NULL);

    for (int i = 0; i < no_of_counters; i++)
    {
        pthread_join(mCounter[i], NULL);
    }
    pthread_join(mMonitor, NULL);
    pthread_join(mCollector, NULL);
}

void *counter(void *data)
{
    int *current_thread = data;
    int tid = *current_thread;
    while (1)
    {
        int seconds = (rand() % 3) + 1;
        sleep(seconds);
        print_blue();
        printf("Counter thread %d: received a message\n", tid);
        printf("Counter thread %d: waiting to write\n", tid);
        sem_wait(&s1);
        // critical section
        no_of_messages++;
        print_blue();
        printf("Counter thread %d: now adding to counter, counter value=%d\n", tid, no_of_messages);
        //-------------------------------------------------------------------------------------------
        sem_post(&s1);
    }
}

void *monitor(void *data)
{
    int value;
    int buffer_state;
    int index;
    while (1)
    {
        int seconds = (rand() % 6) + 4;
        sleep(seconds);

        print_red();
        printf("Monitor thread: waiting to read counter\n");
        sem_wait(&s1);
        // critical section
        value = no_of_messages;
        no_of_messages = 0;
        print_red();
        printf("Monitor thread: reading a count value of %d\n", value);
        //---------------------------------------------------------------
        sem_post(&s1);

        sem_getvalue(&e, &buffer_state);
        if (buffer_state == 0)
        {
            print_red();
            printf("Monitor thread: Buffer full!!\n");
        }
        sem_wait(&e);
        sem_wait(&s2);
        // critical section
        index = rear(buffer) + 1;
        enqueue(buffer, value);
        print_red();
        printf("Monitor thread: writing to buffer at position %d, value = %d\n", index, value);
        //--------------------------------------------------------------------------------------
        sem_post(&s2);
        sem_post(&n);
    }
}

void *collector(void *data)
{
    int value;
    int buffer_state;
    int index;
    while (1)
    {
        int seconds = (rand() % 6) + 4;
        sleep(seconds);

        sem_getvalue(&e, &buffer_state);
        if (buffer_state == size_of_buffer)
        {
            print_green();
            printf("Collector thread: nothing is in the buffer!\n");
        }
        sem_wait(&n);
        sem_wait(&s2);
        // critical section
        index = front(buffer) + 1;
        value = dequeue(buffer);
        print_green();
        printf("Collector thread: reading from the buffer at position %d, value = %d\n", index, value);
        //-----------------------------------------------------------------------------------------------
        sem_post(&s2);
        sem_post(&e);
    }
}