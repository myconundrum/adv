#ifndef BASEDS_H
#define BASEDS_H

#include <stddef.h>
#include <stdbool.h>

typedef struct ll_node {
    void *data;
    struct ll_node *next;
    struct ll_node *prev;
} ll_node;

typedef struct ll_list {
    ll_node *head;
    ll_node *tail;
    size_t data_size;
    size_t size;
} ll_list;

typedef struct {
    ll_list list;
} stack;

typedef struct {
    ll_list list;
} queue;


void ll_init(ll_list *list, size_t data_size);
void ll_push(ll_list *list, void *data);
void ll_pop(ll_list *list);
void ll_push_front(ll_list *list, void *data);
void ll_pop_front(ll_list *list);

void *ll_get(ll_list *list, size_t index);

void ll_remove(ll_list *list, size_t index);

void ll_list_destroy(ll_list *list);
void ll_list_remove_all(ll_list *list);

void stack_init(stack *stack, size_t data_size);
void stack_push(stack *stack, void *data);
void stack_pop(stack *stack);
void *stack_top(stack *stack);
bool stack_empty(stack *stack);

void queue_init(queue *queue, size_t data_size);
void queue_push(queue *queue, void *data);
void queue_pop(queue *queue);

#endif
