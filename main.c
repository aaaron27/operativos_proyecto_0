#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
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

int semaforo = 0;

typedef struct {
    int id;
    int speed;
    int isAmbulance; // true-false
    int horaLlegada;
} Car;


void readConfigFile() {
    FILE *f = fopen("config.txt", "r");

    if (f == NULL) {
        printf("Error al abrir archivo config.txt\n");
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

int cantidadCarrosEnTiempoX(int tiempoX, int mediaLlegada) {
    if (mediaLlegada <= 0) return 0;

    double lambda = (double)tiempoX / mediaLlegada;
    double limite = exp(-lambda);
    double multiplicador = 1.0;
    int cantidadCarros = 0;

    do {
        cantidadCarros++;
        double u = (double)rand() / RAND_MAX;
        multiplicador *= u;
    } while (multiplicador > limite);

    return cantidadCarros - 1;
}


void generarTiemposAbsolutos(int cantidadCarros, int mediaLlegada, int tiemposAbsolutos[]) {
    if (cantidadCarros <= 0) return;

    int tiempoAcumulado = 0;
    for (int i = 0; i < cantidadCarros; i++) {
        int tiempoEspera = tiempoLlegada(mediaLlegada);
        tiempoAcumulado += tiempoEspera;
        tiemposAbsolutos[i] = tiempoAcumulado;
    }
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
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void* semaforoControlador(void* arg) {
    // while true
    while(1) {
        pthread_mutex_lock(&mutex);
        semaforo = 0;
        printf("Semaforo Izquierdo en verde por %d segundos\n", duracionVerde1);
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mutex);
        sleep(duracionVerde1);

        pthread_mutex_lock(&mutex);
        semaforo = 1;
        printf("Semaforo Derecho en verde por %d segundos\n", duracionVerde2);
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mutex);
        sleep(duracionVerde2);
    }

    return NULL;
}

void* ladoIzquierdo(void* arg) {
    Car carro = *(Car*)arg;
    unsigned long id = carro.id;

    // region critica
    pthread_mutex_lock(&mutex);

    while (semaforo) {
        printf("Carro %lu lado izq esperando a la hora %d\n", id, carro.horaLlegada);
        pthread_cond_wait(&cond, &mutex);
    }

    printf("Carro %lu lado izq cruzando\n", id);
    pthread_mutex_unlock(&mutex);   

    sleep(longitudPuente / carro.speed);
    printf("Carro %lu lado izq finalizo\n", id);

    return NULL;
}

void* ladoDerecho(void* arg) {
    Car carro = *(Car*)arg;
    unsigned long id = carro.id;

    // region critica
    pthread_mutex_lock(&mutex);

    while (semaforo != 1) {
        printf("Carro %lu lado der esperando a la hora %d\n", id, carro.horaLlegada);
        pthread_cond_wait(&cond, &mutex);
    }

    printf("Carro %lu lado der cruzando\n", id);
    pthread_mutex_unlock(&mutex);   

    sleep(longitudPuente / carro.speed);
    printf("Carro %lu lado der finalizo\n", id);

    return NULL;
}

void semaforoModo(int seconds) {
    int nIzq = cantidadCarrosEnTiempoX(seconds, mediaLLegada1);
    int nDer = cantidadCarrosEnTiempoX(seconds, mediaLLegada2);

    pthread_t controlador;
    pthread_t carroIzq_t[nIzq];
    pthread_t carroDer_t[nDer];

    int llegadaIzq[nIzq];
    int llegadaDer[nDer];

    generarTiemposAbsolutos(nIzq, mediaLLegada1, llegadaIzq);
    generarTiemposAbsolutos(nDer, mediaLLegada2, llegadaDer);

    Car carroIzq[nIzq];
    Car carroDer[nDer];

    pthread_create(&controlador, NULL, semaforoControlador, NULL);

    for (int i = 0; i < nIzq; i++) {
        carroIzq[i].speed = generarVelocidad(velocidadPromedio1, minVelocidad1, maxVelocidad1);
        carroIzq[i].isAmbulance = esAmbulancia(porcentajeAmbulancias);
        carroIzq[i].horaLlegada = llegadaIzq[i];
        carroIzq[i].id = i;

        pthread_create(&carroIzq_t[i], NULL, ladoIzquierdo, &carroIzq[i]);
    }

    for (int i = 0; i < nDer; i++) {
        carroDer[i].speed = generarVelocidad(velocidadPromedio1, minVelocidad1, maxVelocidad1);
        carroDer[i].isAmbulance = esAmbulancia(porcentajeAmbulancias);
        carroDer[i].horaLlegada = llegadaDer[i];
        carroDer[i].id = i;

        pthread_create(&carroDer_t[i], NULL, ladoDerecho, &carroDer[i]);
    }

    for (int i = 0; i < nIzq; i++) {
        pthread_join(carroIzq_t[i], NULL);
    }

    for (int i = 0; i < nDer; i++) {
        pthread_join(carroDer_t[i], NULL);
    }
}

int main() {
    srand(time(NULL));

    readConfigFile();

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
            semaforoModo(180);
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

    // destruir el mutex
    pthread_mutex_destroy(&mutex);

    return 0;
}