//Beatriz Herguedas Pinedo

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mytar.h"

extern char *use;

/** Copy nBytes bytes from the origin file to the destination file.
 *
 * origin: pointer to the FILE descriptor associated with the origin file
 * destination:  pointer to the FILE descriptor associated with the destination file
 * nBytes: number of bytes to copy
 *
 * Returns the number of bytes actually copied or -1 if an error occured.
 */
//Copia nBytes de origin a destination
int copynFile(FILE * origin, FILE * destination, int nBytes){
	
	//creamos el array c y reservamos el espacio necesario para copiar nBytes de origin
	char* c = (char*) malloc(nBytes * sizeof(char));

	//leemos nBytes elementos de tamaño sizeof(char) de origin, los copiamos en c y devolvemos el numero de bytes leidos
	int n = fread(c, sizeof(char), nBytes, origin);

	//escribimos n elementos de tamaño sizeof(char) de c en destination
	fwrite(c, sizeof(char), n, destination);

	//liberamos c
	free(c);

	//devolvemos el numero de bytes copiados
	return n;
}

/** Loads a string from a file.
 *
 * file: pointer to the FILE descriptor 
 * 
 * The loadstr() function must allocate memory from the heap to store 
 * the contents of the string read from the FILE. 
 * Once the string has been properly built in memory, the function returns
 * the starting address of the string (pointer returned by malloc()) 
 * 
 * Returns: !=NULL if success, NULL if error
 */
//Lee un string del fichero file
char* loadstr(FILE * file){

	//creamos un contador para saber cuanto ocupa el string
	int counter = 0;

	//mientras no lleguemos al final del string aumentamos el contador
	while(getc(file) != '\0')
		counter++;

	//sabiendo ya cuanto ocupa, devolvemos el puntero al principio del string
	fseek(file, -(counter + 1), SEEK_CUR);

	//creamos el array c y reservamos el espacio necesario para copiar el string
	char* c = (char*) malloc(counter * sizeof(char));

	//leemos el string y lo guardamos en c
	fread(c, sizeof(char), counter + 1, file);

	//devolvemos el string leido
	return c;
}

/** Read tarball header and store it in memory.
 *
 * tarFile: pointer to the tarball's FILE descriptor 
 * nFiles: output parameter. Used to return the number 
 * of files stored in the tarball archive (first 4 bytes of the header).
 *
 * On success it returns the starting memory address of an array that stores 
 * the (name,size) pairs read from the tar file. Upon failure, the function returns NULL.
 */
//Lee el encabezado del fichero tarFile
stHeaderEntry* readHeader(FILE * tarFile, int *nFiles){

	//leemos el primer entero que nos dice el numero de ficheros que hay en tarFile y lo guardamos en nFiles
	fread(nFiles, sizeof(int), 1, tarFile);

	//creamos un array de pares y reservamos el espacio necesario
	stHeaderEntry* pair = (stHeaderEntry*) malloc(sizeof(stHeaderEntry) * (*nFiles));

	//tantas veces como numero de ficheros hay
	for(int i = 0; i < *nFiles; i++){
		//leemos el nombre y lo guardamos en su posicion del array creado .name
		pair[i].name = loadstr(tarFile);
		//leemos el tamaño que ocupa y lo guardamos en el array creado .size
		fread(&(pair[i].size), sizeof(int), 1, tarFile);
	}

	//devolvemos el array de pares nombre-tamaño
	return pair;
}

/** Creates a tarball archive 
 *
 * nfiles: number of files to be stored in the tarball
 * filenames: array with the path names of the files to be included in the tarball
 * tarname: name of the tarball archive
 * 
 * On success, it returns EXIT_SUCCESS; upon error it returns EXIT_FAILURE. 
 * (macros defined in stdlib.h).
 *
 * HINTS: First reserve room in the file to store the tarball header.
 * Move the file's position indicator to the data section (skip the header)
 * and dump the contents of the source files (one by one) in the tarball archive. 
 * At the same time, build the representation of the tarball header in memory.
 * Finally, rewind the file's position indicator, write the number of files as well as 
 * the (file name,file size) pairs in the tar archive.
 *
 * Important reminder: to calculate the room needed for the header, a simple sizeof 
 * of stHeaderEntry will not work. Bear in mind that, on disk, file names found in (name,size) 
 * pairs occupy strlen(name)+1 bytes.
 *
 */
