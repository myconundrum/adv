#include "baseds.h"
#include "mempool.h"
#include "appstate.h"
#include "log.h"
#include <stdlib.h>
#include <string.h>


void ll_init(ll_list *list, size_t data_size) {
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    list->data_size = data_size;
}

void ll_push(ll_list *list, void *data) {
    ll_node *new_node = (ll_node *)pool_malloc(sizeof(ll_node) + list->data_size, appstate_get());
    new_node->data = (char*)new_node + sizeof(ll_node);
    memcpy(new_node->data, data, list->data_size);
    new_node->next = NULL;
    new_node->prev = list->tail;
    if (list->head == NULL) {
        list->head = new_node;
    }
    if (list->tail != NULL) {
        list->tail->next = new_node;
    }
    list->tail = new_node;
    list->size++;
}

void ll_push_front(ll_list *list, void *data) {
    ll_node *new_node = (ll_node *)pool_malloc(sizeof(ll_node) + list->data_size, appstate_get());
    new_node->data = (char*)new_node + sizeof(ll_node);
    memcpy(new_node->data, data, list->data_size);
    new_node->next = list->head;
    new_node->prev = NULL;
    if (list->head != NULL) {
        list->head->prev = new_node;
    }
    list->head = new_node;
    list->size++;
}

void ll_pop(ll_list *list) {
    if (list->head == NULL) {
        return;
    }
    ll_node *node = list->head;
    list->head = node->next;
    if (list->head == NULL) {
        list->tail = NULL;
    }
    pool_free(node, appstate_get());
    list->size--;
}

void ll_pop_front(ll_list *list) {
    if (list->head == NULL) {
        return;
    }
    ll_node *node = list->head;
    list->head = node->next;
    if (list->head == NULL) {
        list->tail = NULL;
    }
    pool_free(node, appstate_get());
    list->size--;
}

void *ll_get(ll_list *list, size_t index) {
    if (!list || index >= list->size) {
        return NULL;
    }
    
    ll_node *current = list->head;
    for (size_t i = 0; i < index; i++) {
        if (current == NULL) {
            return NULL;
        }
        current = current->next;
    }
    
    return current ? current->data : NULL;
}

void ll_remove(ll_list *list, size_t index) {
    if (!list || index >= list->size) {
        return;
    }
    
    ll_node *current = list->head;
    for (size_t i = 0; i < index; i++) {
        if (current == NULL) {
            return;
        }
        current = current->next;
    }
    
    if (current == NULL) {
        return;
    }
    
    // Remove the node
    if (current->prev) {
        current->prev->next = current->next;
    } else {
        list->head = current->next;
    }
    
    if (current->next) {
        current->next->prev = current->prev;
    } else {
        list->tail = current->prev;
    }
    
    pool_free(current, appstate_get());
    list->size--;
}

void stack_init(stack *stack, size_t data_size) {
    ll_init(&stack->list, data_size);
}

void stack_push(stack *stack, void *data) {
    ll_push_front(&stack->list, data);
}

void stack_pop(stack *stack) {
    ll_pop(&stack->list);
}

void *stack_top(stack *stack) {
    if (stack->list.head == NULL) {
        return NULL;
    }
    return stack->list.head->data;
}

bool stack_empty(stack *stack) {
    return stack->list.head == NULL;
}

void queue_init(queue *queue, size_t data_size) {
    ll_init(&queue->list, data_size);
}

void queue_push(queue *queue, void *data) {
    ll_push(&queue->list, data);
}

void queue_pop(queue *queue) {
    ll_pop(&queue->list);
}

void ll_list_destroy(ll_list *list) {
    if (!list) return;
    
    ll_node *current = list->head;
    while (current != NULL) {
        ll_node *next = current->next;
        pool_free(current, appstate_get());
        current = next;
    }
    
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

void ll_list_remove_all(ll_list *list) {
    ll_list_destroy(list);
}
