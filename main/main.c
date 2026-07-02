/*
 * main.c — firmware entry point. Do not edit.
 *
 * All OS logic lives in the sealed core (taskmaster_run()); this stub just starts it.
 * Your code goes in an app under apps/ (see apps/app_skeleton) — never here.
 */
#include "taskmaster.h"

void app_main(void)
{
    taskmaster_run();
}
