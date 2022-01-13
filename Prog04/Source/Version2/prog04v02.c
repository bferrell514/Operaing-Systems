#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/**
 * @file prog04v02.c
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
 * @brief A data container to store an integer value and a string.
 *
 * FileLine contains an integer value that represents the line index and
 * a char * value to store a line of text from input file.
 */
typedef struct FileLine
{
	int index;
	char * line;
} FileLine;

/**
 * @brief Compare function for qsort().
 * This function determines whether the line referenced by a comes before
 * the line referenced by b and returns the result as an integer value.
 * @param a Pointer to a FileLine data a
 * @param b Pointer to a FileLine data b
 * @return Returns -1 if a comes before b,
 * 1 if a comes after b,
 * 0 otherwise.
 */
int compareFunction(const void * a, const void * b)
{
	return ((*(FileLine *)a).index - (*(FileLine *)b).index);
}

/**
 * @brief Append a trailing slash to path string, if necessary.
 *
 * This function gets a char * value and checks if it correctly ends with a slash.
 * If it ends with a slash, it clones the input parameter and returns it.
 * Otherwise, it creates a new char * value, clones the input, appends a slash
 * and returns it.
 * @param path Path string to be checked/modified.
 * @return Returns a newly created char * value which ends with a slash.
 */
char * appendTrailingSlash(char * path)
{
	char * result = NULL;
	if (path[strlen(path) - 1] != (char)'/')
	{
		result = (char *)malloc(strlen(path) + 2);
		strcpy(result, path);
		result[strlen(path)] = (char)'/';
	}
	else
	{
		result = (char *)malloc(strlen(path) + 1);
		strcpy(result, path);
	}
	return result;
}

/**
 * @brief Appends .c extension to path string, if necessary.
 * This function gets a char * value and checks if it correctly ends with ".c".
 * If it ends with ".c", it clones the input parameter and returns it.
 * Otherwise, it creates a new char * value, clones the input, appends ".c"
 * and returns it.
 * @param path Path string to be checked/modified.
 * @return Returns a newly created char * value which ends with ".c".
 */
char * appendTrailingExtension(char * path)
{
	char * result = NULL;
	if (path[strlen(path) - 2] != (char)'.' ||
		path[strlen(path) - 1] != (char)'c')
	{
		result = (char *)malloc(strlen(path) + 3);
		strcpy(result, path);
		result[strlen(path)] = (char)'.';
		result[strlen(path) + 1] = (char)'c';
	}
	else
	{
		result = (char *)malloc(strlen(path) + 1);
		strcpy(result, path);
	}
	return result;
}

/**
 * @brief Creates and returns the list of input files and the number of files.
 * This function reads an entire directory structure and creates a list of
 * data files in that directory.
 * It returns the list along with the number of files in the list.
 * @param inDirPath Directory path to be read.
 * @param fileListAddr Pointer to the list of data files (output).
 * @param fileCountAddr Pointer to the number of data files (output).
 */
void createFileList(char * inDirPath, char *** fileListAddr, int * fileCountAddr)
{
	// Open directory
	DIR * iDir = opendir(inDirPath);
	if (iDir == NULL)
	{
		printf("Cannot open input directory %s.\n", inDirPath);
		return;
	}
	// Create file list
	int count = 0;
	int capacity = 1;
	char ** fileList = (char **)malloc(sizeof(char *));
	fileList[0] = NULL;
	struct dirent * ent = NULL;
	while ((ent = readdir(iDir)) != NULL)
	{
		char * fileName = ent->d_name;
		int length = strlen(ent->d_name);
		// Check file type
		if (ent->d_type == DT_REG)
		{
			// Get input file path
			char * iFilePath = (char *)malloc(strlen(inDirPath) + strlen(fileName) + 1);
			strcpy(iFilePath, inDirPath);
			strcpy(iFilePath + strlen(inDirPath), fileName);
			// Increase the array size if necessary
			if (count == capacity)
			{
				capacity *= 2;
				char ** temp = (char **)malloc(sizeof(char *) * capacity);
				memset(temp, 0, sizeof(char *) * capacity);
				memcpy(temp, fileList, sizeof(char *) * count);
				free(fileList);
				fileList = temp;
			}
			// Append the file name to the array
			fileList[count++] = iFilePath;
		}
	}
	// Output
	*fileListAddr = fileList;
	*fileCountAddr = count;
}

