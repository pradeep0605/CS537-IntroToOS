#define true 1
#define false 0

typedef unsigned int bool;

#define for_each_node(itr, ll) \
    for ((itr) = (ll)->head; (itr); (itr) = (itr)->next) \

typedef struct node {
  struct node *prev;
  struct node *next;
  void *data;
} node_t;

typedef struct linked_list {
  node_t *head;
  node_t *tail;
} linked_list_t;


void ll_initialize(linked_list_t *ll);
void ll_insert_head(linked_list_t *ll, void *data);
void ll_insert_tail(linked_list_t *ll, void *data);
void ll_delete_head(linked_list_t *ll);
void ll_delete_tail(linked_list_t *ll);
void ll_delete_node(linked_list_t *ll, void *data);
node_t * ll_find(linked_list_t *ll, void *key, bool (*compare)(void *, void *));
void ll_free(linked_list_t *ll);
void ll_free_with_data(linked_list_t *ll);
void ll_display(linked_list_t *ll);
int ll_size(linked_list_t *ll);

