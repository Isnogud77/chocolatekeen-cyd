// Read-only game files from flash arrays, opened as normal FILE* streams,
// so upstream fopen/fread/fseek code works unchanged.
#include <stdio.h>
#include <strings.h>
#include "keen1_assets.h"

FILE *port_fopen_asset(const char *name) {
    for (int i = 0; i < keen1_filesCount; i++)
        if (!strcasecmp(keen1_files[i].name, name))
            return fmemopen((void *)keen1_files[i].data, keen1_files[i].size, "rb");
    return NULL;
}
