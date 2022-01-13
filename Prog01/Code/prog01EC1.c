#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[])
{
    // application name without ./
    char app_name_without_prefix[128];
    // copy application name to app_name_without_prefix
    strcpy(app_name_without_prefix, (char*)(argv[0] + 2));
    // if the number of arguments is correct
    if (argc == 3)
    {
        printf("The executable %s was launched with two arguments:\n\tThe first argument is: %s,\n\tThe second argument is: %s.\n", app_name_without_prefix, argv[1], argv[2]);
        return 0;
    }
    // otherwise
    else
    {
        // print usage text
        printf("usage: %s <argument1> <argument2>\n", app_name_without_prefix);
        // if less than 3 arguments are given
        if (argc < 3)
            return 1;
        // otherwise
        else
            return 2;
    }
}
