#include <stdio.h>
#include <stdlib.h>

typedef struct list_s
{
        struct list_s *next; /* NULL for the last item in a list */
        int data;
}
list_t;

/**
 *  Counts the number of items in a list.
 *
 *  int count_list_items(const list_t *head) {
 *      if (head->next) {
 *          return count_list_items(head->next) + 1;
 *      } else {
 *          return 1;
 *      }
 *  }

 * 1. Need to check pointer parameter for NULL
 * 2. If the list is long, it can get stack overflow => Better avoid using recursion
 * 3. The result cannot be negative
 */
size_t count_list_items(const list_t *head) {
    int count = 0;
    if (head != NULL) 
        while (head->next != NULL)
        {
            count++;
            head = head->next;
        }
    return count;
}

/**
 *  Inserts a new list item after the one specified as the argument.
 * 
 *  void insert_next_to_list(list_t *item, int data) {
 *      (item->next = malloc(sizeof(list_t)))->next = item->next;
 *      item->next->data = data;
 *  }
 * 
 * 1. Need to check pointer paramter for NULL
 * 2. It's be better to use temporary variable
 */
void insert_next_to_list(list_t *item, int data) {
    if (item != NULL)
    {
        list_t* new_item;
        new_item = malloc(sizeof(list_t));
        new_item->data = data;
        new_item->next = NULL;

        if (item->next != NULL)
            new_item->next = item->next;

        item->next = new_item;
    }
}

/**
 *  Removes an item following the one specificed as the argument.
 * 
 *  void remove_next_from_list(list_t *item) {
 *      if (item->next) {
 *          free(item->next);
 *          item->next = item->next->next;
 *      }
 *  }
 * 
 * 1. Need to check pointer parameter for NULL
 * 2. Have to deallocate memory after assigment operation
 */
void remove_next_from_list(list_t *item) {
    if ((item != NULL) && (item->next != NULL))
    {
        list_t *next_item = item->next;
        item->next = next_item->next;

        free(next_item);        
    }
}

/**
 *  Returns item data as text
 * 
 *  char *item_data(const list_t *list)
 *  {
 *      char buf[12];
 *
 *      sprintf(buf, "%d", list->data);
 *      return buf;
 *  }
 *
 * 1. Need to check pointer parameter for NULL
 * 2. It would be correct allocate `buf` outside the function and pass it by parameter
 * 3. It would be better if the function returns the pointer to the next item
 * 4. `snprintf` is more secure
 */
list_t *item_data(const list_t *list, char *buf, int max_length)
{
    if (list != NULL)
    {
        snprintf(buf, max_length, "%d", list->data);
        return list->next;
    }
    return NULL;
}

/**
 * example 
 */
int main()
{
    list_t *head, *list;
    size_t count;
    char msg[16];

    // allocate head item and fill in the list with values [10...0]    
    head = malloc(sizeof(list_t));
    head->data = 10;
    head->next = NULL;
    
    for (int i=0; i<10; i++)
        insert_next_to_list(head, i);

    count = count_list_items(head);
    printf("count = %ld\r\n", count);

    // print the entire list
    list = head;
    while  ( list != NULL)
    {
        list = item_data(list, msg, sizeof(msg));
        printf("%s, ", msg);
    }
    printf("\r\n");

    // remove all elements
    while ( head->next != NULL )
        remove_next_from_list(head);

    // free head item
    free(head);
    head = NULL;
    
    count = count_list_items(head);
    printf("count = %ld\r\n", count);

    return 0;
}
