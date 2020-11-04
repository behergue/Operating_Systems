// Beatriz Herguedas Pinedo
// Pablo Hernández Aguado

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define N_PARADAS 5
// número de paradas de la ruta
#define EN_RUTA 0
// autobús en ruta
#define EN_PARADA 1
// autobús en la parada
#define MAX_USUARIOS 40 // capacidad del autobús
#define USUARIOS 4 // numero de usuarios

// estado inicial
int estado= EN_RUTA;
int parada_actual = 0; // parada en la que se encuentra el autobus
int n_ocupantes= 0; // ocupantes que tiene el autobús
// personas que desean subir en cada parada
int esperando_parada[N_PARADAS]; //= {0,0,...0};
// personas que desean bajar en cada parada
int esperando_bajar[N_PARADAS]; //= {0,0,...0};

// Otras definiciones globales (comunicación y sincronización)
// Array de mutex para las paradas
pthread_mutex_t esperaSubir[N_PARADAS];
pthread_mutex_t esperaBajar[N_PARADAS];
// Hilo del autobús
pthread_t hiloAutobus;
// Array de hilos para los usuarios
pthread_t usuarios[USUARIOS];
// Variables condicionales
pthread_cond_t suben;
pthread_cond_t bajan;
pthread_cond_t hanSubido;
pthread_cond_t hanBajado;

int min(int num1, int num2){
	if(num1 < num2)
		return num1;
	else
		return num2;
}

void * thread_autobus(void * args) {
	while (1) {
	// esperar a que los viajeros suban y bajen
		Autobus_En_Parada();
	// conducir hasta siguiente parada
		Conducir_Hasta_Siguiente_Parada();
	}
}

void * thread_usuario(int i) {
	int id_usuario, a, b;
	id_usuario = i;
// obtener el id del usario
	while (1) {
		a=rand() % N_PARADAS;
		do{
			b=rand() % N_PARADAS;
		} while(a==b);

		printf("Usuario %d se quiere subir en la parada %d y bajar en la %d \n", id_usuario, a, b);
		Usuario(id_usuario,a,b);
	}
}

void Usuario(int id_usuario, int origen, int destino) {
// Esperar a que el autobus esté en parada origen para subir
	Subir_Autobus(id_usuario, origen);
// Bajarme en estación destino
	Bajar_Autobus(id_usuario, destino);
}

int main(int argc, char *argv[]) {
	int i;
// Definición de variables locales a main
// Opcional: obtener de los argumentos del programa la capacidad del
// autobus, el numero de usuarios y el numero de paradas
// Crear el thread Autobus

// Inicializamos los mutex
	for(i=0; i < N_PARADAS; i++){
        pthread_mutex_init(&esperaSubir[i],NULL);
        pthread_mutex_init(&esperaBajar[i],NULL);
	}
	
// Inicializamos las variables condicionales
	pthread_cond_init(&suben, NULL);
	pthread_cond_init(&bajan, NULL);
	pthread_cond_init(&hanSubido, NULL);
	pthread_cond_init(&hanBajado, NULL);
	
// Creamos el hilo del autobús
	pthread_create(&hiloAutobus, NULL, thread_autobus, NULL);
	
// Crear thread para el usuario i
	for (i = 0; i < USUARIOS; i++)
		pthread_create(&usuarios[i], NULL, thread_usuario, (void *) i);

// Esperar terminación de los hilos
// El argumento del join va sin &!!!
	pthread_join(hiloAutobus, NULL);

	for (i = 0; i < USUARIOS; i++)
		pthread_join(usuarios[i], NULL);

	for(i=0; i<N_PARADAS; i++){
        pthread_mutex_destroy(&esperaSubir[i]);
        pthread_mutex_destroy(&esperaBajar[i]);
	}

	// Destruimos las variables
	pthread_cond_destroy(&suben);
	pthread_cond_destroy(&bajan);
	pthread_cond_destroy(&hanSubido);
	pthread_cond_destroy(&hanBajado);

    return 0;
}

