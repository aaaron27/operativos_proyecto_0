#include <stdio.h>
#include <pthread.h>

typedef struct {
    unsigned int speed;
    int isAmbulance; // true-false
    int type;
} Car;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* foo(void* arg) {
    // region critica
    pthread_mutex_lock(&mutex);

    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main() {
    // inicializacion de hilos
    pthread_t t1;

    // creacion del hilo
    pthread_create(&t1, NULL, foo, NULL);

    // sincronizacion del hilo con el main
    pthread_join(t1, NULL);

    // destruir el mutex
    pthread_mutex_destroy(&mutex);
    
    return 0;
}