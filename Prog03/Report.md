# Prog03  
## Implmentation of the pattern search  
### Data structure  
2-dimensional dynamic arrays are used to store patterns and images.  
These arrays are automatically created and allocated once the image/pattern dimension is read from file.  
These 2-dimensional arrays store pointers to dynamic arrays each of which is a 1-dimensional dynamic array.  
These arrays are deallocated manually once these variables are no longer in use.  

```
char ** create2dArray(int width, int height);
void delete2dArray(char *** bufferAddress, int width, int height);
```
### Alogrithm  
An image file and a pattern file are read into 2 2-dimensional arrays.  
Then by searching every possible locations (row, column) that a match can happen (taking the shape of the pattern into account) one by one, the number of all possible matches are found.  
If this number is greater than 0, it is printed to the output file and the above process is repeated only to print out the locations where match occurs.  
## Possible difficulties  
In order to manipulate file names and absolute paths, care must be taken to correctly ignore/prepend/append quotes, slashes, end-of-line characters and NULL characters.  
For example, in order to handle user inputs where path contains white spaces, the input is first wrapped with double quotes, which must be removed to get absolute paths to image files.  
In addition, when detecting end-of-line characters in shell script, searching for "\r\n" returned none for Windows-style text files, because shell interpreter already escaped the trailing "\n" when reading a file line by line.  
## Possible limitations
Currently, in C program, file paths longer than 256 characters cannot be handled, because when concatenating absolute paths, a char array of length 256 is used.  
Changing this to dynamic array may solve this issue.  