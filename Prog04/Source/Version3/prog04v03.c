#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/**
 * @file prog04v03.c
 * @author Baheem Ferrell
 * @date 22 Oct 2020
 */

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
 * @brief Saves a part of catalog to a file.
 * This function saves file paths in the range of [iInf, iSup) from the given catalog.
 * @param catalog Catalog of file paths.
 * @param pIndex Distributor index.
 * @param iInf Lower bound for index.
 * @param iSup Upper bound for index.
 */
void saveDistributorCatalog(char ** catalog, int pIndex, int iInf, int iSup)
{
	// Open file
	char fileName[20];
	sprintf(fileName, "%d.catalog", pIndex);
	FILE * file = fopen(fileName, "w");
	if (file == NULL)
		return;
	// Print file count
	fprintf(file, "%d\n", iSup - iInf);
	// Write file path
	for (int j = iInf; j < iSup; j++)
		fprintf(file, "%s\n", catalog[j]);
	// Close file
	fclose(file);
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
		printf("usage: ./prog04v03 number_of_processes input_dir_path output_file_path.\n");
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
	// Split catalog and print
	for (int i = 0; i < n; i++)
		saveDistributorCatalog(catalog, i, iInf[i], iSup[i]);
	// Create n distributors
	pid_t * children = (pid_t *)malloc(sizeof(pid_t) * n);
	char distributorPath[512];
	char argument[128];
	sprintf(distributorPath, "%s_distribute", argv[0]);
	for (int pIndex = 0; pIndex < n; pIndex++)
	{
		children[pIndex] = fork();
		// Executed in child process
		if (children[pIndex] == 0)
		{
			char * arguments[3];
			arguments[0] = distributorPath;
			arguments[1] = argv[1];
			sprintf(argument, "%d", pIndex);
			arguments[2] = argument;
			arguments[3] = NULL;
			// Execute distributor process
			execvp(distributorPath, arguments);
			// Exit
			return 0;
		}
	}
	// Wait for all distributors to finish
	waitForChildren(n, children);

	/* Process files */
	// Create n processors
	char processorPath[512];
	sprintf(processorPath, "%s_process", argv[0]);
	for (int pIndex = 0; pIndex < n; pIndex++)
	{
		children[pIndex] = fork();
		// Executed in child process
		if (children[pIndex] == 0)
		{
			char * arguments[4];
			arguments[0] = processorPath;
			arguments[1] = argv[1];
			sprintf(argument, "%d", pIndex);
			arguments[2] = argument;
			arguments[3] = NULL;
			// Execute distributor process
			execvp(processorPath, arguments);
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
		char fileName[20];
		sprintf(fileName, "%d.catalog", i);
		remove(fileName);
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
