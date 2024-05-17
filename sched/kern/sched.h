/* See COPYRIGHT for copyright information. */
#ifndef JOS_KERN_SCHED_H
#define JOS_KERN_SCHED_H
#ifndef JOS_KERNEL
#error "This is a JOS kernel header; user programs should not #include it"
#endif
// This function does not return.
void sched_yield(void) __attribute__((noreturn));

#endif  // !JOS_KERN_SCHED_H

extern struct Env *envs;
void add_new_env_to_history(struct Env *env);
void add_end_of_env_to_history(struct Env *env);
void update_priority(void);