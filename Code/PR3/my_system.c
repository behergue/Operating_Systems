//para ejecutar un comando por consola

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
	//si no tiene dos argumentos error
	if (argc!=2){
		fprintf(stderr, "Usage: %s <command>\n", argv[0]);
		exit(1);
	}

	return system(argv[1]);
}


int system(const char * command){
	int ret = -1;

	//hace un fork y crea dos procesos con el
	int pid = fork();

	//si ha habido un error (pid no puede ser negativo salvo error)
	if(pid == -1)
		fprintf(stderr, "Fallo fork\n");

	//fork devuelve pid = 0 al hijo entonces aqui solo entra el hijo y ejecuta el comando
	else if(pid == 0)
		execlp("/bin/bash", "bash", "-c", command, (char*) NULL);
	
	//aqui entra el padre y se queda esperando al hijo
	//ret es lo que devuelve el hijo
	else{
		wait(&ret);
		ret = WEXITSTATUS(ret);
	}
}