//Crea el fichero tar juntando varios ficheros dados en fileNames[]
int createTar(int nFiles, char *fileNames[], char tarName[]){

	//abrimos el fichero dado tarName en modo escritura
	FILE * f = fopen(tarName, "w");

	//almacenamos en headerSize el tamaño del encabezado (numficheros + nombresficheros + tamsficheros)
	int headerSize = sizeof(int);
	for(int i = 0; i < nFiles; i++)
		headerSize += strlen(fileNames[i]) + 1 + sizeof(int);

	//nos saltamos el espacio donde va el encabezado
	fseek(f, headerSize, SEEK_SET);

	//creamos un array para almacenar los tamaños de los ficheros
	int* fileSizes = (int*) malloc(nFiles * sizeof(int));
	
	//para cada fichero
	for(int i = 0; i < nFiles; i++){
		//lo abrimos en modo lectura
		FILE * a = fopen(fileNames[i], "r");
		//lo vamos copiando de 100 en 100 bytes y guardando el tamaño que ocupa en t
		int t = copynFile(a, f, 100);
		//hasta que terminemos de copiar el fichero
		fileSizes[i] = 0;
		while(t != 0){
			//actualizamos el tamaño del fichero i en su posicion en el array
			fileSizes[i] += t;
			//y seguimos copiando
			t = copynFile(a, f, 100);
		}
		//lo cerramos
		fclose(a);
	}
	
	//volvemos al principio del fichero para escribir el encabezado
	fseek(f, 0, SEEK_SET);

	//escribimos el numero de ficheros en el encabezado
	fwrite(&nFiles, sizeof(int), 1, f);

	//para cada fichero
	for(int i = 0; i < nFiles; i++){
		//escribimos su nombre
		fwrite(fileNames[i], sizeof(char), strlen(fileNames[i]) + 1, f);
		//y su tamaño
		fwrite(&fileSizes[i], sizeof(int), 1, f);
	}

	//cerramos el fichero
	fclose(f);
	//liberamos el espacio de fileSizes
	free(fileSizes);

	//devolvemos exito
	return EXIT_SUCCESS;
}

/** Extract files stored in a tarball archive
 *
 * tarName: tarball's pathname
 *
 * On success, it returns EXIT_SUCCESS; upon error it returns EXIT_FAILURE. 
 * (macros defined in stdlib.h).
 *
 * HINTS: First load the tarball's header into memory.
 * After reading the header, the file position indicator will be located at the 
 * tarball's data section. By using information from the 
 * header --number of files and (file name, file size) pairs--, extract files 
 * stored in the data section of the tarball.
 *
 */
//Extrae todos los ficheros que forman el fichero tar
int extractTar(char tarName[]){

	//abrimos el fichero en modo lectura
	FILE * f = fopen(tarName, "r");

	//leemos el encabezado y devolvemos en nfiles el numero de ficheros que forman el tar 		y en s el array de pares nombre-tamaño
	int nFiles;
	stHeaderEntry* s = readHeader(f, &nFiles);

	//para cada fichero
	for(int i = 0; i < nFiles; i++){
		//lo abrimos en modo escritura
		FILE * a = fopen(s[i].name, "w");
		//copiamos la parte correspodiente a ese fichero de f en el
		copynFile(f, a, s[i].size);
		//lo cerramos
		fclose(a);
	}

	//cerramos f
	fclose(f);
	//devolvemos exito
	return EXIT_SUCCESS;
}
