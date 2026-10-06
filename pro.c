#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PROCESOS 100
#define MAX_NOMBRE 50


typedef struct {
    int processId;
    int parentProcessId;
    char programCounter[MAX_NOMBRE];
    int registros;
    int tamano;
    int hilos;
    int quantum;
    int iteraciones;

    int quantumRestante;
    int iteracionActual;
    int tiempoCPU;
} Proceso;


typedef struct Nodo {
    Proceso *proceso;
    struct Nodo *siguiente;
} Nodo;


typedef struct {
    Nodo *frente;
    Nodo *final;
    int cantidad;
} Cola;


void inicializarCola(Cola *cola) {
    cola->frente = NULL;
    cola->final = NULL;
    cola->cantidad = 0;
}


int colaVacia(Cola *cola) {
    return cola->frente == NULL;
}

void encolar(Cola *cola, Proceso *proceso) {
    Nodo *nuevo = (Nodo *)malloc(sizeof(Nodo));

    if (nuevo == NULL) {
        printf("Error: no se pudo reservar memoria.\n");
        exit(EXIT_FAILURE);
    }

    nuevo->proceso = proceso;
    nuevo->siguiente = NULL;

    if (colaVacia(cola)) {
        cola->frente = nuevo;
        cola->final = nuevo;
    } else {
        cola->final->siguiente = nuevo;
        cola->final = nuevo;
    }

    cola->cantidad++;
}

Proceso *desencolar(Cola *cola) {
    if (colaVacia(cola)) {
        return NULL;
    }

    Nodo *aux = cola->frente;
    Proceso *proceso = aux->proceso;

    cola->frente = cola->frente->siguiente;

    if (cola->frente == NULL) {
        cola->final = NULL;
    }

    free(aux);
    cola->cantidad--;

    return proceso;
}

/* Muestra los procesos que están esperando */
void mostrarCola(Cola *cola) {
    Nodo *actual = cola->frente;

    printf("Cola de procesos: ");

    if (actual == NULL) {
        printf("VACIA\n");
        return;
    }

    while (actual != NULL) {
        printf("[P%d - %s - I%d] ",
               actual->proceso->processId,
               actual->proceso->programCounter,
               actual->proceso->iteracionActual);

        actual = actual->siguiente;
    }

    printf("\n");
}

/* Carga los datos desde el archivo */
int cargarConfiguracion(
    const char *nombreArchivo,
    int *procesadores,
    int *hilos,
    Proceso procesos[],
    int *cantidadProcesos
) {
    FILE *archivo = fopen(nombreArchivo, "r");

    if (archivo == NULL) {
        printf("Error: no se pudo abrir el archivo '%s'.\n", nombreArchivo);
        return 0;
    }

    char linea[300];

    *cantidadProcesos = 0;

    /* Leer cantidad de procesadores */
    if (fgets(linea, sizeof(linea), archivo) == NULL) {
        printf("Error: no se pudo leer la configuracion de procesadores.\n");
        fclose(archivo);
        return 0;
    }

    if (sscanf(linea, "Procesadores %d", procesadores) != 1) {
        printf("Error: formato incorrecto para Procesadores.\n");
        fclose(archivo);
        return 0;
    }

    if (fgets(linea, sizeof(linea), archivo) == NULL) {
        printf("Error: no se pudo leer la configuracion de hilos.\n");
        fclose(archivo);
        return 0;
    }

    if (sscanf(linea, "Hilos %d", hilos) != 1) {
        printf("Error: formato incorrecto para Hilos.\n");
        fclose(archivo);
        return 0;
    }

    /* Leer procesos */
    while (fgets(linea, sizeof(linea), archivo) != NULL) {

        if (strlen(linea) <= 2) {
            continue;
        }

        Proceso *p = &procesos[*cantidadProcesos];

        int resultado = sscanf(
            linea,
            " %d | %d | %49[^|] | %d | %d | %d | %d | %d",
            &p->processId,
            &p->parentProcessId,
            p->programCounter,
            &p->registros,
            &p->tamano,
            &p->hilos,
            &p->quantum,
            &p->iteraciones
        );

        if (resultado != 8) {
            printf("Advertencia: linea ignorada por formato incorrecto:\n%s", linea);
            continue;
        }

        /* Eliminar espacios al final del nombre */
        int longitud = strlen(p->programCounter);

        while (longitud > 0 &&
               (p->programCounter[longitud - 1] == ' ' ||
                p->programCounter[longitud - 1] == '\t')) {
            p->programCounter[longitud - 1] = '\0';
            longitud--;
        }

        /* Inicializar datos de simulacion */
        p->quantumRestante = p->quantum;
        p->iteracionActual = 1;
        p->tiempoCPU = 0;

        (*cantidadProcesos)++;

        if (*cantidadProcesos >= MAX_PROCESOS) {
            printf("Advertencia: se alcanzo el maximo de procesos.\n");
            break;
        }
    }

    fclose(archivo);

    return 1;
}

/* Muestra la informacion inicial */
void mostrarConfiguracion(
    int procesadores,
    int hilos,
    Proceso procesos[],
    int cantidadProcesos
) {
    printf("\n============================================\n");
    printf("       SIMULADOR DE PROCESOS - C\n");
    printf("============================================\n");

    printf("Procesadores: %d\n", procesadores);
    printf("Hilos disponibles: %d\n", hilos);
    printf("Procesos cargados: %d\n", cantidadProcesos);

    printf("\nProcesos:\n");

    printf("--------------------------------------------\n");

    for (int i = 0; i < cantidadProcesos; i++) {
        printf(
            "P%d | Padre: P%d | PC: %s | Registros: %d | "
            "Tamano: %d Bytes | Hilos: %d | Quantum: %d | Iteraciones: %d\n",
            procesos[i].processId,
            procesos[i].parentProcessId,
            procesos[i].programCounter,
            procesos[i].registros,
            procesos[i].tamano,
            procesos[i].hilos,
            procesos[i].quantum,
            procesos[i].iteraciones
        );
    }

    printf("--------------------------------------------\n");
}

