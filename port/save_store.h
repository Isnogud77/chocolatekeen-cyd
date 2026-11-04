// Upstream's writable files (saves, scores) as blobs in the port store.
#ifndef PORT_SAVE_STORE_H
#define PORT_SAVE_STORE_H

#include <stdio.h>

#define SAVE_STORE_MAX 2048

// "rb": FILE over a copy of the stored blob (NULL if missing).
// "wb": FILE that commits its bytes to the store on fclose (nothing before).
// Only SAVED1.* and SCORES.* are served; everything else returns NULL.
FILE *port_save_fopen(const char *name, const char *mode);

#endif
