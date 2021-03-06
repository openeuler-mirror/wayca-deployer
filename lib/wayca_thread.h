#ifndef _WAYCA_THREAD_H
#define _WAYCA_THREAD_H

#define _GNU_SOURCE
#include <sched.h>
#include <syscall.h>
#include <wayca-scheduler.h>
#include "common.h"
#include "bitmap.h"

#define thread_sched_setaffinity(pid, size, cpuset) \
  syscall(__NR_sched_setaffinity, (pid_t)pid, (size_t)size, (void *)cpuset)
#define thread_sched_getaffinity(pid, size, cpuset) \
  syscall(__NR_sched_getaffinity, (pid_t)pid, (size_t)size, (void *)cpuset)
#define thread_sched_gettid(void)	\
  syscall(__NR_gettid)

/* CPU set of all the cpus in the system */
extern cpu_set_t total_cpu_set;
/* Load Array of each cpu, length is cores_in_total() */
extern long long *wayca_cpu_loads;
extern pthread_mutex_t wayca_cpu_loads_mutex;

struct wayca_thread {
	/* Wayca thread id which is identity to this thread */
	wayca_sc_thread_t id;
	/* pid_t of this wayca thread */
	pid_t pid;
	/* Wayca thread attribute */
	wayca_sc_thread_attr_t attribute;
	size_t target_pos;
	cpu_set_t cur_set;
	cpu_set_t allowed_set;
	/* Siblings of this wayca thread in the same group, NULL terminated */
	struct wayca_thread *siblings;
	/* Wayca group this thread directly belongs to */
	struct wayca_sc_group *group;

	/* Internal pthread_t for this thread */
	pthread_t thread;
	/* The routine this thread going to perform */
	void *(*start_routine)(void *);
	/* The args for the routine */
	void *arg;
	/* Is the routine started ? */
	bool start;
};

struct wayca_sc_group {
	/* Wayca group id which is identity to this group */
	wayca_sc_group_t id;
	/* The threads list in this group */
	struct wayca_thread *threads;
	/* The number of the threads in this group */
	int nr_threads;
	/* The sibling groups, NULL terminated */
	struct wayca_sc_group *siblings;
	/* The father of this group, NULL means the toppest level */
	struct wayca_sc_group *father;
	/* The groups in this group */
	struct wayca_sc_group *groups;
	/* The number of the groups in this group */
	int nr_groups;
	/**
	 * The cpuset of which has thread scheduled on.
	 * The set bit means it's occupied.
	 */
	cpu_set_t used;
	/**
	 * The cpuset this group owns.
	 * The set bit means it canbe used by this group.
	 */
	cpu_set_t total;
	/* The attribute specify the arrangement strategy of this group */
	wayca_sc_group_attr_t attribute;
	/* The mutex to protect this data structure */
	pthread_mutex_t mutex;

	/* Stride for arranging the threads */
	int stride;
	int nr_cpus_per_topo;
	/* A hint to indicates where to request cpus, -1 means no hint */
	int topo_hint;
	/* Roll over cnts */
	int roll_over_cnts;
};

#define group_for_each_threads(thread, group)	\
	for (thread = group->threads; thread != NULL; thread = thread->siblings)

/* Do the initialization work for a new create group */
int wayca_group_init(struct wayca_sc_group *group);

/* Arrange the resource of the group according to the attribute */
int wayca_group_arrange(struct wayca_sc_group *group);

/* Add the thread to the group, arrange the resource for it */
int wayca_group_add_thread(struct wayca_sc_group *group, struct wayca_thread *thread);

/* Delete one thread from the group. */
int wayca_group_delete_thread(struct wayca_sc_group *group, struct wayca_thread *thread);

/* Rearrange the resource assigned to the thread as the attribute of thread has been changed */
int wayca_group_rearrange_thread(struct wayca_sc_group *group, struct wayca_thread *thread);

/* Rearrange all the group threads' resources as the attribute of the group has been changed */
int wayca_group_rearrange_group(struct wayca_sc_group *group);

int wayca_group_add_group(struct wayca_sc_group *group, struct wayca_sc_group *father);

int wayca_group_delete_group(struct wayca_sc_group *group, struct wayca_sc_group *father);

void wayca_thread_update_load(struct wayca_thread *thread, bool add);

static inline int cpuset_find_first_unset(cpu_set_t *cpusetp)
{
	int pos;

	pos = find_first_zero_bit((unsigned long *)cpusetp, CPU_SETSIZE);

	return pos == CPU_SETSIZE ? -1 : pos;
}

static inline int cpuset_find_first_set(cpu_set_t *cpusetp)
{
	int pos;

	pos = find_first_bit((unsigned long *)cpusetp, CPU_SETSIZE);

	return pos == CPU_SETSIZE ? -1 : pos;
}

static inline int cpuset_find_last_set(cpu_set_t *cpusetp)
{
	int pos;

	pos = find_last_bit((unsigned long *)cpusetp, CPU_SETSIZE);

	return pos == CPU_SETSIZE ? -1 : pos;
}

static inline int cpuset_find_next_set(cpu_set_t *cpusetp, int begin)
{
	int pos;

	pos = find_next_bit((unsigned long *)cpusetp, CPU_SETSIZE, begin + 1);

	return pos == CPU_SETSIZE ? -1 : pos;
}
#endif	/* _WAYCA_THREAD_H */
