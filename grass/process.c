/*
 * (C) 2022, Cornell University
 * All rights reserved.
 */

/* Author: Yunhao Zhang
 * Description: process switching, communication and termination
 * system calls are basically inter-process communication
 */

#include "egos.h"
#include "grass.h"
#include <string.h>

int proc_nprocs, proc_curr_idx;
struct process proc_set[MAX_NPROCESS];

static void (*kernel_entry)();
static void proc_yield();
static void intr_entry(int id);

void proc_init() {
    earth->intr_register(intr_entry);

    proc_nprocs = 0;
    memset(proc_set, 0, sizeof(struct process) * MAX_NPROCESS);

    /* the first process is now running */
    proc_alloc();
    proc_curr_idx = 0;
    proc_set_running(PID(proc_curr_idx));
}

static void intr_entry(int id) {
    if (id == INTR_ID_TMR) {
        /* timer interrupt for scheduling */
        kernel_entry = proc_yield;
        ctx_start(&proc_set[proc_curr_idx].sp, (void*)KERNEL_STACK_TOP);
    } else if (id == INTR_ID_SOFT) {
        /* software interrupt for system call */
        struct syscall *sc = (struct syscall*)SYSCALL_ARGS_BASE;
        sc->type = SYS_UNUSED;
        *((int*)RISCV_CLINT0_MSIP_BASE) = 0;

        INFO("Got system call #%d with arg %d", sc->type, sc->args.exit.status);        
    } else {
        FATAL("Got unknown interrupt #%d", id);
    }
}

void ctx_entry() {
    kernel_entry();
    void* tmp;
    ctx_switch(&tmp, proc_set[proc_curr_idx].sp);
}

static void proc_yield() {
    if (earth->disk_busy()) {
        timer_reset();
        return;
    }

    int proc_next_idx = -1;
    for (int i = 1; i <= MAX_NPROCESS; i++) {
        int tmp_next = (proc_curr_idx + i) % MAX_NPROCESS;
        if (proc_set[tmp_next].status == PROC_READY ||
            proc_set[tmp_next].status == PROC_RUNNABLE) {
            proc_next_idx = tmp_next;
            break;
        }
    }

    if (proc_next_idx == proc_curr_idx) {
        timer_reset();
        return;
    }

    int next_pid = PID(proc_next_idx);
    earth->mmu_switch(next_pid);
    FATAL("Switching from proc %d to proc %d", PID(proc_curr_idx), next_pid);
}


int proc_alloc() {
    proc_nprocs++;
    for (int i = 0; i < MAX_NPROCESS; i++) {
        if (proc_set[i].pid == 0) {
            proc_set[i].pid = proc_nprocs;
            proc_set[i].status = PROC_UNUSED;
            return proc_nprocs;
        }
    }
    FATAL("Reach the limit of %d processes", MAX_NPROCESS);
    return -1;
}

static void proc_set_status(int pid, int status) {
    for (int i = 0; i < MAX_NPROCESS; i++) {
        if (proc_set[i].pid == pid) {
            proc_set[i].status = status;
            return;
        }
    }
}

void proc_free(int pid) {
    FATAL("proc_free not implemented");
}

void proc_set_ready(int pid) {
    proc_set_status(pid, PROC_READY);
}

void proc_set_running(int pid) {
    proc_set_status(pid, PROC_RUNNING);
}

void proc_set_runnable(int pid) {
    proc_set_status(pid, PROC_RUNNABLE);
}