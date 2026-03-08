#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#define M_PI 3.14159265358979323846
// ========================================================================================
//     GLOBALLLLLLLLLLLLLLLLLLLLLL
// ========================================================================================
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

int carrosEnPuenteLado1 = 0;
int carrosEnPuenteLado2 = 0;
int ambulanciasEsperando1 = 0;
int ambulanciasEsperando2 = 0;
int* puenteVisual;
pthread_mutex_t mutexGrafico = PTHREAD_MUTEX_INITIALIZER;
int simulacionActiva = 1;

typedef struct {
    unsigned int speed;
    int isAmbulance; // true-false
    int type;
} Car;


void readConfigFile() {
    FILE *f = fopen("C:/Users/faken/Desktop/code/c/proyecto0SO/operativos_proyecto_0/config.txt", "r");

    if (f == NULL) {
        printf("Error al abrir archivo config.txt\n");
        exit(1);
    }

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

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* foo(void* arg) {
    // region critica
    pthread_mutex_lock(&mutex);

    pthread_mutex_unlock(&mutex);

    return NULL;
}


// ============================================================================================================================
//     MODO CARNAGE // FIFOOOOO
// ============================================================================================================================

pthread_mutex_t mutexPuente = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condLado1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t condLado2 = PTHREAD_COND_INITIALIZER;

void entrarPuente(Car* miCarro) {
    pthread_mutex_lock(&mutexPuente);

    if (miCarro->type == 1) { // --- LADO 1 ---
        if (miCarro->isAmbulance) ambulanciasEsperando1++;
        while (carrosEnPuenteLado2 > 0 ||
              (!miCarro->isAmbulance && (ambulanciasEsperando1 > 0 || ambulanciasEsperando2 > 0))) {
            pthread_cond_wait(&condLado1, &mutexPuente);
        }

        if (miCarro->isAmbulance) ambulanciasEsperando1--;
        carrosEnPuenteLado1++;

        printf("[Lado 1] ENTRA. (%s) | Vel: %d\n",
               miCarro->isAmbulance ? "AMBULANCIA" : "Normal", miCarro->speed);

    } else { // --- LADO 2 ---
        if (miCarro->isAmbulance) ambulanciasEsperando2++;

        while (carrosEnPuenteLado1 > 0 ||
              (!miCarro->isAmbulance && (ambulanciasEsperando1 > 0 || ambulanciasEsperando2 > 0)) ||
              (miCarro->isAmbulance && ambulanciasEsperando1 > 0)) {
            pthread_cond_wait(&condLado2, &mutexPuente);
        }

        if (miCarro->isAmbulance) ambulanciasEsperando2--;
        carrosEnPuenteLado2++;

        printf("[Lado 2] ENTRA. (%s) | Vel: %d\n",
               miCarro->isAmbulance ? "AMBULANCIA" : "Normal", miCarro->speed);
    }

    pthread_mutex_unlock(&mutexPuente);
}

void salirPuente(Car* miCarro) {
    pthread_mutex_lock(&mutexPuente);

    if (miCarro->type == 1) {
        carrosEnPuenteLado1--;
        printf("[Lado 1] SALE. (%s)\n", miCarro->isAmbulance ? "AMBULANCIA" : "Normal");
    } else {
        carrosEnPuenteLado2--;
        printf("[Lado 2] SALE. (%s)\n", miCarro->isAmbulance ? "AMBULANCIA" : "Normal");
    }

    if (carrosEnPuenteLado1 == 0 && carrosEnPuenteLado2 == 0) {
        pthread_cond_broadcast(&condLado1);
        pthread_cond_broadcast(&condLado2);
    }

    pthread_mutex_unlock(&mutexPuente);
}

void* rutinaCarro(void* arg) {
    Car* miCarro = (Car*)arg;

    entrarPuente(miCarro);

    int tiempoCruce = longitudPuente / miCarro->speed;
    if (tiempoCruce <= 0) tiempoCruce = 1;

    int microSegundosPorPaso = (tiempoCruce * 1000000) / longitudPuente;

    for (int paso = 0; paso < longitudPuente; paso++) {
        int posicion = (miCarro->type == 1) ? paso : (longitudPuente - 1 - paso);

        pthread_mutex_lock(&mutexGrafico);
        puenteVisual[posicion] = miCarro->isAmbulance ? (miCarro->type == 1 ? 3 : 4) : miCarro->type;
        pthread_mutex_unlock(&mutexGrafico);

        usleep(microSegundosPorPaso);

        pthread_mutex_lock(&mutexGrafico);
        puenteVisual[posicion] = 0;
        pthread_mutex_unlock(&mutexGrafico);
    }

    salirPuente(miCarro);

    free(miCarro);
    return NULL;
}

void* generadorLado1(void* arg) {
    int tiempoSimulacion = *(int*)arg;
    int totalCarros = cantidadCarrosEnTiempoX(tiempoSimulacion, mediaLLegada1);
    printf("[Lado 1] Planificados %d vehiculos para los proximos %d segs.\n", totalCarros, tiempoSimulacion);

    if (totalCarros > 0) {
        int tiempos[totalCarros];
        generarTiemposAbsolutos(totalCarros, mediaLLegada1, tiempos);

        int tiempoActual = 0;

        for (int i = 0; i < totalCarros; i++) {
            int espera = tiempos[i] - tiempoActual;

            if (espera > 0) {
                sleep(espera);
                tiempoActual += espera;
            }

            // --- NACE EL CARRO ---
            Car* nuevoCarro = (Car*)malloc(sizeof(Car));
            nuevoCarro->speed = generarVelocidad(velocidadPromedio1, minVelocidad1, maxVelocidad1);
            nuevoCarro->isAmbulance = esAmbulancia(porcentajeAmbulancias);
            nuevoCarro->type = 1;

            pthread_t hiloCarro;
            pthread_create(&hiloCarro, NULL, rutinaCarro, (void*)nuevoCarro);
            pthread_detach(hiloCarro);
        }
    }

    printf("[Lado 1] El generador termino su turno.\n");
    return NULL;
}

void* generadorLado2(void* arg) {
    int tiempoSimulacion = *(int*)arg;
    int totalCarros = cantidadCarrosEnTiempoX(tiempoSimulacion, mediaLLegada2);
    printf("[Lado 2] Planificados %d vehiculos para los proximos %d segs.\n", totalCarros, tiempoSimulacion);

    if (totalCarros > 0) {
        int tiempos[totalCarros];
        generarTiemposAbsolutos(totalCarros, mediaLLegada2, tiempos);

        int tiempoActual = 0;

        for (int i = 0; i < totalCarros; i++) {
            int espera = tiempos[i] - tiempoActual;

            if (espera > 0) {
                sleep(espera);
                tiempoActual += espera;
            }

            Car* nuevoCarro = (Car*)malloc(sizeof(Car));
            nuevoCarro->speed = generarVelocidad(velocidadPromedio2, minVelocidad2, maxVelocidad2);
            nuevoCarro->isAmbulance = esAmbulancia(porcentajeAmbulancias);
            nuevoCarro->type = 2;

            pthread_t hiloCarro;
            pthread_create(&hiloCarro, NULL, rutinaCarro, (void*)nuevoCarro);
            pthread_detach(hiloCarro);
        }
    }

    printf("[Lado 2] El generador termino su turno.\n");
    return NULL;
}
void* hiloDibujante(void* arg) {
    while (simulacionActiva) {
        printf("\033[H\033[J");

        printf("=============================================================\n");
        printf("                  SIMULADOR DE PUENTE ESTRECHO                \n");
        printf("=============================================================\n\n");

        printf("  [ LADO 1 (OESTE) ]                      [ LADO 2 (ESTE) ]\n");
        printf("  Fila Normal: %d                         Fila Normal: %d\n",
               (carrosEnPuenteLado1 > 0 ? 0 : 0), (carrosEnPuenteLado2 > 0 ? 0 : 0));
        printf("    Ambulancias: %d                         Ambulancias: %d\n",
               ambulanciasEsperando1, ambulanciasEsperando2);
        printf("-------------------------------------------------------------\n\n");

        printf("         \033[0;37m");
        for (int i = 0; i < longitudPuente + 4; i++) printf("=");
        printf("\033[0m\n");

        printf("       | \033[100m");

        pthread_mutex_lock(&mutexGrafico);
        for (int i = 0; i < longitudPuente; i++) {
            switch(puenteVisual[i]) {
                case 0: printf("\033[100;37m ░ \033[0m\033[100m"); break;
                case 1: printf("\033[44;1;37m > \033[0m\033[100m"); break;
                case 2: printf("\033[42;1;37m < \033[0m\033[100m"); break;
                case 3: printf("\033[41;1;33mA >\033[0m\033[100m"); break;
                case 4: printf("\033[41;1;33m< A\033[0m\033[100m"); break;
            }
        }
        pthread_mutex_unlock(&mutexGrafico);

        printf("\033[0m |\n");

        printf("         \033[0;37m");
        for (int i = 0; i < longitudPuente + 4; i++) printf("=");
        printf("\033[0m\n");
        printf("           \033[0;36m≈ ≈ ≈ ≈ ≈ ≈ ≈ ≈ ≈ ≈ ≈ ≈ ≈ ≈ ≈ ≈ ≈ ≈ ≈ ≈\033[0m\n\n");

        usleep(166666);
    }
    return NULL;
}



int main() {
    srand(time(NULL));

    readConfigFile();
    puenteVisual = (int*)calloc(longitudPuente, sizeof(int));
    printf("Cuanto va a durar la simulacion?");
    int tiempoSim;
    scanf("%d", &tiempoSim);
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
            printf("Iniciando modalidad Carnage (FIFO)...\n\n");
            simulacionActiva = 1;
            pthread_t dibujante;
            pthread_create(&dibujante, NULL, hiloDibujante, NULL);
            pthread_t gen1, gen2;
            pthread_create(&gen1, NULL, generadorLado1, &tiempoSim);
            pthread_create(&gen2, NULL, generadorLado2, &tiempoSim);

            pthread_join(gen1, NULL);
            pthread_join(gen2, NULL);

            sleep(10);
            printf("Simulacion finalizada.\n");

            simulacionActiva = 0;
            pthread_join(dibujante, NULL);
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

    free(puenteVisual);
    return 0;
}