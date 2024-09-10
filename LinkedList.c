/*
 * Copyright ©2024 Hannah C. Tang.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Washington
 * CSE 333 for use solely during Spring Quarter 2024 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include <stdio.h>
#include <stdlib.h>

#include "CSE333.h"
#include "LinkedList.h"
#include "LinkedList_priv.h"


///////////////////////////////////////////////////////////////////////////////
// LinkedList implementation.

LinkedList* LinkedList_Allocate(void) {
  // Allocate the linked list record.
  LinkedList *ll = (LinkedList *) malloc(sizeof(LinkedList));
  Verify333(ll != NULL);

  // STEP 1: initialize the newly allocated record structure.
  ll -> num_elements = 0;
  ll -> head = NULL;
  ll -> tail = NULL;
  // Return our newly minted linked list.
  return ll;
}

void LinkedList_Free(LinkedList *list,
                     LLPayloadFreeFnPtr payload_free_function) {
  Verify333(list != NULL);
  Verify333(payload_free_function != NULL);

  // STEP 2: sweep through the list and free all of the nodes' payloads
  // (using the payload_free_function supplied as an argument) and
  // the nodes themselves.
  LinkedListNode* curr_node = list->head;
  // loops over each node in linkedlist to free.
  while (curr_node != NULL) {
    LinkedListNode* next_node = curr_node->next;
    (payload_free_function)(curr_node->payload);
    free(curr_node);
    curr_node = next_node;
  }
  // free the LinkedList
  free(list);
}

int LinkedList_NumElements(LinkedList *list) {
  Verify333(list != NULL);
  return list->num_elements;
}

void LinkedList_Push(LinkedList *list, LLPayload_t payload) {
  Verify333(list != NULL);

  // Allocate space for the new node.
  LinkedListNode *ln = (LinkedListNode *) malloc(sizeof(LinkedListNode));
  Verify333(ln != NULL);

  // Set the payload
  ln->payload = payload;

  if (list->num_elements == 0) {
    // Degenerate case; list is currently empty
    Verify333(list->head == NULL);
    Verify333(list->tail == NULL);
    ln->next = ln->prev = NULL;
    list->head = list->tail = ln;
    list->num_elements = 1;
  } else {
    // STEP 3: typical case; list has >=1 elements
    // pushes new node to head of linkedlist and shifts list to the right.
    ln->next = list->head;
    list->head->prev = ln;
    ln->prev = NULL;
    list->head = ln;
    list->num_elements++;
  }
}

bool LinkedList_Pop(LinkedList *list, LLPayload_t *payload_ptr) {
  Verify333(payload_ptr != NULL);
  Verify333(list != NULL);

  // STEP 4: implement LinkedList_Pop.  Make sure you test for
  // and empty list and fail.  If the list is non-empty, there
  // are two cases to consider: (a) a list with a single element in it
  // and (b) the general case of a list with >=2 elements in it.
  // Be sure to call free() to deallocate the memory that was
  // previously allocated by LinkedList_Push().

  // can't remove any elements from empty list.
  if (list->num_elements == 0) {
    return false;
  } else {
    LinkedListNode* to_pop = list->head;
    *payload_ptr = to_pop->payload;
    // if only 1 element, list becomes empty.
    if (list->num_elements == 1) {
      list->head = list->tail = NULL;
    } else {
      // > 1 elements case.
      list->head = to_pop->next;
      list->head->prev = NULL;
    }
    free(to_pop);
    list->num_elements--;
    return true;  // head successfully removed.
  }
  return false;  // you may need to change this return value
}

void LinkedList_Append(LinkedList *list, LLPayload_t payload) {
  Verify333(list != NULL);

  // STEP 5: implement LinkedList_Append.  It's kind of like
  // LinkedList_Push, but obviously you need to add to the end
  // instead of the beginning.
  // Allocate space for the new node.
  LinkedListNode *ln = (LinkedListNode *) malloc(sizeof(LinkedListNode));
  Verify333(ln != NULL);

  // Set the payload
  ln->payload = payload;

  if (list->num_elements == 0) {
    // Degenerate case; list is currently empty
    Verify333(list->head == NULL);
    Verify333(list->tail == NULL);
    ln->next = ln->prev = NULL;
    list->head = list->tail = ln;
    list->num_elements = 1;
  } else {
    // Typical case; list has >=1 elements
    ln->next = NULL;
    ln->prev = list->tail;
    list->tail->next = ln;
    list->tail = ln;
    list->num_elements++;
  }
}

void LinkedList_Sort(LinkedList *list, bool ascending,
                     LLPayloadComparatorFnPtr comparator_function) {
  Verify333(list != NULL);
  if (list->num_elements < 2) {
    // No sorting needed.
    return;
  }

  // We'll implement bubblesort! Nnice and easy, and nice and slow :)
  int swapped;
  do {
    LinkedListNode *curnode;

    swapped = 0;
    curnode = list->head;
    while (curnode->next != NULL) {
      int compare_result = comparator_function(curnode->payload,
                                               curnode->next->payload);
      if (ascending) {
        compare_result *= -1;
      }
      if (compare_result < 0) {
        // Bubble-swap the payloads.
        LLPayload_t tmp;
        tmp = curnode->payload;
        curnode->payload = curnode->next->payload;
        curnode->next->payload = tmp;
        swapped = 1;
      }
      curnode = curnode->next;
    }
  } while (swapped);
}


///////////////////////////////////////////////////////////////////////////////
// LLIterator implementation.

LLIterator* LLIterator_Allocate(LinkedList *list) {
  Verify333(list != NULL);

  // OK, let's manufacture an iterator.
  LLIterator *li = (LLIterator *) malloc(sizeof(LLIterator));
  Verify333(li != NULL);

  // Set up the iterator.
  li->list = list;
  li->node = list->head;

  return li;
}

void LLIterator_Free(LLIterator *iter) {
  Verify333(iter != NULL);
  free(iter);
}

bool LLIterator_IsValid(LLIterator *iter) {
  Verify333(iter != NULL);
  Verify333(iter->list != NULL);

  return (iter->node != NULL);
}

bool LLIterator_Next(LLIterator *iter) {
  Verify333(iter != NULL);
  Verify333(iter->list != NULL);
  Verify333(iter->node != NULL);

  // STEP 6: try to advance iterator to the next node and return true if
  // you succeed, false otherwise
  // Note that if the iterator is already at the last node,
  // you should move the iterator past the end of the list

  // no nodes left if last in the list.
  if (iter->node != iter->list->tail) {
    iter->node = iter->node->next;
    return true;  // moves iter pointer successfully to valid node.
  }
  iter->node = iter->node->next;
  return false;  // iter moved past the end of the list.
}

void LLIterator_Get(LLIterator *iter, LLPayload_t *payload) {
  Verify333(iter != NULL);
  Verify333(iter->list != NULL);
  Verify333(iter->node != NULL);

  *payload = iter->node->payload;
}

bool LLIterator_Remove(LLIterator *iter,
                       LLPayloadFreeFnPtr payload_free_function) {
  Verify333(iter != NULL);
  Verify333(iter->list != NULL);
  Verify333(iter->node != NULL);

  // STEP 7: implement LLIterator_Remove.  This is the most
  // complex function you'll build.  There are several cases
  // to consider:
  // - degenerate case: the list becomes empty after deleting.
  // - degenerate case: iter points at head
  // - degenerate case: iter points at tail
  // - fully general case: iter points in the middle of a list,
  //                       and you have to "splice".
  //
  // Be sure to call the payload_free_function to free the payload
  // the iterator is pointing to, and also free any LinkedList
  // data structure element as appropriate.

  // frees payload.
  (payload_free_function)(iter->node->payload);

  // 1 element case.
  if (iter->list->num_elements == 1) {
    free(iter->node);
    iter->list->head = iter->list->tail = iter->node = NULL;
    iter->list->num_elements--;
    return false;  // list is now empty.
  } else {
    // > 1 elements cases.
    // iter node is the tail case.
    if (iter->list->num_elements > 1 && iter->node == iter->list->tail) {
      LinkedListNode* temp = iter->node->prev;
      free(iter->node);
      temp->next = NULL;
      iter->node = temp;
      iter->list->tail = iter->node;
    } else {
      // iter node is not tail case.
      // is_head will be true if iter node is head, and false if not.
      bool is_head = iter->list->head == iter->node ? 1 : 0;
      LinkedListNode* temp_next = iter->node->next;
      LinkedListNode* temp_prev = iter->node->prev;
      free(iter->node);
      iter->node = temp_next;
      iter->node->prev = temp_prev;
      // replaces head if node was head.
      if (is_head) {
        iter->list->head = iter->node;
      } else {
        // any node not head or tail.
        temp_prev->next = iter->node;
      }
    }
    iter->list->num_elements--;
    return true;  // deletion succeeded.
  }
  return false;  // deletion failed.
}


///////////////////////////////////////////////////////////////////////////////
// Helper functions

bool LLSlice(LinkedList *list, LLPayload_t *payload_ptr) {
  Verify333(payload_ptr != NULL);
  Verify333(list != NULL);

  // STEP 8: implement LLSlice.

  // can't remove nodes if list is empty.
  if (list->num_elements == 0) {
    return false;
  } else {
    LinkedListNode* to_slice = list->tail;
    *payload_ptr = to_slice->payload;
    // 1 element case.
    if (list->num_elements == 1) {
      list->head = list->tail = NULL;
    } else {
      // > 1 elements case.
      list->tail = to_slice->prev;
      list->tail->next = NULL;
    }
    free(to_slice);
    list->num_elements--;
    return true;  // successfully removed tail node.
  }
  return false;  // list is empty.
}

void LLIteratorRewind(LLIterator *iter) {
  iter->node = iter->list->head;
}
