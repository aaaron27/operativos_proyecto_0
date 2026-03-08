#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#define M_PI 3.14159265358979323846

int longitudPuente;

int mediaLLegada1;
int mediaLLegada2;

int velocidadPromedio1;
int velocidadPromedio2;

int minVelocidad1;
int minVelocidad2;

int maxVelocidad1;
int maxVelocidad2;

int k1;
int k2;

int duracionVerde1;
int duracionVerde2;

int porcentajeAmbulancias;

typedef struct {
    unsigned int speed;
    int isAmbulance; // true-false
    int type;
} Car;


void readConfigFile() {
    FILE *f = fopen("C:/Users/faken/Desktop/code/c/proyecto0SO/operativos_proyecto_0/config.txt", "r");

    if (f == NULL) {
        printf("Error al abrir archivo config.txt\n");
        // Salir del programa si no hay configuración, ya que sin ella nada funcionará
        exit(1);
    }

    // Leemos y guardamos directamente en las variables globales
    fscanf(f, "%d", &longitudPuente);

    fscanf(f, "%d", &mediaLLegada1);
    fscanf(f, "%d", &mediaLLegada2);

    fscanf(f, "%d", &velocidadPromedio1);
    fscanf(f, "%d", &velocidadPromedio2);

    fscanf(f, "%d", &minVelocidad1);
    fscanf(f, "%d", &minVelocidad2);

    fscanf(f, "%d", &maxVelocidad1);
    fscanf(f, "%d", &maxVelocidad2);

    fscanf(f, "%d", &k1);
    fscanf(f, "%d", &k2);

    fscanf(f, "%d", &duracionVerde1);
    fscanf(f, "%d", &duracionVerde2);

    fscanf(f, "%d", &porcentajeAmbulancias);

    fclose(f);
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

int generarVelocidad(int media, int min, int max) {
    double u1 = (double)rand() / RAND_MAX;
    double u2 = (double)rand() / RAND_MAX;
    if (u1 <= 0.0) u1 = 0.0000001;
    double z0 = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
    double desviacion = (max - min) / 6.0;
    int velocidad = (int)round(media + (z0 * desviacion));
    if (velocidad < min) velocidad = min;
    if (velocidad > max) velocidad = max;

    return velocidad;
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

            break;
        case 2:

            break;
        case 3:

            break;
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
    for(int i=0; i<1000; i++) {
        int x = generarVelocidad(config.velocidadPromedio1, config.minVelocidad1, config.maxVelocidad1);
        printf("%d",x);
        printf("\n");
    }

    return 0;
}