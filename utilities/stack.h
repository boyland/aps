
#ifndef LINKED_STACK_H
#define LINKED_STACK_H

#include <stdbool.h>
#include <stdint.h>

struct linked_stack {
  void* value;
  struct linked_stack* next;
};

typedef struct linked_stack LinkedStack;

/**
 * @brief Create stack using endogenous linked list
 * @param stack pointer to a stack pointer
 */
void stack_create(LinkedStack** stack);

/**
 * @brief Push method of stack
 * @param stack pointer to a stack pointer
 * @param value value to push to stack
 */
void stack_push(LinkedStack** stack, void* value);

/**
 * @brief Pop method of stack
 * @param stack pointer to a stack pointer
 * @param value that has just been popped from the stack
 * @return boolean indicating whether popping from the stack was successful or
 * not
 */
bool stack_pop(LinkedStack** stack, void** v);

/**
 * @brief Checks whether stack is empty or not
 * @param stack pointer to a stack pointer
 * @return boolean indicating whether stack is empty or not
 */
bool stack_is_empty(LinkedStack** stack);

/**
 * @brief Frees the memory allocated for the stack and deallocates each
 * individual element of stack
 * @param stack pointer to a stack pointer
 */
void stack_destroy(LinkedStack** stack);

#endif