void simular(
    int procesadores,
    int hilos,
    Proceso procesos[],
    int cantidadProcesos
) {
    Cola cola;
    inicializarCola(&cola);

    /* Agregar todos los procesos a la cola */
    for (int i = 0; i < cantidadProcesos; i++) {
        encolar(&cola, &procesos[i]);
    }

    int ciclo = 0;

    printf("\n============================================\n");
    printf("          INICIO DE LA SIMULACION\n");
    printf("============================================\n");

    /*
     * La simulacion continua hasta que no existan
     * procesos pendientes.
     */
    while (!colaVacia(&cola)) {

        ciclo++;

        printf("\n\n--------------------------------------------\n");
        printf("CICLO DE CPU: %d\n", ciclo);
        printf("--------------------------------------------\n");

        printf("Procesadores disponibles: %d\n", procesadores);
        printf("Hilos disponibles: %d\n", hilos);

        mostrarCola(&cola);

        /*
         * Se ejecutan procesos de acuerdo con la
         * cantidad de procesadores disponibles.
         */
        for (int cpu = 0; cpu < procesadores; cpu++) {

            if (colaVacia(&cola)) {
                break;
            }

            Proceso *proceso = desencolar(&cola);

            printf("\nProcesador %d\n", cpu + 1);
            printf("  Proceso: P%d\n", proceso->processId);
            printf("  Nombre: %s\n", proceso->programCounter);
            printf("  Hilos del proceso: %d\n", proceso->hilos);
            printf("  Iteracion: %d de %d\n",
                   proceso->iteracionActual,
                   proceso->iteraciones);
            printf("  Quantum inicial: %d\n", proceso->quantum);
            printf("  Quantum restante: %d\n",
                   proceso->quantumRestante);

            /*
             * Verificar que el proceso pueda ejecutarse
             * segun los hilos disponibles.
             */
            if (proceso->hilos > hilos) {
                printf("  Estado: ESPERANDO - demasiados hilos.\n");

                encolar(&cola, proceso);
                continue;
            }

            printf("  Estado: EJECUTANDO\n");

            /*
             * Simulacion del quantum.
             * Cada ciclo consume una unidad de quantum.
             */
            while (proceso->quantumRestante > 0) {

                proceso->quantumRestante--;
                proceso->tiempoCPU++;

                printf(
                    "    CPU -> P%d | Iteracion %d | Quantum restante: %d | "
                    "Tiempo CPU: %d\n",
                    proceso->processId,
                    proceso->iteracionActual,
                    proceso->quantumRestante,
                    proceso->tiempoCPU
                );
            }

            printf("  Quantum agotado.\n");

            /*
             * Si aun quedan iteraciones, se reinicia
             * el quantum y el proceso vuelve a la cola.
             */
            if (proceso->iteracionActual < proceso->iteraciones) {

                proceso->iteracionActual++;
                proceso->quantumRestante = proceso->quantum;

                printf(
                    "  P%d vuelve a la cola para la iteracion %d.\n",
                    proceso->processId,
                    proceso->iteracionActual
                );

                encolar(&cola, proceso);

            } else {

                printf(
                    "  P%d FINALIZADO. Todas las iteraciones fueron completadas.\n",
                    proceso->processId
                );
            }
        }

        printf("\nEstado despues del ciclo:\n");
        mostrarCola(&cola);
    }

    printf("\n============================================\n");
    printf("          SIMULACION FINALIZADA\n");
    printf("============================================\n");

    printf("\nResumen de procesos:\n");

    for (int i = 0; i < cantidadProcesos; i++) {

        printf(
            "P%d - %s | Iteraciones: %d | Tiempo total CPU: %d | FINALIZADO\n",
            procesos[i].processId,
            procesos[i].programCounter,
            procesos[i].iteraciones,
            procesos[i].tiempoCPU
        );
    }

    printf("\nTodos los procesos han terminado correctamente.\n");
}


int main() {

    int procesadores;
    int hilos;
    int cantidadProcesos;

    Proceso procesos[MAX_PROCESOS];

   
    if (!cargarConfiguracion(
            "config.txt",
            &procesadores,
            &hilos,
            procesos,
            &cantidadProcesos
        )) {

        printf("\nNo se pudo iniciar la simulacion.\n");
        return EXIT_FAILURE;
    }

   
    if (procesadores <= 0) {
        printf("Error: la cantidad de procesadores debe ser mayor que 0.\n");
        return EXIT_FAILURE;
    }

    if (hilos <= 0) {
        printf("Error: la cantidad de hilos debe ser mayor que 0.\n");
        return EXIT_FAILURE;
    }

    if (cantidadProcesos == 0) {
        printf("Error: no existen procesos para simular.\n");
        return EXIT_FAILURE;
    }

    
    mostrarConfiguracion(
        procesadores,
        hilos,
        procesos,
        cantidadProcesos
    );

   
    simular(
        procesadores,
        hilos,
        procesos,
        cantidadProcesos
    );

    return EXIT_SUCCESS;
}