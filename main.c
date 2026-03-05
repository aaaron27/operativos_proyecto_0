#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct {
    unsigned int speed;
    int isAmbulance; // true-false
    int type;
} Car;

typedef struct {
    int longitudPuente;
    int mediaLLegada1;
    int mediaLLegada2;
    int velocidadPromedio1;
    int velocidadPromedio2;
    int k1;
    int k2;
    int duracionVerde1;
    int duracionVerde2;
    int porcentajeAmbulancias1;
    int porcentajeAmbulancias2;
} Config;

Config readConfigFile() {
    FILE *f = fopen("config.txt", "r");

    Config config;

    if (f == NULL) {
        printf("Error al abrir archivo config.txt");
        return config;
    }

    fscanf(f, "%d", &config.longitudPuente);

    fscanf(f, "%d", &config.mediaLLegada1);
    fscanf(f, "%d", &config.mediaLLegada2);

    fscanf(f, "%d", &config.velocidadPromedio1);
    fscanf(f, "%d", &config.velocidadPromedio2);

    fscanf(f, "%d", &config.k1);
    fscanf(f, "%d", &config.k2);

    fscanf(f, "%d", &config.duracionVerde1);
    fscanf(f, "%d", &config.duracionVerde2);


    fscanf(f, "%d", &config.porcentajeAmbulancias1);
    fscanf(f, "%d", &config.porcentajeAmbulancias2);

    fclose(f);

    return config;
}

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* foo(void* arg) {
    // region critica
    pthread_mutex_lock(&mutex);

    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main() {
    Config config = readConfigFile();

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