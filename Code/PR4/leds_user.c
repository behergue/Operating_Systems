//Beatriz Herguedas Pinedo

//Para llamar a chardev_leds y hacer cosas

#include <stdio.h>
#include <stdlib.h>

#define PATH "/dev/chardev_leds"

int main(int argc, char * argv[]){
	//Si no le pasas 2 argumentos error (hay que poner el nombre del programa + uno de los modos)
	if (argc!=2){
		fprintf(stderr, "Uso: %s cont_binario/rotativo (Elegir uno de los dos modos)\n", argv[0]);
		exit(1);
	}

	//Para ver si podemos abrir el fichero
	FILE * file = fopen(PATH, "r+");
	if (file == NULL){
		printf("No se puede abrir el archivo\n");
		exit(1);
	}
	fclose(file);

	//Si el primer argumento al llamar por consola es contbinario
	if(strcmp(argv[1], "cont_binario") == 0){
		cont_binario();
	}

	//Si es rotativo
	else if(strcmp(argv[1], "rotativo") == 0){
		rotativo();
	}
	//Si no error
	else{
		fprintf(stderr, "Modo: %s no valido\n", argv[1]);
		exit(1);
	}

	return 0;
}

//Para que vayan rotando los leds
void rotativo(){

	//Array para pasar el entero a char
	char  leds[] = "0";

	//Descriptor de fichero
	FILE * file;

	//Bucle que abre el fichero, escribe en el led que queremos encender y lo cierra
	int i = 1;
	while(1){
		file = fopen(PATH, "r+");

		//Como un printf normal pero en vez de sacarlo por pantalla lo guarda en el array leds
		sprintf(leds, "%d", i);

		fwrite(leds, sizeof(char), strlen(leds), file);

		printf("He encendido el led %s\n", leds);
		fclose(file);

		sleep(1);

		i = (i%3) + 1;
	}
}

//Funcion que enciende los leds segun un contador binario
void cont_binario(){

	//Ponemos los 3 bits a 0
	char leds[] = "000";

	//Descriptor del fichero
	FILE * file;

	int i = 0;
	//Damos vueltas de 0 a 7 abriendo y cerrando el fichero
	while(1){
		int num = i;
		file = fopen(PATH, "r+");

		//Si hay que pintar un 1 en el led de la izquierda
		if(num >= 4){
			leds[0] = '1';
			num = num -4;
		}

		//Si hay que pintar un 1 en el led el medio
		if (num >= 2){
			leds[1] = '2';
			num = num -2;
		}

		//Si hay que pintar un 1 en el led de la derecha
		if(num >= 1){
			leds[2] = '3';
		}

		//Escribimos la cadena que nos ha quedado
		fwrite(leds, sizeof(char), strlen(leds), file);

		printf("He encendido los leds %s\n", leds);

		fclose(file);

		//Para que tarde un rato entre uno y otro
		sleep(1);

		i = (i+1)%8;

		leds[0] = '0';
		leds[1] = '0';
		leds[2] = '0';
	}
}