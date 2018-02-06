/*
 * $Id: list.h,v 1.19 2010/02/05 23:50:22 simakov Exp $
 *
 * EPSILON - wavelet image compression library.
 * Copyright (C) 2006,2007,2010 Alexander Simakov, <xander@entropyware.info>
 *
 * This file is part of EPSILON
 *
 * EPSILON is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * EPSILON is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with EPSILON.  If not, see <http://www.gnu.org/licenses/>.
 *
 * http://epsilon-project.sourceforge.net
 */

/** \file
 *
 *  \brief Doubly-linked lists
 *
 *  This file contains routines to deal with doubly-linked lists. */

#ifndef __LIST_H__
#define __LIST_H__

#ifdef __cplusplus
extern "C" {
#endif

/** \addtogroup list Doubly-linked lists */
/*@{*/

#include <common.h>

/** This macro evaluates to 1 if list \a _x is empty */
#define LIST_IS_EMPTY(_x)       (_x->first == NULL)

/** List node structure */
typedef struct list_node_tag list_node;

/** List node structure
 *
 *  This structure represents a doubly-linked list node. */
struct list_node_tag {
    /** Data block pointer */
    void *data;
    /** Next list node */
    list_node *next;
    /** Previous list node */
    list_node *prev;
};

/** Doubly-linked list structure
 *
 *  This structure represents a doubly-linked list. */
typedef struct linked_list_tag {
    /** First node */
    list_node *first;
    /** Last node */
    list_node *last;
} linked_list;

/** Allocate new doubly-linked list
 *
 *  This function allocates new doubly-linked list.
 *
 *  \return New list pointer */
linked_list *alloc_linked_list(void);

/** Release doubly-linked list
 *
 *  This function releases doubly-linked \a list with all
 *  its internal nodes. Data blocks associated with nodes
 *  are also released.
 *
 *  \return \c VOID */
void free_linked_list(linked_list *list);

/** Allocate new list node
 *
 *  This function allocates new list node with associated
 *  data block of size \a data_size.
 *
 *  \return New node pointer */
list_node *alloc_list_node(int data_size);

/** Release list node
 *
 *  This function releases list \a node and data block
 *  associated with it.
 *
 *  \return \c VOID */
void free_list_node(list_node *node);

/** Append list node
 *
 *  This function appends new \a node to the \a list tail.
 *
 *  \note Node must be properly allocated beforehand.
 *
 *  \return \c VOID */
void append_list_node(linked_list *list, list_node *node);

/** Prepend list node
 *
 *  This function prepends new \a node to the \a list head.
 *
 *  \note Node must be properly allocated beforehand.
 *
 *  \return \c VOID */
void prepend_list_node(linked_list *list, list_node *node);

/** Remove list node
 *
 *  This function removes \a node links from the \a list.
 *
 *  \note The \a node itself is not released and can be added
 *  to another list.
 *
 *  \return \c VOID */
void remove_list_node_link(linked_list *list, list_node *node);

/** Remove list node
 *
 *  This function removes \a node from the \a list.
 *
 *  \note The \a node itself is released and cannot be added
 *  to another list.
 *
 *  \return \c VOID */
void remove_list_node(linked_list *list, list_node *node);

/** Move node
 *
 *  This function moves \a node from \a src_list to \a dst_list.
 *
 *  \return \c VOID */
void move_list_node(linked_list *src_list, linked_list *dst_list, list_node *node);

/** Insert node
 *
 *  This function inserts \a new_node to the \a list just before \a node.
 *
 *  \return \c VOID */
void insert_before_list_node(linked_list *list, list_node *node, list_node *new_node);

/** Insert node
 *
 *  This function inserts \a new_node to the \a list just after \a node.
 *
 *  \return \c VOID */
void insert_after_list_node(linked_list *list, list_node *node, list_node *new_node);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* __LIST_H__ */
