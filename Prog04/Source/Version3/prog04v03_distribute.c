#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/**
 * @file prog04v03_distribute.c
 * @author Baheem Ferrell
 * @date 22 Oct 2020
 */

/**
 * @brief A node in a linked list of integers.
 *
 * This node can contain an integer value and a pointer to the next
 * node in the linked list.
 */
typedef struct IntNode
{
	int data;
	struct IntNode * next;
} IntNode;

/**
 * @brief Creates and returns file lists for different workers respectively.
 * This function creates and returns n lists of file index to be
 * to be used in processing by n different workers.
 * @param n The number of workers.
 * @param index Distributor index.
 * @param catalogAddr Pointer to a list of file paths (output).
 * @param countAddr Pointer to the number of files (output).
 * @param indexListsAddr Pointer to n lists of file index (output).
 */
void createIndexLists(int n, int index, char *** catalogAddr, int * countAddr, IntNode *** indexListsAddr)
{
	// Initialize variables
	IntNode ** indexListHeads = (IntNode **)malloc(sizeof(IntNode *) * n);
	IntNode ** indexListTails = (IntNode **)malloc(sizeof(IntNode *) * n);
	for (int i = 0; i < n; i++)
	{
		indexListHeads[i] = NULL;
		indexListTails[i] = NULL;
	}

	// Open file
	char fileName[20];
	sprintf(fileName, "%d.catalog", index);
	FILE * file = fopen(fileName, "r");
	if (file == NULL)
		return;

	// Get count
	size_t len = 0;
	ssize_t read = 0;
	char * line = NULL;
	read = getline(&line, &len, file);
	if (line[strlen(line) - 1] == (char)'\n')
		line[strlen(line) - 1] = (char)'\0';
	int fileCount = atoi(line);
	free(line);

	// Create a catalog and lists of files by reading a line at a time
	char ** catalog = (char **)malloc(sizeof(char *) * fileCount);
	for (int i = 0; i < fileCount; i++)
	{
		// Read a line
		len = 0;
		read = 0;
		read = getline(&line, &len, file);
		if (line[strlen(line) - 1] == (char)'\n')
			line[strlen(line) - 1] = (char)'\0';
		catalog[i] = line;

		// Open file
		FILE * inFile = fopen(catalog[i], "r");
		if (inFile == NULL)
		{
			printf("Cannot open input file for indexing %s.\n", catalog[i]);
			return;
		}

		// Read index
		int processorIndex = 0;
		fscanf(inFile, "%d", &processorIndex);

		// Create a node with the index
		IntNode * node = (IntNode *)malloc(sizeof(IntNode));
		node->data = i;
		node->next = NULL;

		// Add the node to appropriate list
		if (indexListTails[processorIndex] == NULL)
			indexListHeads[processorIndex] = node;
		else
			indexListTails[processorIndex]->next = node;
		indexListTails[processorIndex] = node;

		// Close file
		fclose(inFile);
	}
	free(indexListTails);

	// Output
	*catalogAddr = catalog;
	*countAddr = fileCount;
	*indexListsAddr = indexListHeads;
}

/**
 * @brief Save n lists of files as a distributor result.
 * This function saves n lists of files as the result of distributor processing.
 * @param n Number of process.
 * @param pIndex Distributor index.
 * @param catalog List of file paths.
 * @param newLists n list of files to be saved.
 */
void saveFileLists(int n, int pIndex, char ** catalog, IntNode ** newLists)
{
	// Open output file
	char fileName[10];
	sprintf(fileName, "%d.list", pIndex);
	FILE * outFile = fopen(fileName, "w");
	if (outFile == NULL)
	{
		printf("Cannot open distributor file %s.\n", fileName);
		return;
	}
	// Write index lists
	for (int i = 0; i < n; i++)
	{
		// Get the count
		IntNode * node = newLists[i];
		int count = 0;
		while (node != NULL)
		{
			node = node->next;
			count++;
		}
		// Print count
		fprintf(outFile, "%d\n", count);
		// Print file path
		node = newLists[i];
		while (node != NULL)
		{
			fprintf(outFile, "%s\n", catalog[node->data]);
			node = node->next;
		}
	}
	// Close file
	fclose(outFile);
}

/**
 * @brief Entry point.
 * This function reads lines from a list of files and
 * saves n lists of files to be distributed.
 * @param argc Number of command line arguments.
 * @param argv An array of command line arguments.
 * @return Returns 0, if successful, non-zero otherwise.
 */
int main(int argc, char * argv[])
{
	/* Parse input arugments */
	int n = atoi(argv[1]);
	int pIndex = atoi(argv[2]);

	/* Create catalog of file names and generate n lists of files */
	int fileCount = 0;
	char ** catalog = NULL;
	IntNode ** newLists = NULL;
	createIndexLists(n, pIndex, &catalog, &fileCount, &newLists);

	/* Save lists */
	saveFileLists(n, pIndex, catalog, newLists);

	/* Clear variables */
	for (int i = 0; i < fileCount; i++)
		free(catalog[i]);
	free(catalog);
	for (int i = 0; i < n; i++)
	{
		IntNode * node = newLists[i];
		while (node != NULL)
		{
			IntNode * next = node->next;
			free(node);
			node = next;
		}
	}
	free(newLists);

	return 0;
}