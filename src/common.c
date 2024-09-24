#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include <bsd/string.h>

#include "common.h"


bool StringToBool(char *input)
{
    const int max_length = 6;
    int length = strlen(input);

    if (input == NULL || length > max_length)
        return false;

    char temp[6];
    for (size_t i = 0; input[i]; i++)
    {
        temp[i] = tolower(input[i]);
    }

    temp[length] = '\0';

    return strcmp(temp, "true") == 0 || strcmp(temp, "1") == 0;
}

void StripTrailingWSpace(char *str)
{
    if (str == NULL)
        return;

    // Remove trailing whitespace
    int length = strlen(str);
    while (length > 0 && isspace(str[length - 1]))
    {
        length--;
    }

    str[length] = '\0';
}

void CombinePath(char *dest, size_t dest_size, const char *path1, const char *path2)
{
    if (!dest)
    {
        fprintf(stderr, "Destination buffer is NULL\n");
        return;
    }

    char sep = '/';
    dest[0] = '\0'; // Ensure the destination buffer is empty initially

    if (path1 && *path1)
    {
        strlcpy(dest, path1, dest_size);

        if (path2 && *path2)
        {
            size_t len = strlen(path1);

            if (dest[len - 1] == sep)
            {
                if (*path2 == sep)
                {
                    strlcat(dest, path2 + 1, dest_size);
                }
                else
                {
                    strlcat(dest, path2, dest_size);
                }
            }
            else
            {
                if (*path2 == sep)
                {
                    strlcat(dest, path2, dest_size);
                }
                else
                {
                    if (len + 1 < dest_size)
                    {
                        dest[len] = sep;
                        dest[len + 1] = '\0';
                        strlcat(dest, path2, dest_size);
                    }
                    else
                    {
                        fprintf(stderr, "Destination buffer is too small\n");
                    }
                }
            }
        }
    }
    else if (path2 && *path2)
    {
        strlcpy(dest, path2, dest_size);
    }
}

int ExpandPath(char *expanded_path, const char *path, size_t buffer_size)
{
    if (path[0] != '~')
    {
        // If the path doesn't start with '~', copy it directly
        if (strlcpy(expanded_path, path, buffer_size) >= buffer_size)
        {
            fprintf(stderr, "Buffer size is too small\n");
            return -1;
        }
        return 0;
    }

    const char *home = getenv("HOME");
    if (home == NULL)
    {
        fprintf(stderr, "HOME environment variable not set\n");
        return -1;
    }

    size_t home_len = strlen(home);
    size_t path_len = strlen(path);

    // Ensure the buffer size is sufficient for the expanded path
    if (home_len + path_len >= buffer_size)
    {
        fprintf(stderr, "Buffer size is too small.\n");
        return -1;
    }

    // Copy the home directory and the rest of the path
    if (strlcpy(expanded_path, home, buffer_size) >= buffer_size)
    {
        fprintf(stderr, "Buffer size is too small\n");
        return -1;
    }

    if (strlcat(expanded_path, path + 1, buffer_size) >= buffer_size)
    {
        fprintf(stderr, "Buffer size is too small\n");
        return -1;
    }

    return 0;
}

bool PowerOfTwo(size_t x)
{
    return (x != 0) && ((x & (x - 1)) == 0);
}

bool MultiplesOf8(size_t x)
{
    // Equivalent to x % 8 == 0
    return (x & 7) == 0;
}
