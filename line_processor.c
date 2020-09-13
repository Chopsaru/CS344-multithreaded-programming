#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define SIZE        1000
#define LINE_SIZE   80

char buffer1[SIZE];
char buffer2[SIZE];
char buffer3[SIZE];
char outBuff[LINE_SIZE];
int doneFlag = 0;
int outputIndex = 0;

void *getInput();
void *seperateLine();
void *replaceSign();
void *printOutput();
int isEmpty(char*);

// Initialize the mutex
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex3 = PTHREAD_MUTEX_INITIALIZER;

// Initialize the condition variables
pthread_cond_t full1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t full2 = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty2 = PTHREAD_COND_INITIALIZER;
pthread_cond_t full3 = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty3 = PTHREAD_COND_INITIALIZER;

//returns 1 if passed buffer is empty, returns 0 otherwise.
int isEmpty(char buffer[]){
    if (buffer[0] == '\0'){
        return 1;
    }
    return 0;
}

void *getInput(void *args){
    while(doneFlag != 1){
        // Lock mutex1 before checking where there is space in buffer1
        pthread_mutex_lock(&mutex1);
        while (isEmpty(buffer1) != 1){
            // Buffer1 is full. Wait for the consumer to signal that buffer1 has space
            pthread_cond_wait(&empty1, &mutex1);
        }
        //get input from stdin
        fgets(buffer1, SIZE, stdin);

        if (strcmp(buffer1, "DONE\n") == 0){
            doneFlag = 1;
        }
        // Signal to the consumer that buffer1 is no longer empty
        pthread_cond_signal(&full1);
        // Unlock the mutex
        pthread_mutex_unlock(&mutex1);
    }
    return NULL;
}

void *seperateLine(void *args){
    while(doneFlag != 1){
        // Lock  mutex1 before checking if buffer1 has data      
        pthread_mutex_lock(&mutex1);
        while (isEmpty(buffer1)){
            // Buffer1 is empty. Wait for the producer to signal that the buffer has data
            pthread_cond_wait(&full1, &mutex1);
        }

        //iterate thru buffer one and replace instances of \n with space
        int i;
        for (i=0; i<=SIZE; i++){
            if (buffer1[i] == '\0'){
                break;
            }
            if (buffer1[i] == '\n'){
                buffer1[i] = ' ';
            }
        }

        // Lock mutex2 before checking where there is space in buffer2
        pthread_mutex_lock(&mutex2);
        while (isEmpty(buffer2) != 1){
            // Buffer2 is full. Wait for the consumer to signal that buffer2 has space
            pthread_cond_wait(&empty2, &mutex2);
        }
        strcpy(buffer2, buffer1);
        // Signal to the consumer that  buffer2 is no longer empty
        pthread_cond_signal(&full2);
        // Unlock mutex2
        pthread_mutex_unlock(&mutex2);

        //empty buffer 1
        memset(buffer1, '\0', SIZE);
        // Signal to the producer that buffer1 has space
        pthread_cond_signal(&empty1);
        // Unlock the buffer1 mutex
        pthread_mutex_unlock(&mutex1);
    }
    return NULL;
}

void *replaceSign(void *args){
     while(doneFlag != 1){
        // Lock  mutex2 before checking if buffer1 has data      
        pthread_mutex_lock(&mutex2);
        while (isEmpty(buffer2)){
            // Buffer2 is empty. Wait for the producer to signal that buffer2 has data
            pthread_cond_wait(&full2, &mutex2);
        }
        //copy buffer 2 to a temp string so we can release it sooner
        char * temp = strdup(buffer2);
        //empty buffer 2
        memset(buffer2, '\0', SIZE);
        // Signal to the producer that buffer2 has space
        pthread_cond_signal(&empty2);
        // Unlock the buffer2 mutex
        pthread_mutex_unlock(&mutex2);

        // Lock mutex3 before checking where there is space in buffer3
        pthread_mutex_lock(&mutex3);
        while (isEmpty(buffer3) != 1){
            // Buffer3 is full. Wait for the consumer to signal that buffer3 has space
            pthread_cond_wait(&empty3, &mutex3);
        }
        //copy each character over from temp to buffer 3 iteratively.
        //replace all instances of ++ with ^
        int i, j;
        for (i=0, j=0; i<=SIZE; i++, j++){
            if (temp[i] == '\0'){
                break;
            }
            if (temp[i] == '+' && temp[i+1] == '+'){
                buffer3[j] = '^';
                i++;
            }
            else{
                buffer3[j] = temp[i];
            }
        }
        free(temp);
        // Signal to the consumer that  buffer3 is no longer empty
        pthread_cond_signal(&full3);
        // Unlock mutex3
        pthread_mutex_unlock(&mutex3);
     }
     return NULL;
}

void *printOutput(void *args){
    while(doneFlag != 1){
        // Lock  mutex3 before checking if  buffer3 has data      
        pthread_mutex_lock(&mutex3);
        while (isEmpty(buffer3)){
            // Buffer is empty. Wait for the producer to signal that the buffer has data
            pthread_cond_wait(&full3, &mutex3);
        }
        //iterate thru buffer 3 and pass each char to outbuff. if outbuff has 80 chars, print contents and reset
        int i;
        for (i=0; i<=SIZE; i++, outputIndex++){
            outBuff[outputIndex] = buffer3[i];
            if (outputIndex == 79){
                printf("%s\n", outBuff);
                memset(outBuff, '\0', LINE_SIZE);
                outputIndex = -1;
            }
            if (buffer3[i] == '\0'){
                break;
            }
        }  
        memset(buffer3, '\0', SIZE);
        // Signal to the producer that the buffer has space
        pthread_cond_signal(&empty3);
        // Unlock the mutex
        pthread_mutex_unlock(&mutex3);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    memset(buffer1, '\0', SIZE);
    memset(buffer2, '\0', SIZE);
    memset(buffer3, '\0', SIZE);

    // Create the producer and consumer threads
    pthread_t in_t, line_t, plus_t, out_t;
    pthread_create(&in_t, NULL, getInput, NULL);
    pthread_create(&line_t, NULL, seperateLine, NULL);
    pthread_create(&plus_t, NULL, replaceSign, NULL);
    pthread_create(&out_t, NULL, printOutput, NULL);
    
    //join all threads after doneflag is raised
    pthread_join(in_t, NULL);
    pthread_join(in_t, NULL);
    pthread_join(plus_t, NULL);
    pthread_join(out_t, NULL);
    return 0;
}