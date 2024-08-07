#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <bsd/string.h>

#include "common.h"


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

    const char* home = getenv("HOME");
    if (home == NULL)
    {
        // HOME environment variable not set
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
