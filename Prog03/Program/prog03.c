#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>

// Create a 2-dimensional dynamic array with the given width and height
char ** create2dArray(int width, int height)
{
	char ** buffer = (char **)malloc(sizeof(char *) * height);
	for (int i = 0; i < height; i++)
		buffer[i] = NULL;//(char *)malloc(sizeof(char) * (width + 1));
	return buffer;
}

// Free the given 2-dimensional array
void delete2dArray(char *** bufferAddress, int width, int height)
{
	char ** buffer = *bufferAddress;
	for (int i = 0; i < height; i++)
		free(buffer[i]);
	free(buffer);
}

// Delete the trailing slash from path string, if any
void deleteTrailingSlash(char * path)
{
	if (path[strlen(path) - 1] == (char)'/')
		path[strlen(path) - 1] = '\0';
}

// Get the last index of '/' excluding the very last one
char * getFileName(char * path)
{
	deleteTrailingSlash(path);
	// search for the last index of slash in path
	int lastSlashIndex = 0;
    for (int i = 0; i < strlen(path); i++)
        if (path[i] == (char)'/')
            lastSlashIndex = i;
    // search for the last index of dot in path
    int lastDotIndex = 0;
    for (int i = 0; i < strlen(path); i++)
        if (path[i] == (char)'.')
            lastDotIndex = i;
    // copy file name only
    char * fileName = (char *)malloc(sizeof(char) * 256);
    strcpy(fileName, path + lastSlashIndex + 1);
    fileName[lastDotIndex - lastSlashIndex - 1] = '\0';
    return fileName;
}

// Read image/pattern file
char ** readFile(char * filePath, int * widthAddress, int * heightAddress)
{
	// Open file
	FILE * file = fopen(filePath, "r");
	if (file == NULL)
	{
		printf("Cannot open file %s errno = %d.\n", filePath, errno);
		return NULL;
	}
	// Read width & height
	int width = 0, height = 0;
	fscanf(file, "%d %d\n", &width, &height);
	// Create a 2d dynamic array
	char ** data = create2dArray(width, height);
	// Read line by line
	size_t len = 0;
	ssize_t read = 0;
	for (int i = 0; i < height; i++)
	{
		read = getline(&data[i], &len, file);
		if (read > width)
			data[i][width] = '\0';
	}
	// Close file
	fclose(file);
	// Set width and height
	*widthAddress = width;
	*heightAddress = height;
	// Return data
	return data;
}

int main(int argc, char * argv[])
{
	// Check the number of arguments
	if (argc != 4)
	{
		printf("usage: ./fibo2 [F1 F2 n]+, with F2>F1>0 and n>0.\n");
		return 1;
	}
	// Remove double quotes if necessary
	for (int i = 0; i < argc; i++)
	{
		if (argv[i][0] == '\"')
			argv[i]++;
		if (argv[i][strlen(argv[i]) - 1] == '\"')
			argv[i][strlen(argv[i]) - 1] = '\0';
	}

	// Read pattern file
	int pWidth = 0, pHeight = 0;
	char ** pattern = readFile(argv[1], &pWidth, &pHeight);
	if (pattern == NULL)
		return -1;

	// Split pattern file name
	char * pFileName = getFileName(argv[1]);
	
	// Get output file name
	char oFilePath[256];
	deleteTrailingSlash(argv[3]);
	strcpy(oFilePath, argv[3]);
	strcat(oFilePath + strlen(argv[3]), "/");
	strcat(oFilePath + strlen(argv[3]) + 1, pFileName);
	strcat(oFilePath + strlen(argv[3]) + 1 + strlen(pFileName), "_Matches.txt");
	free(pFileName);

	// Open output file
	FILE * oFile = fopen(oFilePath, "w");
	if (oFile == NULL)
	{
		printf("Cannot open file %s.\n", oFilePath);
		return -1;
	}

	// Open input directory
	deleteTrailingSlash(argv[2]);
	DIR * iDir = opendir(argv[2]);
	if (iDir == NULL)
	{
		printf("Cannot open input directory %s.\n", argv[2]);
		return -2;
	}

	// Read directory structure
	struct dirent * ent;
	while ((ent = readdir(iDir)) != NULL)
	{
		char * fileName = ent->d_name;
		int length = strlen(ent->d_name);
		// Check file type
		if (length > 4 &&
			fileName[length - 4] == (char)'.' &&
			fileName[length - 3] == (char)'i' &&
			fileName[length - 2] == (char)'m' &&
			fileName[length - 1] == (char)'g')
		{
			// Get input file path
			char iFilePath[256];
			strcpy(iFilePath, argv[2]);
			strcat(iFilePath + strlen(argv[2]), "/");
			strcat(iFilePath + strlen(argv[2]) + 1, fileName);

			// Read input file
			int iWidth = 0, iHeight = 0;
			char ** image = readFile(iFilePath, &iWidth, &iHeight);
			if (image == NULL)
				continue;

			// Count pattern
			int patternCount = 0;
			for (int i = 0; i <= iHeight - pHeight; i++)
			{
				for (int j = 0; j <= iWidth - pWidth; j++)
				{
					int patternFound = 1;
					for (int k = 0; k < pHeight; k++)
					{
						char temp[256];
						strcpy(temp, image[i + k] + j);
						temp[pWidth] = '\0';
						if (strcmp(temp, pattern[k]) != 0)
						{
							patternFound = 0;
							break;
						}
					}
					if (patternFound == 1)
						patternCount++;
				}
			}

			// Output input file name
			fprintf(oFile, "%s\n", fileName);
			// If pattern is found for at least once
			if (patternCount > 0)
			{
				// Output pattern count
				fprintf(oFile, "\t%d", patternCount);
				for (int i = 0; i <= iHeight - pHeight; i++)
				{
					for (int j = 0; j <= iWidth - pWidth; j++)
					{
						int patternFound = 1;
						for (int k = 0; k < pHeight; k++)
						{
							char temp[256];
							strcpy(temp, image[i + k] + j);
							temp[pWidth] = '\0';
							if (strcmp(temp, pattern[k]) != 0)
							{
								patternFound = 0;
								break;
							}
						}
						if (patternFound == 1)
							fprintf(oFile, " %d %d", i, j);
					}
				}
				fprintf(oFile, "\n");
			}

			// Delete image data
			delete2dArray(&image, iWidth, iHeight);
		}
	}
	fclose(oFile);

	// Delete pattern data
	delete2dArray(&pattern, pWidth, pHeight);

	return 0;
}
