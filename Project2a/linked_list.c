#include<stdio.h>
#include<stdlib.h>
#include "linked_list.h"

void ll_initialize(linked_list_t *ll) {
  ll->head = ll->tail = NULL;
}

static node_t * create_new_node(void *data) {
    node_t *new = malloc(sizeof(node_t));
    new->data = data;
    new->prev = new->next = NULL;
    return new;
}


void ll_insert_head(linked_list_t *ll, void *data) {
  node_t *new = create_new_node(data);

  /* If the list is empty */
  if (ll->head == NULL || ll->tail == NULL) {
    ll->head = ll->tail = new;
    return;
  }

  /* inset at head modify the pointers accordingly */
  new->next = ll->head;
  ll->head->prev = new;
  ll->head = new;
}

void ll_insert_tail(linked_list_t *ll, void *data) {
  node_t *new = create_new_node(data);
  if (ll->head == NULL || ll->tail == NULL) {
    ll->head = ll->tail = new;
    return;
  }
  new->prev = ll->tail;
  ll->tail->next = new;
  ll->tail = new;
}

void ll_delete_head(linked_list_t *ll) {
  node_t *del_node;
  /* Handling Error case */
  if (ll->head == NULL) {
    return;
  }

  del_node = ll->head;
  ll->head = ll->head->next;
  if (ll->head == NULL) {
    ll->tail = NULL;
  } else {
    ll->head->prev = NULL;
  }
  free(del_node);
}

void ll_delete_tail(linked_list_t *ll) {
  node_t *del_node;
  /* Handling Error case */
  if (ll->tail == NULL) {
    return;
  }
  del_node = ll->tail;
  ll->tail = ll->tail->prev;
  if (ll->tail == NULL) {
    ll->head = NULL;
  } else {
    ll->tail->next = NULL;
  }
  free(del_node);
}

void ll_delete_node(linked_list_t *ll, void *key) {
  node_t *itr = ll->head;

  if (itr == NULL)
    return;

  if (ll->head->data == key) {
    ll_delete_head(ll);
    return;
  } else if (ll->tail->data == key) {
    ll_delete_tail(ll);
    return;
  }

  for_each_node(itr, ll) {
    /* key found ! delete it */
    if (itr->data == key) {
      itr->prev->next = itr->next;
      itr->next->prev = itr->prev;
      free(itr);
      return;
    }
  }
}

node_t * ll_find(linked_list_t *ll, void *key,
  bool (*compare)(void *, void *)) {
  node_t *itr;
  for_each_node(itr, ll) {
    if (compare(key, itr->data) == true) {
      return itr;
    }
  }
  return NULL;
}

void ll_free(linked_list_t *ll) {
  node_t *itr = ll->head;
  node_t *del_node;
  if (itr == NULL)
    return;

  while (itr != NULL) {
      del_node = itr;
      itr = itr->next;
      del_node->next = del_node->prev = NULL;
      free(del_node);
  }

  ll->head = ll->tail = NULL;
  return;
}

void ll_free_with_data(linked_list_t *ll) {
  node_t *itr = ll->head;
  node_t *del_node;
  if (itr == NULL)
    return;

  while (itr != NULL) {
      del_node = itr;
      itr = itr->next;
      del_node->next = del_node->prev = NULL;
      free(del_node->data);
      free(del_node);
  }

  ll->head = ll->tail = NULL;
  return;
}

void ll_display(linked_list_t *ll) {
  node_t *itr = ll->head;
  for_each_node(itr, ll) {
    printf("%p-> ", itr->data);
  }
  printf("\n");
}

int ll_size(linked_list_t *ll) {
  node_t *itr = ll->head;
  int count = 0;
  for_each_node(itr, ll) {
    count++;
  }
  return count;
}
