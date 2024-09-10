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

#include <stdint.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <map>
#include <string>
#include <vector>

#include "./HttpRequest.h"
#include "./HttpUtils.h"
#include "./HttpConnection.h"

using std::map;
using std::string;
using std::vector;

namespace hw4 {

static const char* kHeaderEnd = "\r\n\r\n";
static const int kHeaderEndLen = 4;

bool HttpConnection::GetNextRequest(HttpRequest* const request) {
  // Use WrappedRead from HttpUtils.cc to read bytes from the files into
  // private buffer_ variable. Keep reading until:
  // 1. The connection drops
  // 2. You see a "\r\n\r\n" indicating the end of the request header.
  //
  // Hint: Try and read in a large amount of bytes each time you call
  // WrappedRead.
  //
  // After reading complete request header, use ParseRequest() to parse into
  // an HttpRequest and save to the output parameter request.
  //
  // Important note: Clients may send back-to-back requests on the same socket.
  // This means WrappedRead may also end up reading more than one request.
  // Make sure to save anything you read after "\r\n\r\n" in buffer_ for the
  // next time the caller invokes GetNextRequest()!

  // STEP 1:
  // check if buffer_ contains "\r\n\r\n".
  // read until next request end or error.
  size_t header_end = buffer_.find(kHeaderEnd);
  if (header_end == string::npos) {
    size_t bytes_read;
    unsigned char char_buf[512];
    while (1) {
      // read into buffer.
      bytes_read = WrappedRead(fd_, char_buf, 512);
      buffer_.append(reinterpret_cast<char*>(char_buf), bytes_read);
      // check if end of request string is now in buffer.
      header_end = buffer_.find(kHeaderEnd);
      if (header_end != string::npos) {
        break;
      }
      if (bytes_read == 0) {
        // connection dropped before complete header was read, return false.
        return false;
      }
    }
  }  // entire request already in buffer, no need to read further.

  // save and parse current request, then return parsed request.
  string end = kHeaderEnd;
  string curr_req = buffer_.substr(0, header_end + end.length());
  HttpRequest res = ParseRequest(curr_req);
  *request = res;

  // remove parsed request from buffer.
  buffer_.erase(0, header_end + end.length());
  return true;
}

bool HttpConnection::WriteResponse(const HttpResponse& response) const {
  string str = response.GenerateResponseString();
  int res = WrappedWrite(fd_,
                         reinterpret_cast<const unsigned char*>(str.c_str()),
                         str.length());
  if (res != static_cast<int>(str.length()))
    return false;
  return true;
}

HttpRequest HttpConnection::ParseRequest(const string& request) const {
  HttpRequest req("/");  // by default, get "/".

  // Plan for STEP 2:
  // 1. Split the request into different lines (split on "\r\n").
  // 2. Extract the URI from the first line and store it in req.URI.
  // 3. For the rest of the lines in the request, track the header name and
  //    value and store them in req.headers_ (e.g. HttpRequest::AddHeader).
  //
  // Hint: Take a look at HttpRequest.h for details about the HTTP header
  // format that you need to parse.
  //
  // You'll probably want to look up boost functions for:
  // - Splitting a string into lines on a "\r\n" delimiter
  // - Trimming whitespace from the end of a string
  // - Converting a string to lowercase.
  //
  // Note: If a header is malformed, skip that line.

  // STEP 2:
  // split request into lines.
  vector<string> lines;
  boost::split(lines, request,
              boost::is_any_of("\r\n"),
              boost::token_compress_on);

  // first line split by spaces.
  vector<string> first_line;
  boost::split(first_line, lines[0],
              boost::is_any_of(" "),
              boost::token_compress_on);

  // set URI from the second argument in the first line.
  req.set_uri(first_line[1]);

  // parse headers from subsequent lines, split by ":".
  for (size_t i = 1; i < lines.size(); i++) {
    vector<string> header;
    boost::split(header, lines[i],
                boost::is_any_of(":"),
                boost::token_compress_on);
    if (header.size() == 2) {
      // trim and convert headers.
      boost::algorithm::trim(header[1]);
      boost::algorithm::to_lower(header[0]);
      boost::algorithm::to_lower(header[1]);
      req.AddHeader(header[0], header[1]);
    }
  }
  return req;
}

}  // namespace hw4
