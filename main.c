#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>

#define M_PI 3.14159265358979323846

// ========================================================================================
//     GLOBALS
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

int totalActiveCars = 0;
pthread_mutex_t activeCarsMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t allCarsFinishedCond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t trafficLightMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t trafficLightCond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t *mutexs;
pthread_cond_t *conds;

int carInBridge = 0;
int bridgeDirection = -1;
pthread_cond_t condCounter = PTHREAD_COND_INITIALIZER;

pthread_mutex_t bridgeMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t leftSideCond = PTHREAD_COND_INITIALIZER;
pthread_cond_t rightSideCond = PTHREAD_COND_INITIALIZER;

pthread_cond_t condAmbulance = PTHREAD_COND_INITIALIZER;
pthread_mutex_t officerMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t officerNormalCond = PTHREAD_COND_INITIALIZER;
pthread_cond_t officerAmbulanceCond = PTHREAD_COND_INITIALIZER;
pthread_cond_t officerWakeupCond = PTHREAD_COND_INITIALIZER;

int officerCarsInBridge = 0;
int officerBridgeDirection = -1;

int officerAmbulancesLeft = 0;
int officerAmbulancesRight = 0;

int officerTurn = 0;
int carsPassedThisTurn = 0;
int waitingLeftNormal = 0;
int waitingRightNormal = 0;

int trafficLigthControllerFlag = 1;
int trafficLight = 0;

// ========================================================================================
//     INIT / DESTROY
// ========================================================================================
void mutexsInit() {
    mutexs = malloc(bridgeWeight * sizeof(pthread_mutex_t));
    conds  = malloc(bridgeWeight * sizeof(pthread_cond_t));
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

    pthread_mutex_destroy(&graphicMutex);
    pthread_mutex_destroy(&bridgeMutex);
    pthread_cond_destroy(&leftSideCond);
    pthread_cond_destroy(&rightSideCond);

    pthread_mutex_destroy(&officerMutex);
    pthread_cond_destroy(&officerNormalCond);
    pthread_cond_destroy(&officerAmbulanceCond);
    pthread_cond_destroy(&officerWakeupCond);
}

