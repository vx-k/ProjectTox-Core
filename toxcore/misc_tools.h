/* misc_tools.h
 *
 * Miscellaneous functions and data structures for doing random things.
 *
 *  This file is in public domain. Note that this does not apply to the rest 
 *  of Tox project, which is licensed under GPLv3.
 *  When modifying this file, please explicitly mention that you release your
 *  modifications into public domain.
 */

#ifndef MISC_TOOLS_H
#define MISC_TOOLS_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h> /* for memcpy() */

unsigned char *hex_string_to_bin(char hex_string[]);

/*********************Debugging Macros********************
 * wiki.tox.im/index.php/Internal_functions_and_data_structures#Debugging
 *********************************************************/
#ifdef DEBUG
    #include <assert.h>
    #include <stdio.h>
    #include <string.h>

    #define DEBUG_PRINT(str, ...) do { \
        char msg[1000]; \
        sprintf(msg, "%s(): line %d (file %s): %s%%c\n", __FUNCTION__, __LINE__, __FILE__, str); \
        fprintf(stderr, msg, __VA_ARGS__); \
    } while (0)

    #define WARNING(...) do { \
        fprintf(stderr, "warning in "); \
        DEBUG_PRINT(__VA_ARGS__, ' '); \
    } while (0)

    #define INFO(...) do { \
        DEBUG_PRINT(__VA_ARGS__, ' '); \
    } while (0)

    #undef ERROR
    #define ERROR(exit_status, ...) do { \
        fprintf(stderr, "error in "); \
        DEBUG_PRINT(__VA_ARGS__, ' '); \
        exit(exit_status); \
    } while (0)
#else
    #define WARNING(...)
    #define INFO(...)
    #undef ERROR
    #define ERROR(...)
#endif // DEBUG

/************************Linked List***********************
 * http://wiki.tox.im/index.php/Internal_functions_and_data_structures#Linked_List
 * TODO:
 *    -Add a tox_array_for(range).
 **********************************************************/

#define MEMBER_OFFSET(member_name_in_parent, parent_type) \
    (&(((parent_type*)0)->member_name_in_parent))

/* Get a pointer to parent of X. X has to be a member of data structure parent_type. */
#define GET_PARENT(X, name_of_X_in_parent, parent_type) \
    ((parent_type*)((uint64_t)(&(X)) - (uint64_t)(MEMBER_OFFSET(name_of_X_in_parent, parent_type))))

/* tox_list_for_each*(lst, lst_name, tmp, tmp_type) { stuff_to_do_with_the_list(tmp); }
    Macro for performing an action on each element of tox_list.
    *WARNING* You should not use this macro unless the list contains at least 1 element.
parameters:
    lst        pointer to an instance of tox_list structure
    lst_name   name of lst inside the main data structure (see wiki)
    tmp        name of a pointer which is used to iterate the list
    tmp_type   type of data which tmp points to
*/
#define tox_list_for_each(lst, lst_name, tmp, tmp_type) \
    tox_list* __tox_ ## tmp = (lst)->prev; /* don't touch __tox_tmp */ \
    tmp_type* tmp = GET_PARENT(*(__tox_ ## tmp), lst_name, tmp_type); \
    for (; __tox_ ## tmp != (lst); \
         __tox_ ## tmp = __tox_ ## tmp ->prev, \
         tmp = GET_PARENT(*(__tox_ ## tmp), lst_name, tmp_type))

#define tox_list_for_each_reverse(lst, lst_name, tmp, tmp_type) \
    tox_list* __tox_ ## tmp = (lst)->next; /* don't touch __tox_tmp */ \
    tmp_type* tmp = GET_PARENT(*(__tox_ ## tmp), lst_name, tmp_type); \
    for (; __tox_ ## tmp != (lst); \
         __tox_ ## tmp = __tox_ ## tmp ->next, \
         tmp = GET_PARENT(*(__tox_ ## tmp), lst_name, tmp_type))

typedef struct tox_list {
    struct tox_list *prev, *next;
} tox_list;

/* Initializes a new list. */
static inline void tox_list_init(tox_list *lst)
{
    lst->prev = lst->next = lst;
}

/* Inserts a new tox_lst after lst.
 * lst is a head of a list, and new_lst is data to add.
 */
static inline void tox_list_add(tox_list *lst, tox_list *new_lst)
{
    tox_list_init(new_lst);

    new_lst->next = lst->next;
    new_lst->next->prev = new_lst;

    lst->next = new_lst;
    new_lst->prev = lst;
}

/* Remove 1 element from a list. */
static inline void tox_list_remove(tox_list *lst)
{
    lst->prev->next = lst->next;
    lst->next->prev = lst->prev;
}

/**********************************Array*********************************
 * Array which manages its own memory allocation.
 * It stores copy of data (not pointers).
 * TODO:
 *    -Add a tox_array_reverse() macro.
 *    -Add wiki info usage.
 *    -Add a function pointer to check if space is empty.
 *    -Add a function tox_array_add(); similar to push, but
 *     uses isEmpty to check for empties and returns place of new element.
 *    -tox_array_push_ptr should be able to add several elements at a time.
 *    -Add sorting algorithms (use qsort from stdlib?)
 *    -Add a tox_array_for(range).
 *    -uint8_t* or void* for storing data (?)
 *    -make tox_array_get() return ptr instead of element (?)
 ************************************************************************/

typedef struct tox_array {
    uint8_t *data;
    uint32_t len;
    size_t elem_size; /* in bytes */
    bool (*isElemEmpty)(uint8_t*);
} tox_array;

static inline void tox_array_init(tox_array *arr, size_t elem_size, bool (*isEmpty)(uint8_t*))
{
    arr->data = NULL;
    arr->len = 0;
    arr->elem_size = elem_size;
    arr->isEmpty = isEmpty;
}

static inline void tox_array_delete(tox_array *arr)
{
    free(arr->data);
    arr->len = arr->elem_size = 0;
    arr->isEmpty = NULL;
}

/* Only adds members at the end. Guaranteed to append item. */
static inline void tox_array_push_ptr(tox_array *arr, uint8_t *item, uint32_t num)
{
    arr->data = realloc(arr->data, arr->elem_size * (arr->len+1));
    if (item != NULL)
        memcpy(arr->data + arr->elem_size*arr->len, item, arr->elem_size);
    arr->len++;
}
#define tox_array_push(arr, item) tox_array_push_ptr(arr, (uint8_t*)(&(item)))

/* Deletes num items from array.
 * Not same as pop in stacks, because to access elements you use data.
 */
static inline void tox_array_pop(tox_array *arr, uint32_t num)
{
    if (num == 0)
        num = 1;
    arr->len -= num;
    arr->data = realloc(arr->data, arr->elem_size*arr->len);
}

/* TODO: return ptr and do not take type */
#define tox_array_get(arr, i, type) (((type*)(arr)->data)[i])

#define tox_array_for_each(arr, type, tmp_name) \
    type *tmp_name = &tox_array_get(arr, 0, type); uint32_t tmp_name ## _i = 0; \
    for (; tmp_name ## _i < (arr)->len; tmp_name = &tox_array_get(arr, ++ tmp_name ## _i, type))

#endif // MISC_TOOLS_H

