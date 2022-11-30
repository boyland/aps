#include "stack.h"
#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/**
 * @brief Create stack using endogenous linked list
 * @param stack pointer to a stack pointer
 */
void stack_create(LinkedStack** stack) {
  *stack = NULL;
}

/**
 * @brief Push method of stack
 * @param stack pointer to a stack pointer
 * @param value value to push to stack
 */
void stack_push(LinkedStack** stack, uintptr_t value) {
  LinkedStack* item = (LinkedStack*)malloc(sizeof(LinkedStack));
  item->value = value;
  item->next = *stack;
  *stack = item;
}

/**
 * @brief Pop method of stack
 * @param stack pointer to a stack pointer
 * @param value that has just been popped from the stack
 * @return boolean indicating whether popping from the stack was successful or
 * not
 */
bool stack_pop(LinkedStack** stack, uintptr_t* v) {
  LinkedStack* old = *stack;
  if (old == NULL)
    return false;

  *v = old->value;
  *stack = old->next;
  free(old);
  return true;
}

/**
 * @brief Checks whether stack is empty or not
 * @param stack pointer to a stack pointer
 * @return boolean indicating whether stack is empty or not
 */
bool stack_is_empty(LinkedStack** stack) {
  return *stack == NULL;
}

/**
 * @brief Frees the memory allocated for the stack and deallocates each
 * individual element of stack
 * @param stack pointer to a stack pointer
 */
void stack_destroy(LinkedStack** stack) {
  uintptr_t v;
  while (stack_pop(stack, &v))
    ;
}
