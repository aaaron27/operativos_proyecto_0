#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>

#define M_PI 3.14159265358979323846
#define max(a,b) ((a) > (b) ? (a) : (b))

// ========================================================================================
//     GLOBALLLLLLLLLLLLLLLLLLLLLL
// ========================================================================================
int bridgeWeight;

int averageArrivalTimeLeft;
int averageArrivalTimeRight;

int averageSpeedLeft;
int averageSpeedRight;

int minSpeedLeft;
int minSpeedRight;

int maxSpeedLeft;
int maxSpeedRight;

int k1;
int k2;

int greenDurationLeft;
int greenDurationRight;

int ambulancesPercentage;

int carsInBridgeLeft = 0;
int carsInBridgeRight = 0;
int ambulancesWaitingLeft = 0;
int ambulancesWaitingRight = 0;

int* visualBridge;
int activeSimulation = 1;
pthread_mutex_t graphicMutex = PTHREAD_MUTEX_INITIALIZER;
int carrosEnPuenteLado1 = 0;
int carrosEnPuenteLado2 = 0;
int ambulanciasEsperando1 = 0;
int ambulanciasEsperando2 = 0;
int* puenteVisual;
pthread_mutex_t mutexGrafico = PTHREAD_MUTEX_INITIALIZER;
int simulacionActiva = 1;
typedef struct {
    int id;
    int speed;
    int isAmbulance; // true-false
    int arrivalTime;
    int type;
} Car;

pthread_mutex_t trafficLightMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t trafficLightCond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t *mutexs;
pthread_cond_t *conds;

int carInBridge = 0;
int bridgeDirection = -1;
pthread_cond_t condCounter = PTHREAD_COND_INITIALIZER;

int ambulancesLeft = 0;
int ambulancesRight = 0;
pthread_cond_t condAmbulance = PTHREAD_COND_INITIALIZER;

void mutexsInit() {
    mutexs = malloc(bridgeWeight * sizeof(pthread_mutex_t));
    conds = malloc(bridgeWeight * sizeof(pthread_cond_t));
    for (int i = 0; i < bridgeWeight; i++) {
        pthread_mutex_init(&mutexs[i], NULL);
        pthread_cond_init(&conds[i], NULL);
    }
}

void destroyMutexsAndConds() {
    pthread_mutex_destroy(&trafficLightMutex);

    pthread_cond_destroy(&condCounter);
    pthread_cond_destroy(&trafficLightCond);
    pthread_cond_destroy(&condAmbulance);

    for (int i = 0; i < bridgeWeight; i++) {
        pthread_mutex_destroy(&mutexs[i]);
        pthread_cond_destroy(&conds[i]);
    }

    free(mutexs);
    free(conds);

    pthread_mutex_destroy(&mutexGrafico);
    pthread_mutex_destroy(&mutexPuente);
    pthread_cond_destroy(&condLado1);
    pthread_cond_destroy(&condLado2);
    pthread_cond_destroy(&condAmbulancia);
}

void readConfigFile() {
    FILE *f = fopen("config.txt", "r");

    if (f == NULL) {
        printf("Error while opening config.txt\n");
        exit(1);
    }

    fscanf(f, "%d", &bridgeWeight);

    fscanf(f, "%d", &averageArrivalTimeLeft);
    fscanf(f, "%d", &averageArrivalTimeRight);

    fscanf(f, "%d", &averageSpeedLeft);
    fscanf(f, "%d", &averageSpeedRight);

    fscanf(f, "%d", &minSpeedLeft);
    fscanf(f, "%d", &minSpeedRight);

    fscanf(f, "%d", &maxSpeedLeft);
    fscanf(f, "%d", &maxSpeedRight);

    fscanf(f, "%d", &k1);
    fscanf(f, "%d", &k2);

    fscanf(f, "%d", &greenDurationLeft);
    fscanf(f, "%d", &greenDurationRight);

    fscanf(f, "%d", &ambulancesPercentage);

    fclose(f);
}

int arrivalTime(int mean) {
    double u = (double)rand() / RAND_MAX;
    double res = -log(1.0 - u) * mean;
    return (int)res;
}

int isAmbulance(int percentage) {
    double u = (double)rand() / RAND_MAX;
    if (u <= (double)percentage/100) return 1;
    return 0;
}