/**
 * @brief Creates and returns lists for lower & upper bounds of file index respectively.
 * This function creates and returns 2 lists of lower bounds and upper bounds of file index
 * to be used in processing by n different workers.
 * @param fileCount The number of files to be split into n workers.
 * @param n The number of workers.
 * @param iInfAddr Pointer to the list of lower bounds of file index (output).
 * @param iSupAddr Pointer to the list of upper bounds of file index (output).
 */
void createBounds(int fileCount, int n, int ** iInfAddr, int ** iSupAddr)
{
	// Split fileCount to n equal parts (more or less)
	int * iInf = (int *)malloc(sizeof(int) * n);
	int * iSup = (int *)malloc(sizeof(int) * n);
	for (int i = 0; i < n; i++)
	{
		iInf[i] = fileCount / n * i;
		iSup[i] = fileCount / n * (i + 1);
		if (iSup[i] > fileCount)
			iSup[i] = fileCount;
	}
	if (iSup[n - 1] < fileCount)
		iSup[n - 1] = fileCount;
	// Output
	*iInfAddr = iInf;
	*iSupAddr = iSup;
}

/**
 * @brief Creates and returns file lists for different workers respectively.
 * This function creates and returns n lists of file index to be
 * to be used in processing by n different workers.
 * @param n The number of workers.
 * @param catalog List of file paths.
 * @param iInf Lower bound for file index
 * @param iSup Upper bound for file index
 * @param indexListsAddr Pointer to n lists of file index (output).
 */
