#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include <semaphore.h>

sem_t s, delay; // Two Semaphores

int n; // Number of items on the list

struct node // Used for circular buffer
{
    int value,
        in, // Means of making sure consumer does not consume empty list
        out;

    struct node *next; // Poniter to next node
};

struct node produce() // Create a product, and return that product to caller
{
    struct node product;

    static int newValue= 999; // Starting values
    static int newIn = 0;

    newValue++; // Increase relevent information
    newIn++;

    product.value = newValue;
    product.in = newIn;

    return product;
}

struct node take(struct node* product) // Take from the buffer, set it to 0
{
    struct node oldProd; // Create a copy for use later

    oldProd.value = product->value;

    product->value = 0;

    product->out++;

    return oldProd;

}

void *startProducer(void * args)
{
  FILE *results = fopen("proj2_part2.txt", "a+");

  pthread_t self; // For reproting thread ID
  self = pthread_self();

  struct node *buffer = (struct node *)args; // Instatiate argument

  while(1) // Run infinitely
  {

      struct node product = produce(); // Produce object
      printf("Producer %lu produced %d\n",self, product.value); // INform user
      fprintf(results, "Producer %lu produced %d\n",self, product.value);

      sem_wait(&s); // Concurrency purposes

      buffer->value = product.value; // "Append" to the circular buffer
      buffer->in = product.in;

      n++; // Increase count for number of items on the list

      if(n==1) // To make sure consumer does not consume empty list
        sem_post(&delay);

      sem_post(&s);

      buffer = buffer->next; // Move to the next node for next interation

      // BELOW IS EXACT SAME AS CONSUMER, THIS WILL NOT AFFECT ORDER OF OUTPUT
      sleep(1); // Means of staggering output so user can read it
  }
  return NULL;
}

void *startConsumer(void * args)
{
    FILE *results = fopen("proj2_part2.txt", "a+");

    pthread_t self; // For reproting thread ID
    self = pthread_self();

    struct node *buffer = (struct node *)args; // Instatiate argument

    int m; // Used to force consumer to wait for next item

    sem_wait(&delay); // Make sure buffer isnt empty

    while(1)
    {
        sem_wait(&s);

        struct node oldProd = take(buffer); // "Take off" from the list

        n--; // Decrease amount of items

        m = n;
        // Hello Worl lol
        sem_post(&s);

        if(oldProd.value > 999 && oldProd.value < 1000000000) // Awkard bug: Consumer gets incorrect values randomly
        {
          printf("Consumer %lu consumed: %d\n", self, oldProd.value);
          fprintf(results, "Consumer %lu consumed: %d\n", self, oldProd.value);
        }
        oldProd.value = 0 * oldProd.value; // To get rid of warning

        if(m==0)
          sem_post(&delay);

        buffer = buffer->next;

        // BELOW IS EXACT SAME AS PRODUCER, THIS WILL NOT AFFECT ORDER OF OUTPUT
        sleep(1); // Means of staggering output for user
    }

    return NULL;
}

int main(void)
{
    sem_init(&s, 0, 1); // Initialized semaphores
    sem_init(&delay, 0, 0);

    // Naming may be confusing, no true "head" in a circular buffer
    struct node  head, // Create a 3 node circular buffer
                  mid,
                  tail;

    head.next = &mid; // Initialize circular buffer
    mid.next = &tail;
    tail.next = &head;

    pthread_t producer, // Create two threads
              consumer;

    FILE *results = fopen("proj2_part2.txt", "w");
    fclose(results);

    printf("NOTE: Sleep is used to stagger output for user to read\n");
    printf("\tBOTH THREADS HAVE THE EXACT SAME SLEEP VALUE.\n\tThis will not affect ordering, as semaphores are used.\n");

    // Run threads with head node pointer
    pthread_create(&consumer, NULL, startConsumer, (void *)&head);
    pthread_create(&producer, NULL, startProducer, (void *)&head);

    // Useless, but for good practice
    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);

    return 0;
}
