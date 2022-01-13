#include <iostream>
#include <fstream>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

/**
 * @file dispatcher_v2.cpp
 * @author Baheem Ferrell
 * @date 10 Nov 2020
 */

using namespace std;

/**
 * @def Maximum number of arguments to a job
 */
#define MAX_ARGS 8

/**
 * @brief Remove a trailing slash from path string, if necessary.
 *
 * This function gets a char * value and checks if it ends with a slash.
 * If it ends with a slash, it removes the slash and returns it.
 * Otherwise, it returns the original string.
 * @param path Path string to be checked/modified.
 * @return Returns a string which does not end with a slash.
 */
char * removeTrailingSlash(char * path)
{
	if (path[strlen(path) - 1] == (char)'/')
		path[strlen(path) - 1] = '\0';
	return path;
}

/**
 * @brief Combine the given path and file name.
 *
 * This function gets 2 char * values for path and file name respectively
 * and returns the combined absolute path string
 * @param dirPath Directory path
 * @param fileName File name
 * @return Returns a string which is an absolute path to the file
 */
char * combinePath(char * dirPath, char * fileName)
{
	int length = strlen(dirPath) + strlen(fileName) + 2;
	char * path = new char[length];
	memset(path, 0, length);
	sprintf(path, "%s/%s", dirPath, fileName);
	return path;
}

/**
 * @brief Process given job file.
 *
 * This function gets an array of char * values representing
 * the path to the job file, the path to the image directory,
 * the path to the output directory and the path to the utilities respectively
 * and process the job file according to the project description for version 2.
 * @param argv An array of 4 paths.
 * @return Returns the processing result
 */
int process2(char * argv[])
{
	// Remove double quotes if necessary
	for (int i = 0; i < 4; i++)
	{
		if (argv[i][0] == '\"')
			argv[i]++;
		if (argv[i][strlen(argv[i]) - 1] == '\"')
			argv[i][strlen(argv[i]) - 1] = '\0';
	}

	// Get paths to job file, input directory and output directory
	char * jobFilePath = argv[0];
	char * inputDirPath = removeTrailingSlash(argv[1]);
	char * outputDirPath = removeTrailingSlash(argv[2]);
	char * exePath = removeTrailingSlash(argv[3]);
	char logFileName[] = "Logs/dispatcher_v2.log";
	char * logPath = combinePath(outputDirPath, logFileName);

	// Open job file and read line by line
	ifstream inputFile(jobFilePath);
	if (inputFile.is_open())
	{
		// Open log file and write logs
		ofstream outputFile(logPath, std::ofstream::app);
		if (outputFile.is_open())
		{
			string line;
			string lineClone;
			while (getline(inputFile, line, '\n'))
			{
				// If the line is empty, continue to the next line
				if (line.length() == 0)
					continue;

				// Clone the line string
				lineClone = line;

				// Parse command
				char * arguments[MAX_ARGS];
				memset(arguments, 0, sizeof(char *) * MAX_ARGS);
				int index = 0;
				arguments[index++] = strtok((char *)lineClone.c_str(), " \n\0");
				while (true)
				{
					arguments[index] = strtok(NULL, " \0\0");
					if (arguments[index] == NULL)
						break;
					index++;
				}

				// Check end command
				if (strcmp(arguments[0], "end") == 0)
				{
					// End
					outputFile << "Success : " << line << endl;
					return 1;
				}

				// flipH/flipV/gray command
				if (strcmp(arguments[0], "flipH") == 0 || 
					strcmp(arguments[0], "flipV") == 0 ||
					strcmp(arguments[0], "gray") == 0)
				{
					// Check validity
					if (arguments[1] == NULL || arguments[2] != NULL)
					{
						outputFile << "Error : " << line << endl;
						continue;
					}
					// Set arguments
					arguments[2] = outputDirPath;
					arguments[1] = combinePath(inputDirPath, arguments[1]);
				}
				// crop command
				else if (strcmp(arguments[0], "crop") == 0)
				{
					// Check validity
					if (arguments[5] == NULL || arguments[6] != NULL)
					{
						outputFile << "Error : " << line << endl;
						continue;
					}
					// Set arguments
					for (int i = 5; i >= 2; i--)
						arguments[i + 1] = arguments[i];
					arguments[2] = outputDirPath;
					arguments[1] = combinePath(inputDirPath, arguments[1]);
				}
				// rotate command
				else if (strcmp(arguments[0], "rotate") == 0)
				{
					// Check validity
					if (arguments[2] == NULL || arguments[3] != NULL)
					{
						outputFile << "Error : " << line << endl;
						continue;
					}
					// Set arguments
					arguments[3] = outputDirPath;
					arguments[2] = combinePath(inputDirPath, arguments[2]);
				}
				arguments[0] = combinePath(exePath, arguments[0]);

				// Call appropriate utility
				pid_t child = fork();
				// Executed in child
				if (child == 0)
				{
					int status = execvp(arguments[0], arguments);
					if (status == -1)
					{
						outputFile << "Error : " << line << endl;
						return errno;
					}
					return 0;
				}
				// Executed in parent
				else
				{
					// Wait for the child to finish
					pid_t newPid;
					do
					{
						int childStatus;
						newPid = wait(&childStatus);
						// if an error has occured
						if (newPid == -1)
						{
							outputFile << "Error : " << line << endl;
							break;
						}
						// If successful
						if (newPid == child)
						{
							if (childStatus == 0)
								outputFile << "Success : " << line << endl;
							else
								outputFile << "Error : " << line << endl;
						}
					}
					while (newPid != child);
				}
			}
			outputFile.flush();
			outputFile.close();
		}
		else
		{
			cout << "Cannot open log file." << endl;
		}
		inputFile.close();
	}
	else
	{
		cout << "Cannot open job file." << endl;
		return -1;
	}
	return 0;
}

int main(int argc, char * argv[])
{
	// Reuse variables
	argc = 4;
	argv = new char * [4];
	// Keep reading one line at a time from stdin
	string line;
	while (getline(cin, line))
	{
		// Parse line into 4 arguments
		int index1 = 0;
		int index2 = 0;
		for (int i = 0; i < 4; i++)
		{
			index2 = line.find("\"", index1 + 1);
			argv[i] = new char[index2 - index1];
			strcpy(argv[i], line.substr(index1 + 1, index2 - index1 - 1).c_str());
			index1 = index2 + 2;
		}

		// Process
		int result = process2(argv);

		// If end condition found
		if (result == 1)
			break;

		// Otherwise
		cout << "Processed " << argv[0] << endl;
	}
	// Print result
	cout << "End " << argv[0] << endl;
	return 0;
}