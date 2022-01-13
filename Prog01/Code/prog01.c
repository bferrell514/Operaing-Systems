#include <stdio.h>

int main(int argc, char* argv[])
{
    // if the number of arguments is correct
    if (argc == 3)
    {
        printf("The executable %s was launched with two arguments:\n\tThe first argument is: %s,\n\tThe second argument is: %s.\n", argv[0], argv[1], argv[2]);
        return 0;
    }
    // otherwise
    else
    {
        // print usage text
        printf("usage: %s <argument1> <argument2>\n", argv[0]);
        // if less than 3 arguments are given
        if (argc < 3)
            return 1;
        // otherwise
        else
            return 2;
    }
}
