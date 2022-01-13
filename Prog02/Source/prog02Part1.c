#include <stdio.h>
#include <stdlib.h>

void printArray(int * myArray, unsigned int size);

int main(int argc, char * argv[])
{
	// Check the number of arguments
	if (argc != 4)
	{
		printf("usage: ./fibo1 F1 F2 n, with F2>F1>0 and n>0.\n");
		return 1;
	}
	// Check F1, F2
	int F1 = atoi(argv[1]);
	int F2 = atoi(argv[2]);
	if (F1 <= 0 || F2 <= 0 || F1 >= F2)
	{
		printf("usage: ./fibo1 F1 F2 n, with F2>F1>0 and n>0.\n");
		return 1;
	}
	// Check n
	int n = atoi(argv[3]);
	if (n <= 0)
	{
		printf("usage: ./fibo1 F1 F2 n, with F2>F1>0 and n>0.\n");
		return 1;
	}
	// Create a dynamic array
	int * fibo = (int *)malloc(n * sizeof(int));
	// Initialize and calculate the first n Fibonacci numbers
	fibo[0] = F1;
	fibo[1] = F2;
	for (int i = 2; i < n; i++)
		fibo[i] = fibo[i - 1] + fibo[i - 2];
	// Print
	printf("%d terms of the Fibonacci series with F1=%d and F2=%d:\n", n, F1, F2);
	for (int i = 0; i < n; i++)
	{
		if (i + 1 < n)
			printf("F%d=%d, ", i + 1, fibo[i]);
		else
			printf("F%d=%d\n", i + 1, fibo[i]);
	}
	// Free
	free(fibo);
	return 0;
}
