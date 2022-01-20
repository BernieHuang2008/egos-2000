#pragma once

#include <stdlib.h>

#include "elf.h"
#include "mmu.h"

extern struct earth *earth;
#define printf    earth->tty_write

#define INFO      earth->log.log_info
#define HIGHLIGHT earth->log.log_highlight
#define SUCCESS   earth->log.log_success
#define ERROR     earth->log.log_error
#define FATAL     earth->log.log_fatal

