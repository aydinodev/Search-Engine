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

#include "./QueryProcessor.h"

#include <iostream>
#include <algorithm>
#include <list>
#include <string>
#include <vector>

extern "C" {
  #include "./libhw1/CSE333.h"
}

using std::list;
using std::sort;
using std::string;
using std::vector;

namespace hw3 {

// creates DocIDTableReader* array by searching for query
// in index tables from itr_array.
// takes query to search index tables, itr_array to access
// all index tables, array_len for length of itr_array, and
// docidtr_array to store the output resulting DocIDTableReader*.
// returns list of offsets for the element fields in hash value bucket.
//  returns empty list if not elements in bucket.
static void createDocidtrArray(const std::string& query,
  IndexTableReader** itr_array, const int& array_len,
  vector<DocIDTableReader*>* docidtr_array);

// creates QueryResult array by searching docidtr_array for DocID table headers
// and looking for the DocID in the doc tables to get rank.
// take docidtr_array to get pointers to DocIDTableReader and
// doctr_array to access all DocTables.
// returns array of query results withDocID and rank.
static vector<QueryProcessor::QueryResult> getQueryResults(
  const vector<DocIDTableReader*>& docidtr_array, DocTableReader** doctr_array);

QueryProcessor::QueryProcessor(const list<string>& index_list, bool validate) {
  // Stash away a copy of the index list.
  index_list_ = index_list;
  array_len_ = index_list_.size();
  Verify333(array_len_ > 0);

  // Create the arrays of DocTableReader*'s. and IndexTableReader*'s.
  dtr_array_ = new DocTableReader* [array_len_];
  itr_array_ = new IndexTableReader* [array_len_];

  // Populate the arrays with heap-allocated DocTableReader and
  // IndexTableReader object instances.
  list<string>::const_iterator idx_iterator = index_list_.begin();
  for (int i = 0; i < array_len_; i++) {
    FileIndexReader fir(*idx_iterator, validate);
    dtr_array_[i] = fir.NewDocTableReader();
    itr_array_[i] = fir.NewIndexTableReader();
    idx_iterator++;
  }
}

QueryProcessor::~QueryProcessor() {
  // Delete the heap-allocated DocTableReader and IndexTableReader
  // object instances.
  Verify333(dtr_array_ != nullptr);
  Verify333(itr_array_ != nullptr);
  for (int i = 0; i < array_len_; i++) {
    delete dtr_array_[i];
    delete itr_array_[i];
  }

  // Delete the arrays of DocTableReader*'s and IndexTableReader*'s.
  delete[] dtr_array_;
  delete[] itr_array_;
  dtr_array_ = nullptr;
  itr_array_ = nullptr;
}

// This structure is used to store a index-file-specific query result.
typedef struct {
  DocID_t doc_id;  // The document ID within the index file.
  int     rank;    // The rank of the result so far.
} IdxQueryResult;

vector<QueryProcessor::QueryResult>
QueryProcessor::ProcessQuery(const vector<string>& query) const {
  Verify333(query.size() > 0);

  // STEP 1.
  // (the only step in this file)
  vector<QueryProcessor::QueryResult> final_result;

  vector<DocIDTableReader*> docidtr_array;
  createDocidtrArray(query.front(), itr_array_, array_len_, &docidtr_array);

  final_result = getQueryResults(docidtr_array, dtr_array_);
  docidtr_array.clear();
  int query_size = static_cast<int>(query.size());
  for (int i = 1; i < query_size; i++) {
    createDocidtrArray(query[i], itr_array_, array_len_, &docidtr_array);

    vector<QueryProcessor::QueryResult> curr_res =
      getQueryResults(docidtr_array, dtr_array_);
    docidtr_array.clear();

    for (auto it = final_result.begin(); it != final_result.end(); ) {
      bool wasFound = false;
      for (QueryProcessor::QueryResult curr_query : curr_res) {
        if (it->document_name == curr_query.document_name) {
          it->rank += curr_query.rank;
          wasFound = true;
          it++;
          break;
        }
      }

      if (!wasFound) {
        it = final_result.erase(it);
      }
    }
  }

  // Sort the final results.
  sort(final_result.begin(), final_result.end());
  return final_result;
}

static void createDocidtrArray(const std::string& query,
  IndexTableReader** itr_array, const int& array_len,
  vector<DocIDTableReader*>* docidtr_array) {
  for (int i = 0; i < array_len; i++) {
    docidtr_array->push_back(itr_array[i]->LookupWord(query));
  }
}

static vector<QueryProcessor::QueryResult> getQueryResults(
  const vector<DocIDTableReader*>& docidtr_array,
    DocTableReader** doctr_array) {
  vector<QueryProcessor::QueryResult> query_result;
  int count = 0;

  for (DocIDTableReader* docidtr : docidtr_array) {
    if (docidtr == nullptr) {
      count++;
      continue;
    }

    list<DocIDElementHeader> docidElementHeaderList = docidtr->GetDocIDList();
    for (DocIDElementHeader docidElementHeader : docidElementHeaderList) {
      QueryProcessor::QueryResult queryResult;
      queryResult.rank = docidElementHeader.num_positions;
      doctr_array[count]->LookupDocID(docidElementHeader.doc_id,
        &queryResult.document_name);

      query_result.push_back(queryResult);
    }
    count++;
  }

  return query_result;
}

}  // namespace hw3
