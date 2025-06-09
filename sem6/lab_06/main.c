#include <linux/dcache.h>
#include <linux/fs_struct.h>
#include <linux/init_task.h>
#include <linux/module.h>
#include <linux/sched.h>

MODULE_LICENSE("GPL");

static int __init md_init(void)
{
    struct task_struct *task = &init_task;

    do
    {
        printk(KERN_INFO "+ task: pid - %5d,  comm - %15s, ppid - %5d, pcomm - %15s, state - %5ld, on_cpu - %5d, flags - %10x, prio - %5d, policy - %d, exit_code - %d, exit_state - %d, in_execve - %d, utime - %13llu, stime - %13llu, root - %s\n",
           task->pid,
           task->comm,
           task->parent->pid,
           task->parent->comm,
           task->state,
           task->on_cpu,
           task->flags,
           task->prio,
           task->policy,
           task->exit_code,
           task->exit_state,
           task->in_execve,
           task->utime,
           task->stime,
           task->fs->root.dentry->d_name.name);
    } while ((task = next_task(task)) != &init_task);

    printk(KERN_INFO "+ current: pid - %5d,  comm - %15s, ppid - %5d, pcomm - %15s, state - %5ld, on_cpu - %5d, flags - %10x, prio - %5d, policy - %d, exit_code - %d, exit_state - %d, in_execve - %d, utime - %13llu, stime - %13llu, root - %s\n",
           current->pid,
           current->comm,
           current->parent->pid,
           current->parent->comm,
           current->state,
           current->on_cpu,
           current->flags,
           current->prio,
           current->policy,
           current->exit_code,
           current->exit_state,
           current->in_execve,
           current->utime,
           current->stime,
           current->fs->root.dentry->d_name.name);

    return 0;
}

static void __exit md_exit(void) 
{ 
    printk(KERN_INFO "+ Good by\n"); 
}

module_init(md_init);
module_exit(md_exit);

