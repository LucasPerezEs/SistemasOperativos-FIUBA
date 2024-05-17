#ifndef JOS_KERN_HISTORY_H
#define JOS_KERN_HISTORY_H
#ifndef JOS_KERNEL
#error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <kern/env.h>
#include <inc/string.h>

// El historial muestra: la creacion de env, cuando el sched selecciona una, la finalizacion
#define HISTORY_LEN 1000
static char history[HISTORY_LEN][100];


void update_history_ind();
void add_run_env_to_history(struct Env *env);
void add_new_env_to_history(struct Env *env);
void add_end_of_env_to_history(struct Env *env);
void print_history(void);

#endif /* !JOS_KERN_HISTORY_H */
