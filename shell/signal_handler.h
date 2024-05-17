#ifndef SIGNAL_HANDLER
#define SIGNAL_HANDLER
#include "defs.h"
#include "runcmd.h"

void handler(int signum);
void init_handler(void);
void free_handler(void);

#endif  // SIGNAL_HANDLER