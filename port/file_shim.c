// Read-only game files from flash arrays, opened as normal FILE* streams,
// so upstream fopen/fread/fseek code works unchanged.
#include <stdio.h>
#include <strings.h>
#include "keen1_assets.h"

const AssetFile_T *port_find_asset(const char *name) {
    for (int i = 0; i < keen1_filesCount; i++)
        if (!strcasecmp(keen1_files[i].name, name))
            return &keen1_files[i];
    return NULL;
}

FILE *port_fopen_asset(const char *name) {
    const AssetFile_T *f = port_find_asset(name);
    return f ? fmemopen((void *)f->data, f->size, "rb") : NULL;
}
