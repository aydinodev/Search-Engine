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

#ifndef HW2_DOCTABLE_H_
#define HW2_DOCTABLE_H_

#include <stdint.h>

#include "libhw1/HashTable.h"

// A type definition for document IDs.  '0' is a reserved value, indicating
// the "invalid" documentID.  We use an unsigned type for document ids so
// that we have the option of using either a counter or a hash to identify
// documents.
#define INVALID_DOCID 0U
typedef uint64_t DocID_t;

// A DocTable bidirectionally associates filenames <--> document IDs.
//
// A "document ID" is a unique integer generated by the DocTable
// and mapped to the passed-in filename ("document").  This document
// ID can be thought of as the document's "nickname" or its "shortened
// name", and it allows other data structures (eg, the inverted index)
// to refer to documents by a 64-bit number rather than a long string,
// thus saving memory and disk.
//
// Although C is a procedural language, HW1 demonstrated that we could
// implement some aspects of object-oriented programming; namely, data
// encapsulation (ie, a struct), access restrictions (ie, hiding the struct
// definition in the .c), and information hiding (ie, hiding functionality
// in the .c).  DocTable is an example of how to implement another OOP
// feature: object composition.  As is the case with an OOP language
// like Java or C++, this is accomplished by nesting one or more instances
// inside a new class.  See also MemIndex for a simple example of inheritance.
typedef struct doctable_st DocTable;

// Allocate and return a new DocTable.  The caller takes responsibility for
// eventually calling DocTable_Free to free memory associated with the table.
//
// Arguments: none.
//
// Returns:
// - the newly-allocated table (never NULL).
DocTable* DocTable_Allocate(void);

// Frees a DocTable that was previously allocated by DocTable_Allocate,
// including all strings stored inside of it.
//
// Arguments:
// - table: a previously-allocated DocTable.
void DocTable_Free(DocTable* table);

// Returns the number of mappings inside the DocTable.
//
// Arguments:
// - table: a DocTable
//
// Returns:
// - the number of mappings within the DocTable.
int DocTable_NumDocs(DocTable* table);

// Add a new filename to the DocTable and return the docID that was
// chosen for it.
//
// Arguments:
// - table: the DocTable to add the doc_name to
// - doc_name: the document's path+name, relative to the current working
//   directory. e.g., "foo/bar/baz.txt" means there is a "foo/"
//   subdirectory inside the current directory, and so on.  This function
//   makes a copy of the docuname, so the client retains ownership of
//   this parameter and is responsible for freeing it (if applicable).
//
// Returns:
// - the docID that was chosen for the document.  If the
//   document already exists inside the DocTable, its existing
//   docID is returned.
DocID_t DocTable_Add(DocTable* table, char* doc_name);

// Returns the docID associated with the passed-in document's path+name, or
// INVALID_DOCID if no such document exists in the table.
//
// Arguments:
// - table: the DocTable to look up the doc_name in
// - doc_name: the document's path+name to look up.  The client retains
//   ownership of this string.
//
// Returns:
// - the docID of a previously-added document, or INVALID_DOCID
DocID_t DocTable_GetDocID(DocTable* table, char* doc_name);

// Returns the path+name associated with the passed-in docID, or NULL if no
// such document exists in the table.  The table retains ownership of the
// returned string; the caller must NOT free() this string.
//
// Arguments:
// - table: the DocTable to look up the doc_id in
// - doc_id: the doc ID to lookup
//
// Returns:
//  - a string containing the file path+name for the document,
//    e.g., "foo/bar/baz.txt" or NULL.
char* DocTable_GetDocName(DocTable* table, DocID_t doc_id);


//////////////////////////////////////////////////////////////////////////////

// Returns the internal id->name mapping table.
//
// For HW3, clients will need to break the DocTable abstraction by directly
// accessing the HashTable which maintains the mapping from document IDs to
// to document names (ie, file path+name).  The client MUST NOT modify the
// returned hash table, since it will leave this DocTable in an undefined
// state.
//
// This function uses the DT_ prefix instead of the DocTable_ prefix, to
// indicate that this function should not be considered part of the DocTable's
// public API.
//
// Arguments:
// - table: the DocTable from which we return the id_to_name hashtable.
//
// Returns:
// - the id_to_name HashTable
HashTable* DT_GetIDToNameTable(DocTable* table);

// Returns the internal name->id mapping table
// This function should not be required by user code
// Arguments:
// - table: the DocTable from which we return the name_to_id table
//
// Returns:
// - the name_to_id HashTable
HashTable* DT_GetNameToIDTable(DocTable* table);

#endif  // HW2_DOCTABLE_H_
