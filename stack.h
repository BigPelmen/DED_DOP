#ifndef STACK_H
#define STACK_H

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <varargs.h>

#ifdef STACK_DEBUG
#define ON_DBG(...) __VA_ARGS__
#else
#define ON_DBG(...)
#endif

#ifdef DEF_STACK_TYPE(type)
#define DEF_STACK_TYPE

struct stack {
    
}

#endif // STACK_H