/*
Copyright (c) 2007-2009, Troy D. Hanson
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef UTLIST_H
#define UTLIST_H

#define UTLIST_VERSION 1.7

/* From: http://uthash.sourceforge.net/utlist.html */
/*
 * This file contains macros to manipulate singly and doubly-linked lists.
 *
 * 1. LL_ macros:  singly-linked lists.
 * 2. DL_ macros:  doubly-linked lists.
 * 3. CDL_ macros: circular doubly-linked lists.
 *
 * To use singly-linked lists, your structure must have a "next" pointer.
 * To use doubly-linked lists, your structure must "prev" and "next" pointers.
 * Either way, the pointer to the head of the list must be initialized to NULL.
 *
 * ----------------.EXAMPLE -------------------------
 * struct item {
 *      int id;
 *      struct item *prev, *next;
 * }
 *
 * struct item *list = NULL:
 *
 * int main() {
 *      struct item *item;
 *      ... allocate and populate item ...
 *      DL_APPEND(list, item);
 * }
 * --------------------------------------------------
 *
 * For doubly-linked lists, the append and delete macros are O(1)
 * For singly-linked lists, append and delete are O(n) but prepend is O(1)
 * The sort macro is O(n log(n)) for all types of single/double/circular lists.
 */


/******************************************************************************
 * doubly linked list macros (non-circular)                                   *
 *****************************************************************************/
#define DL_PREPEND(head,add)                                                     \
do {                                                                             \
 (add)->next = head;                                                             \
 if (head) {                                                                     \
   (add)->prev = (head)->prev;                                                   \
   (head)->prev = (add);                                                         \
 } else {                                                                        \
   (add)->prev = (add);                                                          \
 }                                                                               \
 (head) = (add);                                                                 \
} while (0)

#define DL_APPEND(head,add)                                                      \
do {                                                                             \
  if (head) {                                                                    \
      (add)->prev = (head)->prev;                                                \
      (head)->prev->next = (add);                                                \
      (head)->prev = (add);                                                      \
      (add)->next = NULL;                                                        \
  } else {                                                                       \
      (head)=(add);                                                              \
      (head)->prev = (head);                                                     \
      (head)->next = NULL;                                                       \
  }                                                                              \
} while (0);

#define DL_DELETE(head,del)                                                      \
do {                                                                             \
  if ((del)->prev == (del)) {                                                    \
      (head)=NULL;                                                               \
  } else if ((del)==(head)) {                                                    \
      (del)->next->prev = (del)->prev;                                           \
      (head) = (del)->next;                                                      \
  } else {                                                                       \
      (del)->prev->next = (del)->next;                                           \
      if ((del)->next) {                                                         \
          (del)->next->prev = (del)->prev;                                       \
      } else {                                                                   \
          (head)->prev = (del)->prev;                                            \
      }                                                                          \
  }                                                                              \
} while (0);


#define DL_FOREACH(head,el)                                                      \
    for(el=head;el;el=el->next)

#define DL_FOREACH_SAFE(head,el,tmp)                                             \
    for(el=head,tmp=el->next;el;el=tmp,tmp=(el) ? (el->next) : NULL)

#endif /* UTLIST_H */