void createIndexLists(int n, int index, char ** catalog, int iInf, int iSup, IntNode *** indexListsAddr)
{
	// Initialize
	IntNode ** indexListHeads = (IntNode **)malloc(sizeof(IntNode *) * n);
	IntNode ** indexListTails = (IntNode **)malloc(sizeof(IntNode *) * n);
	for (int i = 0; i < n; i++)
	{
		indexListHeads[i] = NULL;
		indexListTails[i] = NULL;
	}
	// Process files
	for (int i = iInf; i < iSup; i++)
	{
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
 * @brief Read n lists of files to create a list of files to be processed
 * This function reads i-th list of every n lists generated by n distributors
 * and create a list of files to be processed by this process.
 * @param n Number of processes.
 * @param pIndex Process index for this process.
 * @param fileNamesAddr Pointer to the list of file names (output).
 * @param fileCountAddr Pointer to the number of files.
 */
void readFileLists(int n, int pIndex, char *** fileNamesAddr, int * fileCountAddr)
{
	char ** fileNames = NULL;
	int fileCount = 0;
	// Read index files
	for (int i = 0; i < n; i++)
	{
		// Open file
		char fileName[10];
		sprintf(fileName, "%d.list", i);
		FILE * inFile = fopen(fileName, "r");
		// Read n lists
		for (int j = 0; j < n; j++)
		{
			// Get count
			size_t len = 0;
			ssize_t read = 0;
			char * line = NULL;
			read = getline(&line, &len, inFile);
			if (line[strlen(line) - 1] == (char)'\0')
				line[strlen(line) - 1] = (char)'\0';
			int count = atoi(line);
			free(line);
			line = NULL;
			// Increase file name list, if necessary
			if (j == pIndex && count > 0)
			{
				if (fileCount == 0)
				{
					fileNames = (char **)malloc(sizeof(char *) * (fileCount + count));
				}
				else
				{
					char ** newFileNames = (char **)malloc(sizeof(char *) * (fileCount + count));
					memcpy(newFileNames, fileNames, sizeof(char *) * fileCount);
					free(fileNames);
					fileNames = newFileNames;
				}
			}
			// Get file paths
			for (int k = 0; k < count; k++)
			{
				len = 0;
				read = 0;
				read = getline(&line, &len, inFile);
				if (j == pIndex)
				{
					if (line[strlen(line) - 1] == (char)'\n')
						line[strlen(line) - 1] = (char)'\0';
					fileNames[fileCount++] = line;
				}
				// Otherwise, delete
				else
					free(line);
				line = NULL;
			}
		}
	}
	// Output
	*fileNamesAddr = fileNames;
	*fileCountAddr = fileCount;
}

/**
 * @brief Read files and returns a sorted string.
 * This function reads files specified in fileNames
 * and sorts them according to line numbers and returns the 
 * concatenated result as a char * value.
 * @param fileNames List of file paths.
 * @param fileCount Number of files in the list.
 * @return Ordered and concatenated string of file lines.
 */
char * processFiles(char ** fileNames, int fileCount)
{
	// Create Line array
	FileLine * lines = (FileLine *)malloc(sizeof(FileLine) * fileCount);
	memset(lines, 0, sizeof(FileLine) * fileCount);
	// Populate lines
	int length = 0;
	for (int i = 0; i < fileCount; i++)
	{
		// Open file
		FILE * file = fopen(fileNames[i], "r"); 
		if (file == NULL)
		{
			printf("Cannot open input file for processing %s.\n", fileNames[i]);
			return NULL;
		}
		// Read line index
		int lineIndex = 0;
		fscanf(file, "%d %d ", &lineIndex, &lineIndex);
		lines[i].index = lineIndex;
		// Read line
		size_t len = 0;
		ssize_t read = 0;
		char * line = NULL;
		read = getline(&line, &len, file);
		if (read < 0 || line == NULL || len < 1)
		{
			length += 1;
			lines[i].line = NULL;
			free(line);
		}
		else
		{
			if (line[strlen(line) - 1] == (char)'\n')
				line[strlen(line) - 1] = (char)'\0';
			lines[i].line = line;
			length += strlen(line) + 1;
		}
		fclose(file);
	}
	// Sort
	qsort(lines, fileCount, sizeof(FileLine), compareFunction);
	// Merge lines
	char * result = (char *)malloc(length + 1);
	length = 0;
	for (int i = 0; i < fileCount; i++)
	{
		if (lines[i].line != NULL)
		{
			strcpy(result + length, lines[i].line);
			length += strlen(lines[i].line);
		}
		result[length++] = (char)'\n';
		result[length] = (char)'\0';
		free(lines[i].line);
		lines[i].line = NULL;
	}
	free(lines);
	// Output
	return result;
}

/**
 * @brief Save processed result as a text file.
 * This function opens a file (i.result) and prints the text
 * referenced by result value.
 * @param pIndex Process index.
 * @param result Result text to print.
 */
void saveProcessedResult(int pIndex, char * result)
{
	// Open output file
	char fileName[10];
	sprintf(fileName, "%d.result", pIndex);
	FILE * outFile = fopen(fileName, "w");
	if (outFile == NULL)
	{
		printf("Cannot write result file %s.\n", fileName);
		return;
	}
	fprintf(outFile, "%s", result);
	// Close file
	fclose(outFile);
}

/**
 * @brief Read a text file generated by a processor.
 * This functions reads a text file generated by a processor and
 * returns the result.
 * @param pIndex Distributor index.
 * @return Text read from file.
 */
char * readProcessedResult(int pIndex)
{
	// Open input file
	char fileName[10];
	sprintf(fileName, "%d.result", pIndex);
	FILE * inFile = fopen(fileName, "r");
	if (inFile == NULL)
	{
		printf("Cannot read result file %s.\n", fileName);
		return NULL;
	}
	// Read line by line
	char * result = NULL;
	char line[512];
	while (fgets(line, sizeof(line), inFile))
	{
		if (result == NULL)
		{
			int len = strlen(line);
			result = (char *)malloc(len + 1);
			strcpy(result, line);
			result[len] = (char)'\0';
		}
		else
		{
			int len = strlen(result) + strlen(line);
			char * newResult = (char *)malloc(len + 1);
			strcpy(newResult, result);
			strcpy(newResult + strlen(result), line);
			free(result);
			result = newResult;
			result[len] = (char)'\0';
		}
	}
	// Close file
	fclose(inFile);
	// Return
	return result;
}

/**
 * @brief Wait until all child processes are finished.
 * This function blocks the calling process until the 
 * specified n children are finished.
 * @param n Number of processes.
 * @children Processor IDs of children
 */
void waitForChildren(int n, pid_t * children)
{
	// Wait for n children to finish
	for (int pIndex = 0; pIndex < n; pIndex++)
	{
		// Wait for a child to finish
		pid_t newPid;
		do
		{
			int childStatus;
			newPid = wait(&childStatus);
			// if an error has occured
			if (newPid == -1)
				break;
		}
		while (newPid != children[pIndex]);
	}
}

/**
 * @brief Entry point.
 * This function performs indexing and processing of the given data directory
 * and prints the result to the given file.
 * @param argc Number of command line arguments
 * @param argv An array of command line arguments.
 * @return Returns 0, if successful, non-zero otherwise.
 */
int main(int argc, char * argv[])
{
	/* Check the number of arguments */
	if (argc != 4)
	{
		printf("usage: ./prog04v02 number_of_processes input_dir_path output_file_path.\n");
		return 1;
	}

	/* Remove double quotes if necessary */
	for (int i = 0; i < argc; i++)
	{
		if (argv[i][0] == '\"')
			argv[i]++;
		if (argv[i][strlen(argv[i]) - 1] == '\"')
			argv[i][strlen(argv[i]) - 1] = '\0';
	}

	/* Parse input arugments */
	// Get number of processes
	int n = atoi(argv[1]);
	// Get input dir path
	char * inDirPath = appendTrailingSlash(argv[2]);
	// Get output file path
	char * outFilePath = appendTrailingExtension(argv[3]);

	/* Create a catalog of all input file names */
	char ** catalog = NULL;
	int fileCount = 0;
	createFileList(inDirPath, &catalog, &fileCount);

	/* Distribute files */
	// Create index bounds
	int * iInf = NULL;
	int * iSup = NULL;
	createBounds(fileCount, n, &iInf, &iSup);
	// Create n distributors
	pid_t * children = (pid_t *)malloc(sizeof(pid_t) * n);
	for (int pIndex = 0; pIndex < n; pIndex++)
	{
		children[pIndex] = fork();
		// Executed in child process
		if (children[pIndex] == 0)
		{
			// Create lists
			IntNode ** newLists = NULL;
			createIndexLists(n, pIndex, catalog, iInf[pIndex], iSup[pIndex], &newLists);
			// Save lists
			saveFileLists(n, pIndex, catalog, newLists);
			// Clean up
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
			free(children);
			free(iInf);
			free(iSup);
			for (int i = 0; i < fileCount; i++)
				free(catalog[i]);
			free(catalog);
			free(inDirPath);
			free(outFilePath);
			// Exit
			return 0;
		}
	}
	// Wait for all distributors to finish
	waitForChildren(n, children);

	/* Process files */
	// Create n processors
	for (int pIndex = 0; pIndex < n; pIndex++)
	{
		children[pIndex] = fork();
		// Executed in child process
		if (children[pIndex] == 0)
		{
			// Read file lists
			char ** fileNames = NULL;
			int fileCount = 0;
			readFileLists(n, pIndex, &fileNames, &fileCount);
			// Process source files
			char * result = processFiles(fileNames, fileCount);
			// Output result
			saveProcessedResult(pIndex, result);
			// Clean up
			free(children);
			free(iInf);
			free(iSup);
			for (int i = 0; i < fileCount; i++)
				free(catalog[i]);
			free(catalog);
			free(inDirPath);
			free(outFilePath);
			// Exit
			return 0;
		}
	}
	// Wait for all processors to finish
	waitForChildren(n, children);

	/* Concatenate results */
	char * results = NULL;
	for (int pIndex = 0; pIndex < n; pIndex++)
	{
		char * newResults = readProcessedResult(pIndex);
		if (results == NULL)
			results = newResults;
		else
		{
			char * temp = (char *)malloc(strlen(results) + strlen(newResults) + 1);
			strcpy(temp, results);
			strcpy(temp + strlen(results), newResults);
			free(results);
			free(newResults);
			results = temp;
		}
	}

	/* Delete temporary files */
	for (int i = 0; i < n; i++)
	{
		char fileName[10];
		sprintf(fileName, "%d.list", i);
		remove(fileName);
		sprintf(fileName, "%d.result", i);
		remove(fileName);
	}

	/* Output result */
	FILE * outFile = fopen(outFilePath, "w");
	if (outFile == NULL)
	{
		printf("Cannot write output file %s.\n", outFilePath);
		return -1;
	}
	fprintf(outFile, "%s\n", results);
	fclose(outFile);

	/* Clean up */
	free(results);
	free(children);
	free(iInf);
	free(iSup);
	for (int i = 0; i < fileCount; i++)
		free(catalog[i]);
	free(catalog);
	free(inDirPath);
	free(outFilePath);

	return 0;
}
