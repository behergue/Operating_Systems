#include <stdio.h> 
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define NR_PHILOSOPHERS 5

//array de hilos de filosofos (un hilo por cada filosofo)
pthread_t philosophers[NR_PHILOSOPHERS];
//array de cerrojos de los palillos (un cerrojo por cada palillo)
pthread_mutex_t forks[NR_PHILOSOPHERS];

//inicializa los mutex
void init(){
    int i;
    for(i=0; i<NR_PHILOSOPHERS; i++)
        pthread_mutex_init(&forks[i],NULL);
    
}

//funcion de pensar
void think(int i) {
    printf("Philosopher %d thinking... \n" , i);
    sleep(random() % 10);
    printf("Philosopher %d stopped thinking!!! \n" , i);

}

//funcion de comer
void eat(int i) {
    printf("Philosopher %d eating... \n" , i);
    sleep(random() % 5);
    printf("Philosopher %d is not eating anymore!!! \n" , i);

}

//funcion de dormir
void toSleep(int i) {
    printf("Philosopher %d sleeping... \n" , i);
    sleep(random() % 10);
    printf("Philosopher %d is awake!!! \n" , i);
    
}


void* philosopher(void* i){

    //numero de filosofo
    int nPhilosopher = (int)i;

    //el palillo derecho coincide con el numero del filosofo
    int right = nPhilosopher;

    //el palillo izquieredo
    int left = (nPhilosopher - 1 == -1) ? NR_PHILOSOPHERS - 1 : (nPhilosopher - 1);


    while(1){
        
        //el filosofo empieza pensando
        think(nPhilosopher);

        //para saber si ha cogido los palillos
        //int cogidoDcha = -1, cogidoIzqda = -1;
        
        /// TRY TO GRAB BOTH FORKS (right and left)
        //si el filosofo no es el ultimo, intenta coger su palillo derecho y luego el izquierdo
        if(right < left){ 
        	pthread_mutex_lock(&forks[right]);
        	pthread_mutex_lock(&forks[left]);
        }
        //si el filosofo es el ultimo lo hace al reves (para evitar interbloqueos)
        else{
        	pthread_mutex_lock(&forks[left]);
        	pthread_mutex_lock(&forks[right]);

  		}
  		//Estaba mal porque se podían producir bucles
        /*while(cogidoDcha != 0 || cogidoIzqda != 0){

        	while(cogidoDcha != 0){
        		cogidoDcha = pthread_mutex_lock(&forks[right]);
        	}

        	cogidoIzqda = pthread_mutex_lock(&forks[left]);

        	if(cogidoIzqda != 0){
        		cogidoDcha = -1;
        		cogidoIzqda = pthread_mutex_unlock(&forks[right]);
        	}

        }*/

        //el filosofo come
        eat(nPhilosopher);
        
        // PUT FORKS BACK ON THE TABLE
        //suelta los mutex
		pthread_mutex_unlock(&forks[right]);
    	pthread_mutex_unlock(&forks[left]);
        
        //se va a dormir
        toSleep(nPhilosopher);
   }

}

int main()
{
    init();
    unsigned long i;
    for(i=0; i<NR_PHILOSOPHERS; i++)
        pthread_create(&philosophers[i], NULL, philosopher, (void*)i);
    
    for(i=0; i<NR_PHILOSOPHERS; i++)
        pthread_join(philosophers[i],NULL);

    for(i=0; i<NR_PHILOSOPHERS; i++)
        pthread_mutex_destroy(&forks[i]);
    
    return 0;
} 
