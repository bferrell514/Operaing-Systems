#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char * argv[])
{
	// Check the number of arguments
	if (argc % 3 != 1)
	{
		printf("usage: ./fibo2 [F1 F2 n]+, with F2>F1>0 and n>0.\n");
		return 1;
	}
	// Prepare a dynamic array of unique numbers
	int uniqueCapacity = 1;
	int uniqueCount = 0;
	int tripletCount = 0;
	int * uniqueData = (int *)malloc(sizeof(int));
	// Process one triplet at a time
	while (tripletCount * 3 + 1 < argc)
	{
		// Check F1, F2
		int F1 = atoi(argv[tripletCount * 3 + 1]);
		int F2 = atoi(argv[tripletCount * 3 + 2]);
		if (F1 <= 0 || F2 <= 0 || F1 >= F2)
		{
			printf("usage: ./fibo2 [F1 F2 n]+, with F2>F1>0 and n>0.\n");
			tripletCount++;
			continue;
		}
		// Check n
		int n = atoi(argv[tripletCount * 3 + 3]);
		if (n <= 0)
		{
			printf("usage: ./fibo2 [F1 F2 n]+, with F2>F1>0 and n>0.\n");
			tripletCount++;
			continue;
		}
		tripletCount++;
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
		// Update unique array
		for (int i = 0; i < n; i++)
		{
			int isDuplicate = 0;
			for (int j = 0; j < uniqueCount; j++)
			{
				if (uniqueData[j] == fibo[i])
				{
					isDuplicate = 1;
					break;
				}
			}
			// If a new integer is found
			if (isDuplicate == 0)
			{
				// Check if the uniqueCapacity is enough
				if (uniqueCount + 1 <= uniqueCapacity)
				{
					// Just append
					uniqueData[uniqueCount] = fibo[i];
					uniqueCount++;
				}
				else
				{
					// Increase the length by 2x and append
					int * newUniqueData = (int *)malloc(2 * uniqueCount * sizeof(int));
					memcpy(newUniqueData, uniqueData, uniqueCount * sizeof(int));
					newUniqueData[uniqueCount] = fibo[i];
					uniqueCount++;
					uniqueCapacity *= 2;
					free(uniqueData);
					uniqueData = newUniqueData;
				}
			}
		}
		// Free
		free(fibo);
	}
	// Print all unique numbers
	printf("The values encountered in the sequences are\n{");
	for (int i = 0; i < uniqueCount; i++)
	{
		if (i + 1 < uniqueCount)
			printf("%d, ", uniqueData[i]);
		else
			printf("%d}\n", uniqueData[i]);
	}
	// Free
	free(uniqueData);
	return 0;
}
