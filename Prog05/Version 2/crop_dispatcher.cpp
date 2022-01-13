#include <iostream>
#include <fstream>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

/**
 * @file crop_dispatcher.cpp
 * @author Baheem Ferrell
 * @date 10 Nov 2020
 */

using namespace std;

#define MAX_ARGS 7

int main(int argc, char * argv[])
{
	string line;
	// Wait for a command
	while (getline(cin, line))
	{
		// Check end condition
		if (line.find("end") == 0)
			return 0;
		// Read all arguments
		string lines[MAX_ARGS];
		lines[0] = line;
		for (int i = 1; i < MAX_ARGS; i++)
			getline(cin, lines[i]);
		// Prepare arguments
		char * arguments[MAX_ARGS + 1];
		for (int i = 0; i < MAX_ARGS; i++)
		{
			arguments[i] = (char*)lines[i].c_str();
		}
		arguments[MAX_ARGS] = NULL;
		// Execute
		pid_t child = fork();
		// Executed in child
		if (child == 0)
		{
			int status = execvp(arguments[0], arguments);
			if (status == -1)
			{
				cout << "Error : " << line << endl;
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
					cout << "Error : " << line << endl;
					break;
				}
				// If successful
				if (newPid == child)
				{
					if (childStatus == 0)
						cout << "Success : " << line << endl;
					else
						cout << "Error : " << line << endl;
				}
			}
			while (newPid != child);
		}
	}
	return 0;
}