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

// Feature test macro for strtok_r (c.f., Linux Programming Interface p. 63)
#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "libhw1/CSE333.h"
#include "./CrawlFileTree.h"
#include "./DocTable.h"
#include "./MemIndex.h"

//////////////////////////////////////////////////////////////////////////////
// Helper function declarations, constants, etc
static void Usage(void);
static void ProcessQueries(DocTable* dt, MemIndex* mi);
static int GetNextLine(FILE* f, char** ret_str);
static void ToLowerCase(char* string);

#define MAX_INPUT_SIZE 512

//////////////////////////////////////////////////////////////////////////////
// Main
int main(int argc, char** argv) {
  if (argc != 2) {
    Usage();
  }

  // Implement searchshell!  We're giving you very few hints
  // on how to do it, so you'll need to figure out an appropriate
  // decomposition into functions as well as implementing the
  // functions.  There are several major tasks you need to build:
  //
  //  - Crawl from a directory provided by argv[1] to produce and index
  //  - Prompt the user for a query and read the query from stdin, in a loop
  //  - Split a query into words (check out strtok_r)
  //  - Process a query against the index and print out the results
  //
  // When searchshell detects end-of-file on stdin (cntrl-D from the
  // keyboard), searchshell should free all dynamically allocated
  // memory and any other allocated resources and then exit.
  //
  // Note that you should make sure the fomatting of your
  // searchshell output exactly matches our solution binaries
  // to get full points on this part.

  fprintf(stdout, "Indexing '%s'\n", argv[1]);

  MemIndex* index;
  DocTable* doctable;
  if (!CrawlFileTree(argv[1], &doctable, &index)) {
    MemIndex_Free(index);
    DocTable_Free(doctable);
    fprintf(stderr, "Could not process given directory\n");
    Usage();
  }

  // prints result of queries until EOF is inputted.
  while (!feof(stdin)) {
    fprintf(stdout, "enter query:\n");
    ProcessQueries(doctable, index);
  }

  // cleaning up.
  fprintf(stdout, "shutting down...\n");
  MemIndex_Free(index);
  DocTable_Free(doctable);
  return EXIT_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////////
// Helper function definitions

static void Usage(void) {
  fprintf(stderr, "Usage: ./searchshell <docroot>\n");
  fprintf(stderr,
          "where <docroot> is an absolute or relative " \
          "path to a directory to build an index under.\n");
  exit(EXIT_FAILURE);
}

static void ProcessQueries(DocTable* dt, MemIndex* mi) {
  char input[MAX_INPUT_SIZE];
  // returns if there is nothing to read.
  if (!GetNextLine(stdin, &input[0])) {
    return;
  }
  char* queries[MAX_INPUT_SIZE];
  // the input is spit into queries where there spaces or breaks.
  // the number of queries are counted and added to queries.
  char* save_ptr;
  char* curr_query = strtok_r(input, " \n", &save_ptr);
  int query_index = 0;
  while (curr_query != NULL) {
    queries[query_index] = curr_query;
    curr_query = strtok_r(NULL, " \n", &save_ptr);
    query_index++;
  }

  LinkedList* search_results = MemIndex_Search(mi, queries, query_index);
  // returns if no matching docs found in MemIndex.
  if (search_results == NULL) {
    return;
  }

  // prints name and rank from search results.
  LLIterator* iter = LLIterator_Allocate(search_results);
  while (LLIterator_IsValid(iter)) {
    LLPayload_t payload;
    LLIterator_Get(iter, &payload);
    SearchResult* sr = (SearchResult*) payload;

    fprintf(stdout, "\t%s (%d)\n", DocTable_GetDocName(dt, sr->doc_id),
            sr->rank);
    LLIterator_Next(iter);
  }

  LLIterator_Free(iter);
  LinkedList_Free(search_results, free);
}

static int GetNextLine(FILE *f, char **ret_str) {
  char* result = fgets(ret_str, MAX_INPUT_SIZE, f);
  // returns 0 if any characters or lines were read.
  if (result == NULL) {
    return 0;
  }
  // returns 1 if no characters or lines were read.
  ToLowerCase(ret_str);
  return 1;
}

// takes string and makes all characters lower case.
static void ToLowerCase(char* string) {
  for (int i = 0; i < strlen(string); i++) {
    string[i] = tolower(string[i]);
  }
}
