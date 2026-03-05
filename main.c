#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
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
    int porcentajeAmbulancias;
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


    fscanf(f, "%d", &config.porcentajeAmbulancias);

    fclose(f);

    return config;
}

int tiempoLlegada(int media) {
    double u = (double)rand() / RAND_MAX;
    double res = -log(1.0 - u) * media;
    return (int)res; 
}

int esAmbulancia(int porcentaje) {
    double u = (double)rand() / RAND_MAX;
    if (u <= (double)porcentaje/100) return 1;
    return 0;
}

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* foo(void* arg) {
    // region critica
    pthread_mutex_lock(&mutex);

    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main() {
    srand(time(NULL));

    Config config = readConfigFile();

    int modalidad;
    while (1) {
        printf("Modalidad\n");
        printf("\t1. Modo carnage\n");
        printf("\t2. Semaforos\n");
        printf("\t3. Oficiales de transito\n");

        scanf("%d", &modalidad);

        if (modalidad < 1 || modalidad > 3) {
            printf("\nEres mongolo?\n");
        } else {
            break;
        }
    }

    // // inicializacion de hilos
    // pthread_t t1;

    // // creacion del hilo
    // pthread_create(&t1, NULL, foo, NULL);

    // // sincronizacion del hilo con el main
    // pthread_join(t1, NULL);

    // // destruir el mutex
    // pthread_mutex_destroy(&mutex);
    
    return 0;
}