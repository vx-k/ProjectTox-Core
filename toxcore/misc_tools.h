/* misc_tools.h
 *
 * Miscellaneous functions and data structures for doing random things.
 *
 *  Copyright (C) 2013 Tox project All Rights Reserved.
 *
 *  This file is part of Tox.
 *
 *  Tox is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Tox is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Tox.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef MISC_TOOLS_H
#define MISC_TOOLS_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h> /* for memcpy() */

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

/****************************Array***************************
 * Array which manages its own memory allocation.
 * It stores copy of data (not pointers).
 * TODO: Add wiki info usage.
 ************************************************************/

typedef struct tox_array {
    uint8_t *data;
    uint32_t len;
    size_t elem_size; /* in bytes */
} tox_array;

static inline void tox_array_init(tox_array *arr, size_t elem_size)
{
    arr->len = 0;
    arr->elem_size = elem_size;
    arr->data = NULL;
}

static inline void tox_array_delete(tox_array *arr)
{
    free(arr->data);
    arr->len = arr->elem_size = 0;
}

static inline void tox_array_push_ptr(tox_array *arr, uint8_t *item)
{
    uint8_t *temp = realloc(arr->data, arr->elem_size * (arr->len + 1));

    if (temp == NULL)
        return 0;

    arr->data = temp;

    if (item != NULL)
        memcpy(arr->data + arr->elem_size * arr->len, item, arr->elem_size);

    arr->len++;
}
#define tox_array_push(arr, item) tox_array_push_ptr(arr, (uint8_t*)(&(item)))

/* Deletes num items from array.
 * Not same as pop in stacks, because to access elements you use data.
 */
static inline void tox_array_pop(tox_array *arr, uint32_t num)
{
    if (num == 0)
        return;

    if (num > arr->len)
        return;

    if (arr->len == num) {
        free(arr->data);
        arr->data = NULL;
        arr->len = 0;
        return;
    }

    uint8_t *temp = realloc(arr->data, arr->elem_size * (arr->len - num));

    if (temp == NULL)
        return;

    arr->len -= num;
    arr->data = temp;
}

/* TODO: return ptr and do not take type */
#define tox_array_get(arr, i, type) (((type*)(arr)->data)[i])

#define tox_array_for_each(arr, type, tmp_name) \
    type *tmp_name = &tox_array_get(arr, 0, type); uint32_t tmp_name ## _i = 0; \
    for (; tmp_name ## _i < (arr)->len; tmp_name = &tox_array_get(arr, ++ tmp_name ## _i, type))

/****************************Algorithms***************************
 * Macro/generic definitions for useful algorithms
 *****************************************************************/

/* Creates a new quick_sort implementation for arrays of the specified type.
 * For a type T (eg: int, char), creates a function named T_quick_sort.
 *
 * Quick Sort: Complexity O(nlogn)
 * arr   - the array to sort
 * n     - the sort index (should be called with n = length(arr))
 * cmpfn - a function that compares two values of type type. 
 *         Must return -1, 0, 1 for a < b, a == b, and a > b respectively.
 */
/* Must be called in the header file. */ 
#define declare_quick_sort(type) \
void type##_quick_sort(type *arr, int n, int (*cmpfn)(type, type));

/* Must be called in the C file. */
#define make_quick_sort(type) \
void type##_quick_sort(type *arr, int n, int (*cmpfn)(type, type)) \
{ \
    if ((n) < 2) \
        return; \
    type _p_ = (arr)[(n) / 2]; \
    type *_l_ = (arr); \
    type *_r_ = (arr) + n - 1; \
    while (_l_ <= _r_) { \
        if (cmpfn(*_l_, _p_) == -1) { \
            ++_l_; \
            continue; \
        } \
        if (cmpfn(*_r_, _p_) == 1) { \
            --_r_; \
            continue; \
        } \
        type _t_ = *_l_; \
        *_l_++ = *_r_; \
        *_r_-- = _t_; \
    } \
    type##_quick_sort((arr), _r_ - (arr) + 1, cmpfn); \
    type##_quick_sort(_l_, (arr) + n - _l_, cmpfn); \
}

#endif // MISC_TOOLS_H

