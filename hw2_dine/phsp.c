#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

enum distribution {exponential, uniform};
enum states {hungry,thinking, dining};

int number_philop;
int min_think;;
int max_think;
int min_dine;
int max_dine;
int counter;
enum distribution dst;


struct monitor_t{
    pthread_mutex_t *mutx;
    pthread_cond_t *status;
    enum states *state;
} chopsticks;

void monitor_initialize(int num){ // initializing monitor
    for(int i = 0; i < num; i++)
        chopsticks.state[i] = thinking; // they will start thinking here
}



int generate_random_value(int max, int min){
    double think_dine_time = 0;
    if(max == min)
        return max;
  
    if(dst == uniform)
        while(think_dine_time > max || think_dine_time < min){
            double random_number = ((double)rand() / RAND_MAX);
            think_dine_time = max * random_number;
        }
    if(dst == exponential) 
        while(think_dine_time > max || think_dine_time < min){
            double random_value = ((double)rand() / RAND_MAX);
            think_dine_time = (-((double)(min + max)) / 2) * log(1 - random_value);
            // added linker in the makefile since it had undefidend reference for log because of random_value
        }
    else 
        exit(0);
    
    return (int)think_dine_time;

}


void dining_philop(int i){
    if(chopsticks.state[(i + number_philop - 1) % number_philop] != dining)
        if(chopsticks.state[( i + 1 ) % number_philop] != dining)
            if(chopsticks.state[i] == hungry){
                printf(" - Philosopher %d is dining. \n", (i + 1));
                chopsticks.state[i] = dining;
                pthread_cond_signal(&chopsticks.status[i]);
            }
}

int get_chopsticks(int i){
    chopsticks.state[i] = hungry;
    printf(" * Philoshoper %d is hungry. \n", (i + 1));
    clock_t beginning = clock();
    dining_philop(i);
    int lock = -1;
    
    if(lock > 0){
        if(lock == i)
            pthread_mutex_lock(&chopsticks.mutx[(i + 1) % number_philop]); 
        
        else
            pthread_mutex_lock(&chopsticks.mutx[i]);

    }


    if(chopsticks.state[i] != dining){
        if(chopsticks.state[(i + 1) % number_philop] == dining){
            pthread_cond_wait(&chopsticks.status[i], &chopsticks.mutx[(i + 1) % number_philop]);
            lock = (i + 1) % number_philop;
        } else{   
            lock = i;
            pthread_cond_wait(&chopsticks.status[i], &chopsticks.mutx[i]);
        }
    }
    else{ 
        pthread_mutex_lock(&chopsticks.mutx[i]);
        pthread_mutex_lock(&chopsticks.mutx[(i + 1) % number_philop]);
    }
    return (clock() - beginning) * 1000 / CLOCKS_PER_SEC;
}



void put_chopsticks(int philosopher_number){
    chopsticks.state[philosopher_number] = thinking;

    pthread_mutex_unlock(&chopsticks.mutx[philosopher_number]);
    pthread_mutex_unlock(&chopsticks.mutx[(philosopher_number + 1) % number_philop]);

    printf(" ? Philosopher %d is thinking. \n", (philosopher_number + 1));

    dining_philop((philosopher_number + number_philop - 1) % number_philop);
    dining_philop((philosopher_number + 1) % number_philop);

}




void* create_philosopher(void* arg){

    int total_time = 0;
    int id = *((int *)arg);
    int count = 0;

    printf(" -- Philosopher %d was created then started to thinking. \n", (id + 1));

    while(count < counter){
        
        usleep((useconds_t)(generate_random_value(max_think, min_think) * 1000));
        
        total_time += get_chopsticks(id);

        int dining_time = generate_random_value(max_dine, min_dine);
        
        clock_t beginning = clock();
        clock_t difference;
        
        int tme;
        
        while(tme < dining_time){

            difference = clock() - beginning;
            tme = difference * 1000 / CLOCKS_PER_SEC;

        }

        
        put_chopsticks(id);
        count += 1;
    }

    
    printf(" --> Philosopher %d => hungry state = %d\n", (id + 1), total_time);

    pthread_exit(NULL);

}



int main(int argc, char *argv[]){
    number_philop = atoi(argv[1]);

    if(number_philop % 2 == 0 || number_philop > 27){
        printf("The number of philophers cannot be larger than 27 and should be odd number. \n"); 
        exit(0);
    }

    min_think = atoi(argv[2]);
    max_think = atoi(argv[3]);
    min_dine = atoi(argv[4]);
    max_dine = atoi(argv[5]);


    if(strcmp(argv[6], "uniform") == 0)
        dst = uniform;
    
    if(strcmp(argv[6], "exponential") == 0)
        dst = exponential;
    
    else {
        printf("wrong distribution type");
        exit(0);
    }


    if(max_think > 60000){
        printf("Maximum thinking time should not be bigger than  60 seconds. \n");
        exit(0);
    }

    if(max_dine > 60000){
        printf("Maximum dining time should not be bigger than  60 seconds. \n");
        exit(0);
    }

    if(min_think < 1){
        printf("Minimum thinking time should be bigger than 1 ms. \n");
        exit(0);    
    }

    if(min_dine < 1){
        printf("Minimum dining time should be bigger than 1 ms. \n");
        exit(0);
    }


    counter = atoi(argv[7]);



  //https://stackoverflow.com/questions/63819150/c-how-to-initialize-stdmutex-that-has-been-allocated-with-malloc 
    chopsticks.mutx = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t) * number_philop);
    chopsticks.status = (pthread_cond_t *)malloc(sizeof(pthread_cond_t) * number_philop);
    chopsticks.state = (enum states *)malloc(sizeof(enum states) * number_philop);

    monitor_initialize(number_philop);

    for(int i = 0; i < number_philop; i++){
        pthread_mutex_init(&chopsticks.mutx[i], NULL);
        pthread_cond_init(&chopsticks.status[i], NULL);
    }

    pthread_t threads[number_philop]; // create thread as many as num of philophers
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    int ids[number_philop];
    for(int i = 0; i < number_philop; i++)
        ids[i] = i;
    

    for(int i = 0; i < number_philop; i++)
        pthread_create(&threads[i], &attr, create_philosopher, (void *)&ids[i]);
    
    
    for(int i = 0; i < number_philop; i++)
        pthread_join(threads[i], NULL);
    

    
    pthread_attr_destroy(&attr);

    for(int i = 0; i < number_philop; i++){
        pthread_mutex_destroy(&chopsticks.mutx[i]);
        pthread_cond_destroy(&chopsticks.status[i]);
    }


    free(chopsticks.mutx);
    free(chopsticks.status);
    free(chopsticks.state);
    exit(0);
}