void readConfigFile() {
    FILE *f = fopen("config.txt", "r");
    if (f == NULL) { printf("Error while opening config.txt\n"); exit(1); }

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

// ========================================================================================
//     UTILITY
// ========================================================================================
int arrivalTime(int mean) {
    double u = (double)rand() / RAND_MAX;
    double res = -log(1.0 - u) * mean;
    return (int)res;
}

int isAmbulance(int percentage) {
    double u = (double)rand() / RAND_MAX;
    if (u <= (double)percentage / 100) return 1;
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

// ========================================================================================
//     TRAFFIC LIGHT MODE
// ========================================================================================

void* graphicThreadTrafficLight(void* arg) {
    while (activeSimulation) {
        printf("\033[H\033[J");

        printf("=============================================================\n");
        printf("          BRIDGE SIMULATOR - TRAFFIC LIGHT MODE             \n");
        printf("=============================================================\n\n");

        if (trafficLight == 0) {
            printf("  SEMAPHORE: \033[1;32m[ LEFT SIDE  GREEN ]\033[0m  "
                   "\033[0;31m[ RIGHT SIDE  RED  ]\033[0m\n\n");
        } else {
            printf("  SEMAPHORE: \033[0;31m[ LEFT SIDE  RED  ]\033[0m  "
                   "\033[1;32m[ RIGHT SIDE  GREEN ]\033[0m\n\n");
        }

        printf("  [ LEFT SIDE ]                           [ RIGHT SIDE ]\n");
        printf("  Cars on bridge (direction: %s): %d\n\n",
               bridgeDirection == 0 ? "LEFT->RIGHT" :
               bridgeDirection == 1 ? "RIGHT<-LEFT" : "none",
               carInBridge);

        printf("         \033[0;37m");
        for (int i = 0; i < bridgeWeight * 3; i++) printf("=");
        printf("\033[0m\n");

        printf("       | \033[100m");

        pthread_mutex_lock(&graphicMutex);
        for (int i = 0; i < bridgeWeight; i++) {
            switch (visualBridge[i]) {
                case 0: printf("\033[100;37m . \033[0m\033[100m"); break;
                case 1: printf("\033[44;1;37m > \033[0m\033[100m"); break;
                case 2: printf("\033[42;1;37m < \033[0m\033[100m"); break;
                case 3: printf("\033[41;1;33mA>\033[0m\033[100m");  break;
                case 4: printf("\033[41;1;33m<A\033[0m\033[100m");  break;
            }
        }
        pthread_mutex_unlock(&graphicMutex);

        printf("\033[0m |\n");

        printf("         \033[0;37m");
        for (int i = 0; i < bridgeWeight * 3; i++) printf("=");
        printf("\033[0m\n");

        printf("         \033[0;36m");
        for (int i = 0; i < bridgeWeight * 3; i++) printf("~");
        printf("\033[0m\n\n");

        usleep(166666);
    }
    return NULL;
}

void* trafficLightRoutine(void* arg) {
    while (trafficLigthControllerFlag) {
        pthread_mutex_lock(&trafficLightMutex);
        trafficLight = 0;
        pthread_cond_broadcast(&trafficLightCond);
        pthread_mutex_unlock(&trafficLightMutex);
        sleep(greenDurationLeft);

        pthread_mutex_lock(&trafficLightMutex);
        trafficLight = 1;
        pthread_cond_broadcast(&trafficLightCond);
        pthread_mutex_unlock(&trafficLightMutex);
        sleep(greenDurationRight);
    }
    return NULL;
}

void* trafficLightLeftSideRoutine(void* arg) {
    Car car = *(Car*)arg;
    free(arg);

    double secondsPerMutex = 1.0 / car.speed;

    pthread_mutex_lock(&trafficLightMutex);

    if (car.isAmbulance) {
        ambulancesWaitingLeft++;
        while (carInBridge > 0 && bridgeDirection != 0) {
            pthread_cond_wait(&condAmbulance, &trafficLightMutex);
        }
        ambulancesWaitingLeft--;
    } else {
        while (trafficLight || (carInBridge > 0 && bridgeDirection != 0) || ambulancesWaitingRight > 0) {
            if (trafficLight || ambulancesWaitingRight > 0) {
                pthread_cond_wait(&trafficLightCond, &trafficLightMutex);
            } else {
                pthread_cond_wait(&condCounter, &trafficLightMutex);
            }
        }
    }

    carInBridge++;
    bridgeDirection = 0;
    pthread_mutex_unlock(&trafficLightMutex);

    int currentPos = 0;
    pthread_mutex_lock(&mutexs[currentPos]);

    pthread_mutex_lock(&graphicMutex);
    visualBridge[currentPos] = car.isAmbulance ? 3 : 1;
    pthread_mutex_unlock(&graphicMutex);

    usleep(secondsPerMutex * 1000000);

    for (int i = 1; i < bridgeWeight; i++) {
        int nextPos = currentPos + 1;
        pthread_mutex_lock(&mutexs[nextPos]);

        pthread_mutex_lock(&graphicMutex);
        visualBridge[currentPos] = 0;
        visualBridge[nextPos] = car.isAmbulance ? 3 : 1;
        pthread_mutex_unlock(&graphicMutex);

        pthread_mutex_unlock(&mutexs[currentPos]);
        currentPos = nextPos;
        usleep(secondsPerMutex * 1000000);
    }

    pthread_mutex_lock(&graphicMutex);
    visualBridge[currentPos] = 0;
    pthread_mutex_unlock(&graphicMutex);

    pthread_mutex_unlock(&mutexs[currentPos]);

    pthread_mutex_lock(&trafficLightMutex);
    carInBridge--;
    if (!carInBridge) {
        bridgeDirection = -1;
        pthread_cond_broadcast(&condCounter);
        pthread_cond_broadcast(&condAmbulance);
    }
    pthread_mutex_unlock(&trafficLightMutex);

    return NULL;
}

void* trafficLightRightSideRoutine(void* arg) {
    Car car = *(Car*)arg;
    free(arg);

    double secondsPerMutex = 1.0 / car.speed;

    pthread_mutex_lock(&trafficLightMutex);

    if (car.isAmbulance) {
        ambulancesWaitingRight++;
        while (carInBridge > 0 && bridgeDirection != 1) {
            pthread_cond_wait(&condAmbulance, &trafficLightMutex);
        }
        ambulancesWaitingRight--;
    } else {
        while (trafficLight != 1 || (carInBridge > 0 && bridgeDirection != 1) || ambulancesWaitingLeft > 0) {
            if (trafficLight != 1 || ambulancesWaitingLeft > 0) {
                pthread_cond_wait(&trafficLightCond, &trafficLightMutex);
            } else {
                pthread_cond_wait(&condCounter, &trafficLightMutex);
            }
        }
    }

    carInBridge++;
    bridgeDirection = 1;
    pthread_mutex_unlock(&trafficLightMutex);

    int currentPos = bridgeWeight - 1;
    pthread_mutex_lock(&mutexs[currentPos]);

    pthread_mutex_lock(&graphicMutex);
    visualBridge[currentPos] = car.isAmbulance ? 4 : 2;
    pthread_mutex_unlock(&graphicMutex);

    usleep(secondsPerMutex * 1000000);

    for (int i = 1; i < bridgeWeight; i++) {
        int nextPos = currentPos - 1;
        pthread_mutex_lock(&mutexs[nextPos]);

        pthread_mutex_lock(&graphicMutex);
        visualBridge[currentPos] = 0;
        visualBridge[nextPos] = car.isAmbulance ? 4 : 2;
        pthread_mutex_unlock(&graphicMutex);

        pthread_mutex_unlock(&mutexs[currentPos]);
        currentPos = nextPos;
        usleep(secondsPerMutex * 1000000);
    }

    pthread_mutex_lock(&graphicMutex);
    visualBridge[currentPos] = 0;
    pthread_mutex_unlock(&graphicMutex);

    pthread_mutex_unlock(&mutexs[currentPos]);

    pthread_mutex_lock(&trafficLightMutex);
    carInBridge--;
    if (!carInBridge) {
        bridgeDirection = -1;
        pthread_cond_broadcast(&condCounter);
        pthread_cond_broadcast(&condAmbulance);
    }
    pthread_mutex_unlock(&trafficLightMutex);

    return NULL;
}

void trafficLightMode(int seconds) {
    activeSimulation = 1;

    pthread_t graphicThread;
    pthread_create(&graphicThread, NULL, graphicThreadTrafficLight, NULL);

    pthread_t trafficLightController;
    pthread_t carroIzq_t[seconds];
    pthread_t carroDer_t[seconds];

    pthread_create(&trafficLightController, NULL, trafficLightRoutine, NULL);

    int izqIndex = 0;
    int derIndex = 0;

    int t1 = arrivalTime(averageArrivalTimeRight);
    int t2 = arrivalTime(averageArrivalTimeLeft);

    int secondNextDerCar = (t1 > 1) ? t1 : 1;
    int secondNextIzqCar = (t2 > 1) ? t2 : 1;

    for (int t = 1; t <= seconds; t++) {
        if (t == secondNextDerCar) {
            Car* car = malloc(sizeof(Car));
            car->id          = derIndex;
            car->speed       = generateSpeed(averageSpeedRight, minSpeedRight, maxSpeedRight);
            car->arrivalTime = t;
            car->isAmbulance = isAmbulance(ambulancesPercentage);

            pthread_create(&carroDer_t[derIndex], NULL, trafficLightRightSideRoutine, car);
            derIndex++;

            int dt = arrivalTime(averageArrivalTimeRight);
            secondNextDerCar += (dt > 1) ? dt : 1;
        }
        if (t == secondNextIzqCar) {
            Car* car = malloc(sizeof(Car));
            car->id          = izqIndex;
            car->speed       = generateSpeed(averageSpeedLeft, minSpeedLeft, maxSpeedLeft);
            car->arrivalTime = t;
            car->isAmbulance = isAmbulance(ambulancesPercentage);

            pthread_create(&carroIzq_t[izqIndex], NULL, trafficLightLeftSideRoutine, car);
            izqIndex++;

            int dt = arrivalTime(averageArrivalTimeLeft);
            secondNextIzqCar += (dt > 1) ? dt : 1;
        }

        sleep(1);
    }

    for (int i = 0; i < izqIndex; i++) pthread_join(carroIzq_t[i], NULL);
    for (int i = 0; i < derIndex; i++) pthread_join(carroDer_t[i], NULL);

    sleep(10); // let last cars finish crossing

    trafficLigthControllerFlag = 0;
    activeSimulation = 0;

    pthread_join(trafficLightController, NULL);
    pthread_join(graphicThread, NULL);
}

// ============================================================================================================================
//     MODO CARNAGE // FIFOOOOO
// ============================================================================================================================

void enterBridge(Car* miCarro) {
    pthread_mutex_lock(&bridgeMutex);

    if (miCarro->type == 1) {
        if (miCarro->isAmbulance) ambulancesWaitingLeft++;

        while (carsInBridgeRight > 0 ||
            (!miCarro->isAmbulance && (ambulancesWaitingLeft > 0 || ambulancesWaitingRight > 0))) {
            pthread_cond_wait(&leftSideCond, &bridgeMutex);
        }

        if (miCarro->isAmbulance) ambulancesWaitingLeft--;
        carsInBridgeLeft++;

    } else {
        if (miCarro->isAmbulance) ambulancesWaitingRight++;

        while (carsInBridgeLeft > 0 ||
            (!miCarro->isAmbulance && (ambulancesWaitingLeft > 0 || ambulancesWaitingRight > 0)) ||
            (miCarro->isAmbulance && ambulancesWaitingLeft > 0)) {
            pthread_cond_wait(&rightSideCond, &bridgeMutex);
        }

        if (miCarro->isAmbulance) ambulancesWaitingRight--;
        carsInBridgeRight++;
    }

    pthread_mutex_unlock(&bridgeMutex);
}

void exitBridge(Car* miCarro) {
    pthread_mutex_lock(&bridgeMutex);

    if (miCarro->type == 1) {
        carsInBridgeLeft--;
    } else {
        carsInBridgeRight--;
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

    int microSegundosPorPaso = (tiempoCruce * 1000000) / bridgeWeight;
    int posActual = (miCarro->type == 1) ? 0 : (bridgeWeight - 1);
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
    pthread_mutex_lock(&activeCarsMutex);
    totalActiveCars--;
    if (totalActiveCars == 0) {
        pthread_cond_broadcast(&allCarsFinishedCond);
    }
    pthread_mutex_unlock(&activeCarsMutex);
    return NULL;
}

void* generadorLado1(void* arg) {
    int tiempoSimulacion = *(int*)arg;
    int totalCarros = cantCarsInTimeX(tiempoSimulacion, averageArrivalTimeLeft);

    if (totalCarros > 0) {
        pthread_mutex_lock(&activeCarsMutex);
        totalActiveCars += totalCarros;
        pthread_mutex_unlock(&activeCarsMutex);
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
        pthread_mutex_lock(&activeCarsMutex);
        totalActiveCars += totalCarros;
        pthread_mutex_unlock(&activeCarsMutex);
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
    printf("\033[H\033[J\033[?25l");

    while (activeSimulation) {
        printf("\033[H");

        printf("=============================================================\n");
        printf("                  ONE WAY BRIDGE SIMULATOR                   \n");
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

        fflush(stdout);

        usleep(166666);
    }

    printf("\033[?25h");
    return NULL;
}

// ============================================================================================================================
//     MODE TRAFFIC OFFICER
// ============================================================================================================================
void* trafficOfficerController(void* arg) {
    while(activeSimulation) {
        pthread_mutex_lock(&officerMutex);

        while (waitingLeftNormal == 0 && waitingRightNormal == 0 && activeSimulation) {
            pthread_cond_wait(&officerWakeupCond, &officerMutex);
        }

        if (!activeSimulation) {
            pthread_mutex_unlock(&officerMutex);
            break;
        }

        if (waitingLeftNormal > 0) {
            officerTurn = 0;
            carsPassedThisTurn = 0;
            pthread_cond_broadcast(&officerNormalCond);

            while (carsPassedThisTurn < k1 && waitingLeftNormal > 0 &&
                   officerAmbulancesLeft == 0 && officerAmbulancesRight == 0) {
                pthread_cond_wait(&officerWakeupCond, &officerMutex);
            }
        }

        if (waitingRightNormal > 0) {
            officerTurn = 1;
            carsPassedThisTurn = 0;
            pthread_cond_broadcast(&officerNormalCond);

            while (carsPassedThisTurn < k2 && waitingRightNormal > 0 &&
                   officerAmbulancesLeft == 0 && officerAmbulancesRight == 0) {
                pthread_cond_wait(&officerWakeupCond, &officerMutex);
            }
        }

        pthread_mutex_unlock(&officerMutex);
    }
    return NULL;
}

void* trafficOfficerLeftSideRoutine(void *arg) {
    Car* car = (Car*)arg;
    int crossTime = bridgeWeight / car->speed;
    if (crossTime <= 0) crossTime = 1;
    int microSecondsPerStep = (crossTime * 1000000) / bridgeWeight;

    pthread_mutex_lock(&officerMutex);

    if (car->isAmbulance) {
        officerAmbulancesLeft++;
        pthread_cond_signal(&officerWakeupCond);

        while (officerCarsInBridge > 0 && officerBridgeDirection != 0) {
            pthread_cond_wait(&officerAmbulanceCond, &officerMutex);
        }
        officerAmbulancesLeft--;
    } else {
        waitingLeftNormal++;
        pthread_cond_signal(&officerWakeupCond);

        while (officerTurn != 0 || carsPassedThisTurn >= k1 ||
              (officerCarsInBridge > 0 && officerBridgeDirection != 0) ||
              officerAmbulancesLeft > 0 || officerAmbulancesRight > 0) {
            pthread_cond_wait(&officerNormalCond, &officerMutex);
        }

        waitingLeftNormal--;
        carsPassedThisTurn++;
        pthread_cond_signal(&officerWakeupCond);
    }

    officerCarsInBridge++;
    officerBridgeDirection = 0;
    pthread_mutex_unlock(&officerMutex);

    int currentPos = 0;
    pthread_mutex_lock(&mutexs[currentPos]);
    pthread_mutex_lock(&graphicMutex);
    visualBridge[currentPos] = car->isAmbulance ? 3 : 1;
    pthread_mutex_unlock(&graphicMutex);
    usleep(microSecondsPerStep);

    for (int step = 1; step < bridgeWeight; step++) {
        int nextPos = currentPos + 1;
        pthread_mutex_lock(&mutexs[nextPos]);

        pthread_mutex_lock(&graphicMutex);
        visualBridge[currentPos] = 0;
        visualBridge[nextPos] = car->isAmbulance ? 3 : 1;
        pthread_mutex_unlock(&graphicMutex);

        pthread_mutex_unlock(&mutexs[currentPos]);
        currentPos = nextPos;
        usleep(microSecondsPerStep);
    }

    pthread_mutex_lock(&graphicMutex);
    visualBridge[currentPos] = 0;
    pthread_mutex_unlock(&graphicMutex);
    pthread_mutex_unlock(&mutexs[currentPos]);

    pthread_mutex_lock(&officerMutex);
    officerCarsInBridge--;
    if (officerCarsInBridge == 0) {
        officerBridgeDirection = -1;
        pthread_cond_broadcast(&officerNormalCond);
        pthread_cond_broadcast(&officerAmbulanceCond);
        pthread_cond_signal(&officerWakeupCond); // Bridge empty, officer checks queues
    }
    pthread_mutex_unlock(&officerMutex);

    free(car);
    pthread_mutex_lock(&activeCarsMutex);
    totalActiveCars--;
    if (totalActiveCars == 0) {
        pthread_cond_broadcast(&allCarsFinishedCond);
    }
    pthread_mutex_unlock(&activeCarsMutex);
    return NULL;
}

void* trafficOfficerRightSideRoutine(void *arg) {
    Car* car = (Car*)arg;
    int crossTime = bridgeWeight / car->speed;
    if (crossTime <= 0) crossTime = 1;
    int microSecondsPerStep = (crossTime * 1000000) / bridgeWeight;

    pthread_mutex_lock(&officerMutex);

    if (car->isAmbulance) {
        officerAmbulancesRight++;
        pthread_cond_signal(&officerWakeupCond);

        while (officerCarsInBridge > 0 && officerBridgeDirection != 1) {
            pthread_cond_wait(&officerAmbulanceCond, &officerMutex);
        }
        officerAmbulancesRight--;
    } else {
        waitingRightNormal++;
        pthread_cond_signal(&officerWakeupCond);

        while (officerTurn != 1 || carsPassedThisTurn >= k2 ||
              (officerCarsInBridge > 0 && officerBridgeDirection != 1) ||
              officerAmbulancesLeft > 0 || officerAmbulancesRight > 0) {
            pthread_cond_wait(&officerNormalCond, &officerMutex);
        }

        waitingRightNormal--;
        carsPassedThisTurn++;
        pthread_cond_signal(&officerWakeupCond);
    }

    officerCarsInBridge++;
    officerBridgeDirection = 1;
    pthread_mutex_unlock(&officerMutex);

    int currentPos = bridgeWeight - 1;
    pthread_mutex_lock(&mutexs[currentPos]);
    pthread_mutex_lock(&graphicMutex);
    visualBridge[currentPos] = car->isAmbulance ? 4 : 2;
    pthread_mutex_unlock(&graphicMutex);
    usleep(microSecondsPerStep);

    for (int step = 1; step < bridgeWeight; step++) {
        int nextPos = currentPos - 1;
        pthread_mutex_lock(&mutexs[nextPos]);

        pthread_mutex_lock(&graphicMutex);
        visualBridge[currentPos] = 0;
        visualBridge[nextPos] = car->isAmbulance ? 4 : 2;
        pthread_mutex_unlock(&graphicMutex);

        pthread_mutex_unlock(&mutexs[currentPos]);
        currentPos = nextPos;
        usleep(microSecondsPerStep);
    }

    pthread_mutex_lock(&graphicMutex);
    visualBridge[currentPos] = 0;
    pthread_mutex_unlock(&graphicMutex);
    pthread_mutex_unlock(&mutexs[currentPos]);

    pthread_mutex_lock(&officerMutex);
    officerCarsInBridge--;
    if (officerCarsInBridge == 0) {
        officerBridgeDirection = -1;
        pthread_cond_broadcast(&officerNormalCond);
        pthread_cond_broadcast(&officerAmbulanceCond);
        pthread_cond_signal(&officerWakeupCond);
    }
    pthread_mutex_unlock(&officerMutex);

    free(car);
    pthread_mutex_lock(&activeCarsMutex);
    totalActiveCars--;
    if (totalActiveCars == 0) {
        pthread_cond_broadcast(&allCarsFinishedCond);
    }
    pthread_mutex_unlock(&activeCarsMutex);
    return NULL;
}

void* generatorLeftSideOfficer(void* arg) {
    int simTime = *(int*)arg;
    int totalCars = cantCarsInTimeX(simTime, averageArrivalTimeLeft);

    if (totalCars > 0) {
        pthread_mutex_lock(&activeCarsMutex);
        totalActiveCars += totalCars;
        pthread_mutex_unlock(&activeCarsMutex);
        int absoluteTimes[totalCars];
        generateAbsoluteTimes(totalCars, averageArrivalTimeLeft, absoluteTimes);
        int currentTime = 0;

        for (int i = 0; i < totalCars; i++) {
            int waitTime = absoluteTimes[i] - currentTime;
            if (waitTime > 0) { sleep(waitTime); currentTime += waitTime; }

            Car* newCar = (Car*)malloc(sizeof(Car));
            newCar->id = i + 1;
            newCar->arrivalTime = absoluteTimes[i];
            newCar->speed = generateSpeed(averageSpeedLeft, minSpeedLeft, maxSpeedLeft);
            newCar->isAmbulance = isAmbulance(ambulancesPercentage);
            newCar->type = 1;

            pthread_t carThread;
            pthread_create(&carThread, NULL, trafficOfficerLeftSideRoutine, (void*)newCar);
            pthread_detach(carThread);
        }
    }
    return NULL;
}

void* generatorRightSideOfficer(void* arg) {
    int simTime = *(int*)arg;
    int totalCars = cantCarsInTimeX(simTime, averageArrivalTimeRight);

    if (totalCars > 0) {
        pthread_mutex_lock(&activeCarsMutex);
        totalActiveCars += totalCars;
        pthread_mutex_unlock(&activeCarsMutex);
        int absoluteTimes[totalCars];
        generateAbsoluteTimes(totalCars, averageArrivalTimeRight, absoluteTimes);
        int currentTime = 0;

        for (int i = 0; i < totalCars; i++) {
            int waitTime = absoluteTimes[i] - currentTime;
            if (waitTime > 0) { sleep(waitTime); currentTime += waitTime; }

            Car* newCar = (Car*)malloc(sizeof(Car));
            newCar->id = i + 1;
            newCar->arrivalTime = absoluteTimes[i];
            newCar->speed = generateSpeed(averageSpeedRight, minSpeedRight, maxSpeedRight);
            newCar->isAmbulance = isAmbulance(ambulancesPercentage);
            newCar->type = 2;

            pthread_t carThread;
            pthread_create(&carThread, NULL, trafficOfficerRightSideRoutine, (void*)newCar);
            pthread_detach(carThread);
        }
    }
    return NULL;
}

void* graphicThreadOfficer(void* arg) {
    while (activeSimulation) {
        printf("\033[H\033[J");

        printf("=============================================================\n");
        printf("            BRIDGE SIMULATOR - TRAFFIC OFFICER MODE          \n");
        printf("=============================================================\n\n");

        printf(" OFFICER STATE:\n");
        if (officerTurn == 0) {
            printf("  Giving turn to: [LEFT SIDE]  (Crossed %d out of %d limit)\n\n", carsPassedThisTurn, k1);
        } else {
            printf("  Giving turn to: [RIGHT SIDE] (Crossed %d out of %d limit)\n\n", carsPassedThisTurn, k2);
        }

        printf("  [ LEFT SIDE ]                           [ RIGHT SIDE ]\n");
        printf("  Normal Queue: %d                         Normal Queue: %d\n",
               waitingLeftNormal, waitingRightNormal);
        printf("  Ambulances: %d                           Ambulances: %d\n",
               officerAmbulancesLeft, officerAmbulancesRight);
        printf("-------------------------------------------------------------\n\n");

        printf("         \033[0;37m");
        int bridgeVisualLength = bridgeWeight * 3;
        for (int i = 0; i < bridgeVisualLength; i++) printf("=");
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
        for (int i = 0; i < bridgeVisualLength; i++) {
            printf("=");
        }
        printf("\033[0m\n");

        printf("         \033[0;36m");
        for (int i = 0; i < bridgeVisualLength; i++) {
            printf("≈");
        }
        printf("\033[0m\n");

        usleep(166666);
    }
    return NULL;
}

void trafficOfficerMode(int simTime) {
    activeSimulation = 1;

    pthread_t graphicThread;
    pthread_create(&graphicThread, NULL, graphicThreadOfficer, NULL);

    pthread_t officer;
    pthread_create(&officer, NULL, trafficOfficerController, NULL);

    pthread_t gen1, gen2;
    pthread_create(&gen1, NULL, generatorLeftSideOfficer, &simTime);
    pthread_create(&gen2, NULL, generatorRightSideOfficer, &simTime);

    pthread_join(gen1, NULL);
    pthread_join(gen2, NULL);

    pthread_mutex_lock(&activeCarsMutex);
    while (totalActiveCars > 0) {
        pthread_cond_wait(&allCarsFinishedCond, &activeCarsMutex);
    }
    pthread_mutex_unlock(&activeCarsMutex);

    activeSimulation = 0;
    pthread_cond_signal(&officerWakeupCond);

    pthread_join(officer, NULL);
    pthread_join(graphicThread, NULL);
}

int main() {
    srand(time(NULL));

    readConfigFile();

    // init cond vars and mutexs
    mutexsInit();

    visualBridge = (int*)calloc(bridgeWeight, sizeof(int));
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

            pthread_mutex_lock(&activeCarsMutex);
            while (totalActiveCars > 0) {
                // El main se duerme hasta que escuche que los carros llegaron a 0
                pthread_cond_wait(&allCarsFinishedCond, &activeCarsMutex);
            }
            pthread_mutex_unlock(&activeCarsMutex);
            printf("Simulation Finished.\n");

            activeSimulation = 0;
            pthread_join(dibujante, NULL);
            break;
        case 2:
            trafficLightMode(tiempoSim);
            break;
        case 3:
            printf("Iniciando modalidad Oficial de Transito...\n\n");
            trafficOfficerMode(tiempoSim);
            printf("Simulacion finalizada.\n");
            break;
        default:
            printf("\n[ERROR] Unknown mode. must be 1, 2 o 3.\n");
            return 0;
    }


    free(visualBridge);
    destroyMutexsAndConds();
    return 0;
}