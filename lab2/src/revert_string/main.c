#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "revert_string.h"



int main(int argc, char *argv[]) 
{
	// Проверяем, что аргументов командной строки ровно 2 (имя программы и строка для переворота)
	if (argc != 2)
	{
		printf("Usage: %s string_to_revert\n", argv[0]); // Выводим инструкцию по использованию и завершаем программу с кодом ошибки -1
		return -1;
	}

	// Выделяем память под копию строки, учитывая длину и символ конца строки '\0'
	char *reverted_str = malloc(sizeof(char) * (strlen(argv[1]) + 1));
	
	strcpy(reverted_str, argv[1]); // Копируем введенную строку в выделенную память

	RevertString(reverted_str);  

	printf("Reverted: %s\n", reverted_str); 
	free(reverted_str);
	return 0;
}

