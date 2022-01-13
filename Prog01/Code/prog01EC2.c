#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[])
{
    // application name only
    char app_name[128];
    // the index of last slash in argv[0]
    int last_slash_index = 0;
    // search for the last index of slash in argv[0]
    for (int i = 0; i < strlen(argv[0]); i++)
        if (argv[0][i] == (char)'/')
            last_slash_index = i;
    // copy application name to app_name
    strcpy(app_name, (char*)(argv[0] + last_slash_index + 1));
    // if the number of arguments is correct
    if (argc == 3)
    {
        printf("The executable %s was launched with two arguments:\n\tThe first argument is: %s,\n\tThe second argument is: %s.\n", app_name, argv[1], argv[2]);
        return 0;
    }
    // otherwise
    else
    {
        // print usage text
        printf("usage: %s <argument1> <argument2>\n", app_name);
        // if less than 3 arguments are given
        if (argc < 3)
            return 1;
        // otherwise
        else
            return 2;
    }
}