void Autobus_En_Parada(){
/* Ajustar el estado y bloquear al autobús hasta que no haya pasajeros que
quieran bajar y/o subir la parada actual. Después se pone en marcha */
	// Ponemos el mensaje antes de cambiar el estado porque si no fallaba
	// (sacaba e mensaje de "pasajero se sube en parada x" antes de "bus en parada x")
	printf("EL bus está en la parada %d \n", parada_actual);
	estado = EN_PARADA;

	pthread_mutex_lock(&esperaSubir[parada_actual]);
	pthread_mutex_lock(&esperaBajar[parada_actual]);

	// Bajamos a los pasajeros
	n_ocupantes -= esperando_bajar[parada_actual];
	
	// Calculamos el número de pasajeros posibles que pueden subir
	int usuarios_suben = min(MAX_USUARIOS-n_ocupantes, esperando_parada[parada_actual]);
	n_ocupantes += usuarios_suben;
	
	// Avisamos a todo el que se quiera bajar
	pthread_cond_broadcast(&bajan);
	
	// Avisamos a los que quepan:
	int j;
	for(j = 0; j < usuarios_suben; j++){
		//signal solo avisa al primero que este esperando en el wait
		//cond_signal(&suben);
		//broadcast avisa a todos los que estan esperando en el wait entonces el for no haria falta
		pthread_cond_broadcast(&suben);
	}
	
	//mientras haya gente esperando en la parada para subir o para bajar
	while(esperando_parada[parada_actual] > 0 || esperando_bajar[parada_actual] > 0){
		
		//si solo quedan pasageros por bajar
		if(esperando_bajar[parada_actual] > 0 && esperando_parada[parada_actual] == 0)
			//el autobus suelta el mutex de bajada para que puedan bajar (el wait implicitamente suelta el mutex)
			pthread_cond_wait(&hanBajado, &esperaBajar[parada_actual]);

		//si solo quedan pasajeros por subir
		else if(esperando_bajar[parada_actual] == 0 && esperando_parada[parada_actual] > 0)
			//el autobus suelta el mutex de subida para que puedan bajar (el wait implicitamente suelta el mutex)
			pthread_cond_wait(&hanSubido, &esperaSubir[parada_actual]);
		
		//si quedan pasajeros por subir y por bajar
		else{
			//el bus suelta el mutex de subir
			pthread_mutex_unlock(&esperaSubir[parada_actual]);
			//suelta el de bajar y se queda esperando a que le avisen de que han terminado de bajar y vuelve a coger el mutex
			pthread_cond_wait(&hanBajado, &esperaBajar[parada_actual]);
			//el bus vuelve a coger el mutex de subir
			pthread_mutex_lock(&esperaSubir[parada_actual]);
		}

		//vuelve a iterar hasta que no quede gente por subir o por bajar
		
	}
	//el bus suelta los dos mutex
	pthread_mutex_unlock(&esperaSubir[parada_actual]);
	pthread_mutex_unlock(&esperaBajar[parada_actual]);

	estado = EN_RUTA;
	printf("El bus está en ruta \n");
}

void Conducir_Hasta_Siguiente_Parada(){
/* Establecer un Retardo que simule el trayecto y actualizar numero de parada */
	// Para simular el trayecto
	sleep((random() % 5)+1);
	parada_actual = (parada_actual + 1) % N_PARADAS;
}

void Subir_Autobus(int id_usuario, int origen){
/* El usuario indicará que quiere subir en la parada ’origen’, esperará a que
el autobús se pare en dicha parada y subirá. El id_usuario puede utilizarse para
proporcionar información de depuración */

	//la persona coge el mutex para apuntarse en la parada en la que quiere subir
	pthread_mutex_lock(&esperaSubir[origen]);
	esperando_parada[origen]++;

	//mientras el autobus no este en su parada, se queda esperando (y suelta el mutex)
	while(parada_actual != origen || estado != EN_PARADA){
		pthread_cond_wait(&suben, &esperaSubir[origen]);
	}

	//le han devuelto el mutex y se sube al autobus
	esperando_parada[origen]--;

	//si era la ultima persona en subir, avisa al autobus
	if(esperando_parada[origen] == 0)
		pthread_cond_signal(&hanSubido);

	printf("Usuario %d se sube en la parada %d \n", id_usuario, origen);
	
	//suelta el mutex
	pthread_mutex_unlock(&esperaSubir[origen]);
}

void Bajar_Autobus(int id_usuario, int destino){
/* El usuario indicará que quiere bajar en la parada ’destino’, esperará a que
el autobús se pare en dicha parada y bajará. El id_usuario puede utilizarse para
proporcionar información de depuración */

	pthread_mutex_lock(&esperaBajar[destino]);
	esperando_bajar[destino]++;

	while(parada_actual != destino || estado != EN_PARADA){
		pthread_cond_wait(&bajan, &esperaBajar[destino]);
	}

	esperando_bajar[destino]--;

	if(esperando_bajar[destino] == 0)
		pthread_cond_signal(&hanBajado);
	
	printf("Usuario %d se baja en la parada %d \n", id_usuario, destino);
	pthread_mutex_unlock(&esperaBajar[destino]);
}