int generateSpeed(int mean, int min, int max) {
    double u1 = (double)rand() / RAND_MAX;
    double u2 = (double)rand() / RAND_MAX;
    if (u1 <= 0.0) u1 = 0.0000001;
    double z0 = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
    double desviation = (max - min) / 6.0;
    int velocidad = (int)round(mean + (z0 * desviation));
    if (velocidad < min) velocidad = min;
    if (velocidad > max) velocidad = max;

    return velocidad;
}

int cantCarsInTimeX(int tiempoX, int mediaLlegada) {
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

void generateAbsoluteTimes(int cantidadCarros, int mediaLlegada, int tiemposAbsolutos[]) {
    if (cantidadCarros <= 0) return;

    int tiempoAcumulado = 0;
    for (int i = 0; i < cantidadCarros; i++) {
        int tiempoEspera = arrivalTime(mediaLlegada);
        tiempoAcumulado += tiempoEspera;
        tiemposAbsolutos[i] = tiempoAcumulado;
    }
}

// ======================================================================================
//     Traffic light
// ======================================================================================
int trafficLigthControllerFlag = 1;
int trafficLight = 0;

void* trafficLightRoutine(void* arg) {
    while(trafficLigthControllerFlag) {
        pthread_mutex_lock(&trafficLightMutex);
        trafficLight = 0;
        printf("[Side L] Green Light for %d seconds\n", greenDurationLeft);
        pthread_cond_broadcast(&trafficLightCond);
        pthread_mutex_unlock(&trafficLightMutex);
        sleep(greenDurationLeft);

        pthread_mutex_lock(&trafficLightMutex);
        trafficLight = 1;
        printf("[Side R] Green Light for %d seconds\n", greenDurationRight);
        pthread_cond_broadcast(&trafficLightCond);
        pthread_mutex_unlock(&trafficLightMutex);
        sleep(greenDurationRight);
    }

    return NULL;
}

void* trafficLightLeftSideRoutine(void* arg) {
    Car car = *(Car*)arg;
    double secondsPerMutex = 1.0/car.speed;

    pthread_mutex_lock(&trafficLightMutex);
    printf("[Side I] %s %d ready\n", (car.isAmbulance) ? "Ambulance" : "Car", car.id);

    if (car.isAmbulance) {
        ambulancesWaitingLeft++;

        while (carInBridge > 0 && bridgeDirection != 0) {
            printf("[Side I] Ambulance %d waiting for bridge to clear\n", car.id);
            pthread_cond_wait(&condAmbulance, &trafficLightMutex);
        }

        ambulancesWaitingLeft--;
    } else {
        while (trafficLight || (carInBridge > 0 && bridgeDirection != 0) || ambulancesRight > 0) {
            printf("[Side I] Car %d waiting at time %d\n", car.id, car.arrivalTime);
            if (trafficLight || ambulancesRight > 0) {
                pthread_cond_wait(&trafficLightCond, &trafficLightMutex);
            } else {
                pthread_cond_wait(&condCounter, &trafficLightMutex);
            }
        }
    }

    carInBridge++;
    bridgeDirection = 0;
    pthread_mutex_unlock(&trafficLightMutex);

    for (int i = 0; i < bridgeWeight; i++) {
        pthread_mutex_lock(&mutexs[i]);

        printf("[Side I] %s %d in block %d\n", car.isAmbulance ? "Ambulance" : "Car", car.id, i);
        usleep(secondsPerMutex * 1000000);

        pthread_mutex_unlock(&mutexs[i]);
    }

    pthread_mutex_lock(&trafficLightMutex);
    carInBridge--;

    if (!carInBridge) {
        bridgeDirection = -1;
        pthread_cond_broadcast(&condCounter);
        pthread_cond_broadcast(&condAmbulance);
    }

    pthread_mutex_unlock(&trafficLightMutex);
    printf("[Side I] %s %d terminate\n", car.isAmbulance ? "Ambulance" : "Car", car.id);
    return NULL;
}

void* trafficLightRightSideRoutine(void* arg) {
    Car car = *(Car*)arg;
    double secondsPerMutex = 1.0/car.speed;

    pthread_mutex_lock(&trafficLightMutex);
    printf("[Side D] %s %d ready\n", car.isAmbulance ? "Ambulance" : "Car", car.id);

    if (car.isAmbulance) {
        ambulancesWaitingRight++;

        while (carInBridge > 0 && bridgeDirection != 1) {
            printf("[Side D] Ambulance %d waiting for bridge to clear\n", car.id);
            pthread_cond_wait(&condAmbulance, &trafficLightMutex);
        }

        ambulancesWaitingRight--;
    } else {
        while (trafficLight != 1 || (carInBridge > 0 && bridgeDirection != 1) || ambulancesWaitingLeft > 0) {
            printf("[Side D] Car %d waiting at time %d\n", car.id, car.arrivalTime);
            if (trafficLight != 1 || ambulancesWaitingLeft > 0) {
                pthread_cond_wait(&trafficLightCond, &trafficLightMutex);
            } else {
                // semáforo verde pero hay carros, esperar que este vacio
                pthread_cond_wait(&condCounter, &trafficLightMutex);
            }
        }
    }

    carInBridge++;
    bridgeDirection = 1;
    pthread_mutex_unlock(&trafficLightMutex);

    for (int i = bridgeWeight - 1; i >= 0; i--) {
        pthread_mutex_lock(&mutexs[i]);
        printf("[Side D] %s %d in block %d\n", car.isAmbulance ? "Ambulance" : "Car", car.id, i);
        usleep(secondsPerMutex * 1000000);
        pthread_mutex_unlock(&mutexs[i]);
    }

    pthread_mutex_lock(&trafficLightMutex);
    carInBridge--;

    if (!carInBridge) {
        bridgeDirection = -1;
        pthread_cond_broadcast(&condCounter);
        pthread_cond_broadcast(&condAmbulance);
    }

    pthread_mutex_unlock(&trafficLightMutex);
    printf("[Side D] %s %d terminate\n", car.isAmbulance ? "Ambulance" : "Car", car.id);
    return NULL;
}

void trafficLightMode(int seconds) {
    pthread_t trafficLightController;
    pthread_t carroIzq_t[seconds];
    pthread_t carroDer_t[seconds];

    pthread_create(&trafficLightController, NULL, trafficLightRoutine, NULL);

    int izqIndex = 0;
    int derIndex = 0;

    int secondNextDerCar = arrivalTime(averageArrivalTimeRight);
    int secondNextIzqCar = arrivalTime(averageArrivalTimeLeft);
    Car car;

    for (int t = 1; t <= seconds; t++) {
        if (t == secondNextDerCar) {
            car.id = derIndex;
            car.speed = generateSpeed(averageSpeedLeft, minSpeedLeft, maxSpeedLeft);
            car.arrivalTime = t;
            car.isAmbulance = isAmbulance(ambulancesPercentage);

            pthread_create(&carroIzq_t[derIndex], NULL, trafficLightRightSideRoutine, &car);
            derIndex++;
            secondNextDerCar += max(1, arrivalTime(averageArrivalTimeRight));
        }
        if (t == secondNextIzqCar) {
            car.id = izqIndex;
            car.speed = generateSpeed(averageSpeedLeft, minSpeedLeft, maxSpeedLeft);
            car.arrivalTime = t;
            car.isAmbulance = isAmbulance(ambulancesPercentage);

            pthread_create(&carroIzq_t[izqIndex], NULL, trafficLightLeftSideRoutine, &car);
            izqIndex++;
            secondNextIzqCar += max(1, arrivalTime(averageArrivalTimeLeft));
        }

        sleep(1);
    }

    for (int i = 0; i < izqIndex-1; i++) {
        pthread_join(carroIzq_t[i], NULL);
    }

    for (int i = 0; i < derIndex-1; i++) {
        pthread_join(carroDer_t[i], NULL);
    }

    // stop traffic light controller thread
    trafficLigthControllerFlag = 0;
    pthread_join(trafficLightController, NULL);
}


// ============================================================================================================================
//     MODO CARNAGE // FIFOOOOO
// ============================================================================================================================

pthread_mutex_t mutexPuente = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condLado1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t condLado2 = PTHREAD_COND_INITIALIZER;

void enterBridge(Car* miCarro) {
    pthread_mutex_lock(&bridgeMutex);

    if (miCarro->type == 1) { // --- LEFT SIDE ---
        if (miCarro->isAmbulance) ambulancesWaitingLeft++;
        while (carsInBridgeRight > 0 ||
            (!miCarro->isAmbulance && (ambulancesWaitingLeft > 0 || ambulancesWaitingRight > 0))) {
            pthread_cond_wait(&leftSideCond, &bridgeMutex);
        }

        if (miCarro->isAmbulance) ambulancesWaitingLeft--;
        carsInBridgeLeft++;

        //printf("[Lado 1] ENTRA. (%s) | Vel: %d\n",
        //miCarro->isAmbulance ? "AMBULANCIA" : "Normal", miCarro->speed);

    } else { // --- LADO 2 ---
        if (miCarro->isAmbulance) ambulanciasEsperando2++;

        while (carsInBridgeLeft > 0 ||
            (!miCarro->isAmbulance && (ambulancesWaitingLeft > 0 || ambulancesWaitingRight > 0)) ||
            (miCarro->isAmbulance && ambulancesWaitingLeft > 0)) {
            pthread_cond_wait(&rightSideCond, &bridgeMutex);
        }

        if (miCarro->isAmbulance) ambulanciasEsperando2--;
        carrosEnPuenteLado2++;

        //printf("[Lado 2] ENTRA. (%s) | Vel: %d\n",
        // miCarro->isAmbulance ? "AMBULANCIA" : "Normal", miCarro->speed);
    }

    pthread_mutex_unlock(&bridgeMutex);
}

void exitBridge(Car* miCarro) {
    pthread_mutex_lock(&bridgeMutex);

    if (miCarro->type == 1) {
        carrosEnPuenteLado1--;
        //printf("[Lado 1] SALE. (%s)\n", miCarro->isAmbulance ? "AMBULANCIA" : "Normal");
    } else {
        carrosEnPuenteLado2--;
        //printf("[Lado 2] SALE. (%s)\n", miCarro->isAmbulance ? "AMBULANCIA" : "Normal");
    }

    if (carsInBridgeLeft == 0 && carsInBridgeRight == 0) {
        pthread_cond_broadcast(&leftSideCond);
        pthread_cond_broadcast(&rightSideCond);
    }

    pthread_mutex_unlock(&bridgeMutex);
}

void* carRoutine(void* arg) {
    Car* miCarro = (Car*)arg;

    enterBridge(miCarro);

    int tiempoCruce = bridgeWeight / miCarro->speed;
    if (tiempoCruce <= 0) tiempoCruce = 1;

    int microSegundosPorPaso = (tiempoCruce * 1000000) / longitudPuente;
    int posActual = (miCarro->type == 1) ? 0 : (longitudPuente - 1);
    int direccion = (miCarro->type == 1) ? 1 : -1;

    pthread_mutex_lock(&mutexs[posActual]);

    pthread_mutex_lock(&graphicMutex);
    visualBridge[posActual] = miCarro->isAmbulance ? (miCarro->type == 1 ? 3 : 4) : miCarro->type;
    pthread_mutex_unlock(&graphicMutex);

    usleep(microSegundosPorPaso);

    for (int paso = 1; paso < bridgeWeight; paso++) {
        int posSiguiente = posActual + direccion;

        pthread_mutex_lock(&mutexs[posSiguiente]);

        pthread_mutex_lock(&graphicMutex);
        visualBridge[posActual] = 0;
        visualBridge[posSiguiente] = miCarro->isAmbulance ? (miCarro->type == 1 ? 3 : 4) : miCarro->type;
        pthread_mutex_unlock(&graphicMutex);

        pthread_mutex_unlock(&mutexs[posActual]);

        posActual = posSiguiente;

        usleep(microSegundosPorPaso);
    }

    pthread_mutex_lock(&graphicMutex);
    visualBridge[posActual] = 0;
    pthread_mutex_unlock(&graphicMutex);

    pthread_mutex_unlock(&mutexs[posActual]);

    exitBridge(miCarro);

    free(miCarro);
    return NULL;
}

void* generadorLado1(void* arg) {
    int tiempoSimulacion = *(int*)arg;
    int totalCarros = cantCarsInTimeX(tiempoSimulacion, averageArrivalTimeLeft);

    if (totalCarros > 0) {
        int tiempos[totalCarros];
        generateAbsoluteTimes(totalCarros, averageArrivalTimeLeft, tiempos);

        int tiempoActual = 0;
        int contadorId = 1;

        for (int i = 0; i < totalCarros; i++) {
            int espera = tiempos[i] - tiempoActual;

            if (espera > 0) {
                sleep(espera);
                tiempoActual += espera;
            }

            // --- NACE EL CARRO ---
            Car* nuevoCarro = (Car*)malloc(sizeof(Car));
            nuevoCarro->id = contadorId++;
            nuevoCarro->arrivalTime = tiempos[i];
            nuevoCarro->speed = generateSpeed(averageSpeedLeft, minSpeedLeft, maxSpeedLeft);
            nuevoCarro->isAmbulance = isAmbulance(ambulancesPercentage);
            nuevoCarro->type = 1;

            pthread_t hiloCarro;
            pthread_create(&hiloCarro, NULL, carRoutine, (void*)nuevoCarro);
            pthread_detach(hiloCarro);
        }
    }
    return NULL;
}

void* generadorLado2(void* arg) {
    int tiempoSimulacion = *(int*)arg;
    int totalCarros = cantCarsInTimeX(tiempoSimulacion, averageArrivalTimeRight);

    if (totalCarros > 0) {
        int tiempos[totalCarros];
        generateAbsoluteTimes(totalCarros, averageArrivalTimeRight, tiempos);

        int tiempoActual = 0;
        int contadorId = 1;

        for (int i = 0; i < totalCarros; i++) {
            int espera = tiempos[i] - tiempoActual;

            if (espera > 0) {
                sleep(espera);
                tiempoActual += espera;
            }

            Car* nuevoCarro = (Car*)malloc(sizeof(Car));
            nuevoCarro->id = contadorId++;
            nuevoCarro->arrivalTime = tiempos[i];
            nuevoCarro->speed = generateSpeed(averageSpeedRight, minSpeedRight, maxSpeedRight);
            nuevoCarro->isAmbulance = isAmbulance(ambulancesPercentage);
            nuevoCarro->type = 2;

            pthread_t hiloCarro;
            pthread_create(&hiloCarro, NULL, carRoutine, (void*)nuevoCarro);
            pthread_detach(hiloCarro);
        }
    }
    return NULL;
}

void* hiloDibujanteCarnage(void* arg) {
    while (activeSimulation) {
        printf("\033[H\033[J");

        printf("=============================================================\n");
        printf("                  ONE WAY BRIDGE SIMULADOR                   \n");
        printf("=============================================================\n\n");

        printf("         \033[0;37m");
        for (int i = 0; i < bridgeWeight*3; i++) printf("=");
        printf("\033[0m\n");

        printf("       | \033[100m");

        pthread_mutex_lock(&graphicMutex);
        for (int i = 0; i < bridgeWeight; i++) {
            switch(visualBridge[i]) {
                case 0: printf("\033[100;37m ░ \033[0m\033[100m"); break;
                case 1: printf("\033[44;1;37m > \033[0m\033[100m"); break;
                case 2: printf("\033[42;1;37m < \033[0m\033[100m"); break;
                case 3: printf("\033[41;1;33mA >\033[0m\033[100m"); break;
                case 4: printf("\033[41;1;33m< A\033[0m\033[100m"); break;
            }
        }
        pthread_mutex_unlock(&graphicMutex);

        printf("\033[0m |\n");

        printf("         \033[0;37m");
        for (int i = 0; i < bridgeWeight *3; i++) printf("=");
        printf("\033[0m\n");
        printf("           \033[0;36m");
        for (int i = 0; i < bridgeWeight *3; i++) printf("≈");
        printf("\033[0m\n\n");
        usleep(166666);
    }
    return NULL;
}

// ============================================================================================================================
//     MODO 3: OFICIAL DE TRANSITO
// ============================================================================================================================

pthread_mutex_t mutexTransito = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condNormalTransito = PTHREAD_COND_INITIALIZER;
pthread_cond_t condAmbulanciaTransito = PTHREAD_COND_INITIALIZER;
pthread_cond_t condOficial = PTHREAD_COND_INITIALIZER;

int carsinbridgepolice = 0;
int direccbridgepolice = -1;

int ambleftbridgepolice = 0;
int ambrightbridgepolice = 0;

int turnoOficial = 0;
int vehiculosPasadosTurno = 0;
int esperandoIzqNormales = 0;
int esperandoDerNormales = 0;

void* oficialControlador(void* arg) {
    while(simulacionActiva) {
        pthread_mutex_lock(&mutexTransito);

        while (esperandoIzqNormales == 0 && esperandoDerNormales == 0 && simulacionActiva) {
            pthread_cond_wait(&condOficial, &mutexTransito);
        }

        if (!simulacionActiva) {
            pthread_mutex_unlock(&mutexTransito);
            break;
        }

        if (esperandoIzqNormales > 0) {
            turnoOficial = 0;
            vehiculosPasadosTurno = 0;
            pthread_cond_broadcast(&condNormalTransito);

            while (vehiculosPasadosTurno < k1 && esperandoIzqNormales > 0 &&
                   ambleftbridgepolice == 0 && ambrightbridgepolice == 0) {
                pthread_cond_wait(&condOficial, &mutexTransito);
            }
        }

        if (esperandoDerNormales > 0) {
            turnoOficial = 1;
            vehiculosPasadosTurno = 0;
            pthread_cond_broadcast(&condNormalTransito);

            while (vehiculosPasadosTurno < k2 && esperandoDerNormales > 0 &&
                   ambleftbridgepolice == 0 && ambrightbridgepolice == 0) {
                pthread_cond_wait(&condOficial, &mutexTransito);
            }
        }

        pthread_mutex_unlock(&mutexTransito);
    }
    return NULL;
}

void* ladoIzquierdoTransito(void *arg) {
    Car* miCarro = (Car*)arg;
    int tiempoCruce = longitudPuente / miCarro->speed;
    if (tiempoCruce <= 0) tiempoCruce = 1;
    int microSegundosPorPaso = (tiempoCruce * 1000000) / longitudPuente;

    pthread_mutex_lock(&mutexTransito);

    if (miCarro->isAmbulance) {
        ambleftbridgepolice++;
        pthread_cond_signal(&condOficial);

        while (carsinbridgepolice > 0 && direccbridgepolice != 0) {
            pthread_cond_wait(&condAmbulanciaTransito, &mutexTransito);
        }
        ambleftbridgepolice--;
    } else {
        esperandoIzqNormales++;
        pthread_cond_signal(&condOficial);

        while (turnoOficial != 0 || vehiculosPasadosTurno >= k1 ||
              (carsinbridgepolice > 0 && direccbridgepolice != 0) ||
              ambleftbridgepolice > 0 || ambrightbridgepolice > 0) {
            pthread_cond_wait(&condNormalTransito, &mutexTransito);
        }

        esperandoIzqNormales--;
        vehiculosPasadosTurno++;
        pthread_cond_signal(&condOficial);
    }

    carsinbridgepolice++;
    direccbridgepolice = 0;
    pthread_mutex_unlock(&mutexTransito);

    int posActual = 0;
    pthread_mutex_lock(&mutexs[posActual]);
    pthread_mutex_lock(&mutexGrafico);
    puenteVisual[posActual] = miCarro->isAmbulance ? 3 : 1;
    pthread_mutex_unlock(&mutexGrafico);
    usleep(microSegundosPorPaso);

    for (int paso = 1; paso < longitudPuente; paso++) {
        int posSiguiente = posActual + 1;
        pthread_mutex_lock(&mutexs[posSiguiente]);

        pthread_mutex_lock(&mutexGrafico);
        puenteVisual[posActual] = 0;
        puenteVisual[posSiguiente] = miCarro->isAmbulance ? 3 : 1;
        pthread_mutex_unlock(&mutexGrafico);

        pthread_mutex_unlock(&mutexs[posActual]);
        posActual = posSiguiente;
        usleep(microSegundosPorPaso);
    }

    pthread_mutex_lock(&mutexGrafico);
    puenteVisual[posActual] = 0;
    pthread_mutex_unlock(&mutexGrafico);
    pthread_mutex_unlock(&mutexs[posActual]);

    pthread_mutex_lock(&mutexTransito);
    carsinbridgepolice--;
    if (carsinbridgepolice == 0) {
        direccbridgepolice = -1;
        pthread_cond_broadcast(&condNormalTransito);
        pthread_cond_broadcast(&condAmbulanciaTransito);
        pthread_cond_signal(&condOficial);
    }
    pthread_mutex_unlock(&mutexTransito);

    free(miCarro);
    return NULL;
}

void* ladoDerechoTransito(void *arg) {
    Car* miCarro = (Car*)arg;
    int tiempoCruce = longitudPuente / miCarro->speed;
    if (tiempoCruce <= 0) tiempoCruce = 1;
    int microSegundosPorPaso = (tiempoCruce * 1000000) / longitudPuente;

    pthread_mutex_lock(&mutexTransito);

    if (miCarro->isAmbulance) {
        ambrightbridgepolice++;
        pthread_cond_signal(&condOficial);

        while (carsinbridgepolice > 0 && direccbridgepolice != 1) {
            pthread_cond_wait(&condAmbulanciaTransito, &mutexTransito);
        }
        ambrightbridgepolice--;
    } else {
        esperandoDerNormales++;
        pthread_cond_signal(&condOficial);

        while (turnoOficial != 1 || vehiculosPasadosTurno >= k2 ||
              (carsinbridgepolice > 0 && direccbridgepolice != 1) ||
              ambleftbridgepolice > 0 || ambrightbridgepolice > 0) {
            pthread_cond_wait(&condNormalTransito, &mutexTransito);
        }

        esperandoDerNormales--;
        vehiculosPasadosTurno++;
        pthread_cond_signal(&condOficial);
    }

    carsinbridgepolice++;
    direccbridgepolice = 1;
    pthread_mutex_unlock(&mutexTransito);

    int posActual = longitudPuente - 1;
    pthread_mutex_lock(&mutexs[posActual]);
    pthread_mutex_lock(&mutexGrafico);
    puenteVisual[posActual] = miCarro->isAmbulance ? 4 : 2;
    pthread_mutex_unlock(&mutexGrafico);
    usleep(microSegundosPorPaso);

    for (int paso = 1; paso < longitudPuente; paso++) {
        int posSiguiente = posActual - 1;
        pthread_mutex_lock(&mutexs[posSiguiente]);

        pthread_mutex_lock(&mutexGrafico);
        puenteVisual[posActual] = 0;
        puenteVisual[posSiguiente] = miCarro->isAmbulance ? 4 : 2;
        pthread_mutex_unlock(&mutexGrafico);

        pthread_mutex_unlock(&mutexs[posActual]);
        posActual = posSiguiente;
        usleep(microSegundosPorPaso);
    }

    pthread_mutex_lock(&mutexGrafico);
    puenteVisual[posActual] = 0;
    pthread_mutex_unlock(&mutexGrafico);
    pthread_mutex_unlock(&mutexs[posActual]);

    pthread_mutex_lock(&mutexTransito);
    carsinbridgepolice--;
    if (carsinbridgepolice == 0) {
        direccbridgepolice = -1;
        pthread_cond_broadcast(&condNormalTransito);
        pthread_cond_broadcast(&condAmbulanciaTransito);
        pthread_cond_signal(&condOficial);
    }
    pthread_mutex_unlock(&mutexTransito);

    free(miCarro);
    return NULL;
}

void* generadorLado1Transito(void* arg) {
    int tiempoSimulacion = *(int*)arg;
    int totalCarros = cantidadCarrosEnTiempoX(tiempoSimulacion, mediaLLegada1);

    if (totalCarros > 0) {
        int tiempos[totalCarros];
        generarTiemposAbsolutos(totalCarros, mediaLLegada1, tiempos);
        int tiempoActual = 0;

        for (int i = 0; i < totalCarros; i++) {
            int espera = tiempos[i] - tiempoActual;
            if (espera > 0) { sleep(espera); tiempoActual += espera; }

            Car* nuevoCarro = (Car*)malloc(sizeof(Car));
            nuevoCarro->id = i + 1;
            nuevoCarro->horaLlegada = tiempos[i];
            nuevoCarro->speed = generarVelocidad(velocidadPromedio1, minVelocidad1, maxVelocidad1);
            nuevoCarro->isAmbulance = esAmbulancia(porcentajeAmbulancias);
            nuevoCarro->type = 1;

            pthread_t hiloCarro;
            pthread_create(&hiloCarro, NULL, ladoIzquierdoTransito, (void*)nuevoCarro);
            pthread_detach(hiloCarro);
        }
    }
    return NULL;
}

void* generadorLado2Transito(void* arg) {
    int tiempoSimulacion = *(int*)arg;
    int totalCarros = cantidadCarrosEnTiempoX(tiempoSimulacion, mediaLLegada2);

    if (totalCarros > 0) {
        int tiempos[totalCarros];
        generarTiemposAbsolutos(totalCarros, mediaLLegada2, tiempos);
        int tiempoActual = 0;

        for (int i = 0; i < totalCarros; i++) {
            int espera = tiempos[i] - tiempoActual;
            if (espera > 0) { sleep(espera); tiempoActual += espera; }

            Car* nuevoCarro = (Car*)malloc(sizeof(Car));
            nuevoCarro->id = i + 1;
            nuevoCarro->horaLlegada = tiempos[i];
            nuevoCarro->speed = generarVelocidad(velocidadPromedio2, minVelocidad2, maxVelocidad2);
            nuevoCarro->isAmbulance = esAmbulancia(porcentajeAmbulancias);
            nuevoCarro->type = 2;

            pthread_t hiloCarro;
            pthread_create(&hiloCarro, NULL, ladoDerechoTransito, (void*)nuevoCarro);
            pthread_detach(hiloCarro);
        }
    }
    return NULL;
}
void* hiloDibujanteTransito(void* arg) {
    while (simulacionActiva) {
        printf("\033[H\033[J");

        printf("=============================================================\n");
        printf("            SIMULADOR DE PUENTE - MODO TRANSITO              \n");
        printf("=============================================================\n\n");

        // --- ESTADO DEL OFICIAL ---
        printf("  👮 ESTADO DEL OFICIAL:\n");
        if (turnoOficial == 0) {
            printf("  Dando paso a: [LADO 1 / OESTE]  (Han cruzado %d de %d cupos)\n\n", vehiculosPasadosTurno, k1);
        } else {
            printf("  Dando paso a: [LADO 2 / ESTE]   (Han cruzado %d de %d cupos)\n\n", vehiculosPasadosTurno, k2);
        }

        printf("  [ LADO 1 (OESTE) ]                      [ LADO 2 (ESTE) ]\n");
        printf("  Fila Normal: %d                         Fila Normal: %d\n",
               esperandoIzqNormales, esperandoDerNormales);
        printf("  Ambulancias: %d                         Ambulancias: %d\n",
               ambulanciasIzqTransito, ambulanciasDerTransito);
        printf("-------------------------------------------------------------\n\n");

        printf("         \033[0;37m");
        int numcalles = longitudPuente * 3;
        for (int i = 0; i < numcalles; i++) printf("=");
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
        for (int i = 0; i < numcalles; i++) {
            printf("=");
        }
        printf("\033[0m\n");

        printf("         \033[0;36m");
        int numOlas = longitudPuente * 3;
        for (int i = 0; i < numOlas; i++) {
            printf("≈");
        }
        printf("\033[0m\n");

        usleep(166666);
    }
    return NULL;
}

void transitoModo(int tiempoSim) {
    simulacionActiva = 1;

    pthread_t dibujante;
    pthread_create(&dibujante, NULL, hiloDibujante, NULL);

    pthread_t oficial;
    pthread_create(&oficial, NULL, oficialControlador, NULL);

    pthread_t gen1, gen2;
    pthread_create(&gen1, NULL, generadorLado1Transito, &tiempoSim);
    pthread_create(&gen2, NULL, generadorLado2Transito, &tiempoSim);

    pthread_join(gen1, NULL);
    pthread_join(gen2, NULL);

    sleep(10);

    simulacionActiva = 0;
    pthread_cond_signal(&condOficial);

    pthread_join(oficial, NULL);
    pthread_join(dibujante, NULL);
}

int main() {
    srand(time(NULL));

    readConfigFile();

    // init cond vars and mutexs
    mutexsInit();

    puenteVisual = (int*)calloc(longitudPuente, sizeof(int));
    printf("Cuanto va a durar la simulacion? ");
    int tiempoSim;
    scanf("%d", &tiempoSim);
    printf("      ONE WAY BRIDGE SIMULATION - START       \n");
    printf("Select a mode:\n");
    printf("1. Carnage (FIFO)\n");
    printf("2. Traffic Lights\n");
    printf("3. Traffic Officers\n");
    printf("Type a number between (1-3): ");

    int modoelec;
    scanf("%d", &modoelec);

    switch (modoelec) {
        case 1:
            printf("Carnage (FIFO)...\n\n");
            activeSimulation = 1;
            pthread_t dibujante;
            pthread_create(&dibujante, NULL, hiloDibujanteCarnage, NULL);
            pthread_t gen1, gen2;
            pthread_create(&gen1, NULL, generadorLado1, &tiempoSim);
            pthread_create(&gen2, NULL, generadorLado2, &tiempoSim);

            pthread_join(gen1, NULL);
            pthread_join(gen2, NULL);

            sleep(10);
            printf("Simulation Finished.\n");

            activeSimulation = 0;
            pthread_join(dibujante, NULL);
            break;
        case 2:
            trafficLightMode(tiempoSim);
            break;
        case 3:
            printf("Iniciando modalidad Oficial de Transito...\n\n");
            transitoModo(tiempoSim);
            printf("Simulacion finalizada.\n");
            break;
        default:
            printf("\n[ERROR] Unknown mode. must be 1, 2 o 3.\n");
            return 0;
    }
    
    // destruir el mutex
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&mutexGrafico);
    pthread_mutex_destroy(&mutexPuente);

    pthread_cond_destroy(&cond);

    free(puenteVisual);
    destroyMutexsAndConds();
    return 0;
}
