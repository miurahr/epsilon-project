/*
 * $Id: list.c,v 1.18 2010/02/05 23:50:22 simakov Exp $
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

#include <common.h>
#include <list.h>
#include <mem_alloc.h>

linked_list *alloc_linked_list(void)
{
    linked_list *list;

    list = (linked_list *) xmalloc(sizeof(linked_list));
    list->first = list->last = NULL;

    return list;
}

void free_linked_list(linked_list *list)
{
    list_node *cur_node;
    list_node *next_node;

    cur_node = list->first;

    while (cur_node) {
        next_node = cur_node->next;
        free_list_node(cur_node);
        cur_node = next_node;
    }

    free(list);
}

list_node *alloc_list_node(int data_size)
{
    list_node *node;

    node = (list_node *) xmalloc(sizeof(list_node));
    node->data = xmalloc(data_size);
    node->next = node->prev = NULL;

    return node;
}

void free_list_node(list_node *node)
{
    free(node->data);
    free(node);
}

void append_list_node(linked_list *list, list_node *node)
{
    if ((list->first == NULL) && (list->last == NULL)) {
        node->next = node->prev = NULL;
        list->first = list->last = node;
        return;
    }

    node->next = NULL;
    node->prev = list->last;

    list->last->next = node;
    list->last = node;
}

void prepend_list_node(linked_list *list, list_node *node)
{
    if ((list->first == NULL) && (list->last == NULL)) {
        node->next = node->prev = NULL;
        list->first = list->last = node;
        return;
    }

    node->prev = NULL;
    node->next = list->first;

    list->first->prev = node;
    list->first = node;
}

void remove_list_node_link(linked_list *list, list_node *node)
{
    if (node->prev) {
        node->prev->next = node->next;
    } else {
        list->first = node->next;
    }

    if (node->next) {
        node->next->prev = node->prev;
    } else {
        list->last = node->prev;
    }
}

void remove_list_node(linked_list *list, list_node *node)
{
    remove_list_node_link(list, node);
    free_list_node(node);
}

void move_list_node(linked_list *src_list, linked_list *dst_list, list_node *node)
{
    remove_list_node_link(src_list, node);
    append_list_node(dst_list, node);
}

void insert_before_list_node(linked_list *list, list_node *node, list_node *new_node)
{
    if (!node) {
        prepend_list_node(list, new_node);
        return;
    }

    if (node->prev) {
        new_node->next = node;
        new_node->prev = node->prev;
        node->prev = node->prev->next = new_node;
    } else {
        new_node->prev = NULL;
        new_node->next = node;
        node->prev = list->first = new_node;
    }
}

void insert_after_list_node(linked_list *list, list_node *node, list_node *new_node)
{
    if (!node) {
        append_list_node(list, new_node);
        return;
    }

    if (node->next) {
        new_node->prev = node;
        new_node->next = node->next;
        node->next = node->next->prev = new_node;
    } else {
        new_node->next = NULL;
        new_node->prev = node;
        node->next = list->last = new_node;
    }
}
