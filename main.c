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

    printf("      SIMULADOR DE PUENTE ESTRECHO - INICIO       \n");
    printf("Seleccione la modalidad de administracion:\n");
    printf("1. Funcionamiento Carnage (FIFO)\n");
    printf("2. Funcionamiento Semaforos\n");
    printf("3. Funcionamiento Oficial de Transito\n");
    printf("Ingrese el numero de su eleccion (1-3): ");
    
    int modoelec;
    scanf("%d", &modoelec);

    switch (modoelec) {
        case 1:

        case 2:


        case 3:


        default:
            printf("\n[ERROR] Modalidad no reconocida (%d). Debe ser 1, 2 o 3.\n");
            return 0;
    }
    // inicializacion de hilos

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