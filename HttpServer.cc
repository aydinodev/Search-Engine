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

#include <boost/algorithm/string.hpp>
#include <iostream>
#include <map>
#include <memory>
#include <vector>
#include <string>
#include <sstream>

#include "./FileReader.h"
#include "./HttpConnection.h"
#include "./HttpRequest.h"
#include "./HttpUtils.h"
#include "./HttpServer.h"
#include "./libhw3/QueryProcessor.h"

using std::cerr;
using std::cout;
using std::endl;
using std::list;
using std::map;
using std::string;
using std::stringstream;
using std::unique_ptr;

namespace hw4 {
///////////////////////////////////////////////////////////////////////////////
// Constants, internal helper functions
///////////////////////////////////////////////////////////////////////////////
static const char* kThreegleStr =
  "<html><head><title>333gle</title></head>\n"
  "<body>\n"
  "<center style=\"font-size:500%;\">\n"
  "<span style=\"position:relative;bottom:-0.33em;color:orange;\">3</span>"
    "<span style=\"color:red;\">3</span>"
    "<span style=\"color:gold;\">3</span>"
    "<span style=\"color:blue;\">g</span>"
    "<span style=\"color:green;\">l</span>"
    "<span style=\"color:red;\">e</span>\n"
  "</center>\n"
  "<p>\n"
  "<div style=\"height:20px;\"></div>\n"
  "<center>\n"
  "<form action=\"/query\" method=\"get\">\n"
  "<input type=\"text\" size=30 name=\"terms\" />\n"
  "<input type=\"submit\" value=\"Search\" />\n"
  "</form>\n"
  "</center><p>\n";

// static
const int HttpServer::kNumThreads = 100;

// This is the function that threads are dispatched into
// in order to process new client connections.
static void HttpServer_ThrFn(ThreadPool::Task* t);

// Given a request, produce a response.
static HttpResponse ProcessRequest(const HttpRequest& req,
                            const string& base_dir,
                            const list<string>& indices);

// Process a file request.
static HttpResponse ProcessFileRequest(const string& uri,
                                const string& base_dir);

// Process a query request.
static HttpResponse ProcessQueryRequest(const string& uri,
                                 const list<string>& indices);

// gets HTML string of <li> element for document matching queries.
// name links to file contents or website if it's a web link,
// with document rank.
static string GetMatchHTML(string doc_name, int rank);

// adds end HTML to ret body and sets response fields.
static void EndHTMLReponse(HttpResponse* ret);

///////////////////////////////////////////////////////////////////////////////
// HttpServer
///////////////////////////////////////////////////////////////////////////////
bool HttpServer::Run(void) {
  // Create the server listening socket.
  int listen_fd;
  cout << "  creating and binding the listening socket..." << endl;
  if (!socket_.BindAndListen(AF_INET6, &listen_fd)) {
    cerr << endl << "Couldn't bind to the listening socket." << endl;
    return false;
  }

  // Spin, accepting connections and dispatching them.  Use a
  // threadpool to dispatch connections into their own thread.
  cout << "  accepting connections..." << endl << endl;
  ThreadPool tp(kNumThreads);
  while (1) {
    HttpServerTask* hst = new HttpServerTask(HttpServer_ThrFn);
    hst->base_dir = static_file_dir_path_;
    hst->indices = &indices_;
    if (!socket_.Accept(&hst->client_fd,
                    &hst->c_addr,
                    &hst->c_port,
                    &hst->c_dns,
                    &hst->s_addr,
                    &hst->s_dns)) {
      // The accept failed for some reason, so quit out of the server.
      // (Will happen when kill command is used to shut down the server.)
      break;
    }
    // The accept succeeded; dispatch it.
    tp.Dispatch(hst);
  }
  return true;
}

static void HttpServer_ThrFn(ThreadPool::Task* t) {
  // Cast back our HttpServerTask structure with all of our new
  // client's information in it.
  unique_ptr<HttpServerTask> hst(static_cast<HttpServerTask*>(t));
  cout << "  client " << hst->c_dns << ":" << hst->c_port << " "
       << "(IP address " << hst->c_addr << ")" << " connected." << endl;

  // Read in the next request, process it, and write the response.

  // Use the HttpConnection class to read and process the next
  // request from our current client, then write out our response.  If
  // the client sends a "Connection: close\r\n" header, then shut down
  // the connection -- we're done.
  //
  // Hint: the client can make multiple requests on our single connection,
  // so we should keep the connection open between requests rather than
  // creating/destroying the same connection repeatedly.

  // STEP 1:
  // establish connection with client fd.
  HttpConnection htpc(hst->client_fd);

  // continuously process requests until "Connection: close" header or error.
  bool done = false;
  while (!done) {
    // get next request.
    HttpRequest request;
    if (!htpc.GetNextRequest(&request)) {
      break;
    }

    // check if request specifies to close connection.
    if (request.GetHeaderValue("connection") == "close") {
      done = true;
    }

    // process request and write response.
    HttpResponse response = ProcessRequest(request, hst->base_dir,
                                           *hst->indices);
    htpc.WriteResponse(response);
  }
}

static HttpResponse ProcessRequest(const HttpRequest& req,
                            const string& base_dir,
                            const list<string>& indices) {
  // Is the user asking for a static file?
  if (req.uri().substr(0, 8) == "/static/") {
    return ProcessFileRequest(req.uri(), base_dir);
  }

  // The user must be asking for a query.
  return ProcessQueryRequest(req.uri(), indices);
}

static HttpResponse ProcessFileRequest(const string& uri,
                                const string& base_dir) {
  // The response we'll build up.
  HttpResponse ret;

  // Steps to follow:
  // 1. Use the URLParser class to figure out what file name
  //    the user is asking for. Note that we identify a request
  //    as a file request if the URI starts with '/static/'
  //
  // 2. Use the FileReader class to read the file into memory
  //
  // 3. Copy the file content into the ret.body
  //
  // 4. Depending on the file name suffix, set the response
  //    Content-type header as appropriate, e.g.,:
  //      --> for ".html" or ".htm", set to "text/html"
  //      --> for ".jpeg" or ".jpg", set to "image/jpeg"
  //      --> for ".png", set to "image/png"
  //      etc.
  //    You should support the file types mentioned above,
  //    as well as ".txt", ".js", ".css", ".xml", ".gif",
  //    and any other extensions to get bikeapalooza
  //    to match the solution server.
  //
  // be sure to set the response code, protocol, and message
  // in the HttpResponse as well.
  string file_name = "";

  // STEP 2:
  // parse URI without "/static/"
  URLParser p;
  string front = "/static/";
  p.Parse(uri.substr(front.length()));
  file_name = p.path();

  // initialize FileReader for base_dir and file_name, then read contents.
  FileReader fr(base_dir, file_name);
  string file_contents;
  if (!fr.ReadFile(&file_contents) || !IsPathSafe(base_dir, base_dir + "/"
      + file_name)) {
    // handle file not found or outside directory with HTTP 404.
    ret.set_protocol("HTTP/1.1");
    ret.set_response_code(404);
    ret.set_message("Not Found");
    ret.AppendToBody("<html><body>Couldn't find file \"" + EscapeHtml(file_name)
                     + "\"</body></html>\n");
    return ret;
  }

  // append file_contents to response body.

  // determine file suffix and set appropriate content type.
  size_t suffix_pos = file_name.find_last_of(".");
  string suffix = file_name.substr(suffix_pos + 1);

  // set response details, including content type based on file suffix.
  map<string, string> file_types {
    {"html", "text/html"},
    {"htm", "text/html"},
    {"txt", "text/plain"},
    {"js", "text/javascript"},
    {"css", "text/css"},
    {"xml", "application/xml"},
    {"jpg", "image/jpeg"},
    {"jpeg", "image/jpeg"},
    {"png", "image/png"},
    {"gif", "image/gif"}
  };

  // set content type if suffix is defined in file_types.
  if (file_types.find(suffix) != file_types.end()) {
    ret.set_content_type(file_types[suffix]);
  }
  ret.set_protocol("HTTP/1.1");
  ret.set_response_code(200);
  ret.set_message("OK");

  return ret;
}

static HttpResponse ProcessQueryRequest(const string& uri,
                                 const list<string>& indices) {
  // The response we're building up.
  HttpResponse ret;

  // Your job here is to figure out how to present the user with
  // the same query interface as our solution_binaries/http333d server.
  // A couple of notes:
  //
  // 1. The 333gle logo and search box/button should be present on the site.
  //
  // 2. If the user had previously typed in a search query, you also need
  //    to display the search results.
  //
  // 3. you'll want to use the URLParser to parse the uri and extract
  //    search terms from a typed-in search query.  convert them
  //    to lower case.
  //
  // 4. Initialize and use hw3::QueryProcessor to process queries with the
  //    search indices.
  //
  // 5. With your results, try figuring out how to hyperlink results to file
  //    contents, like in solution_binaries/http333d. (Hint: Look into HTML
  //    tags!)

  // STEP 3:
  // initial HTML setup.
  ret.AppendToBody(kThreegleStr);

  // parse URI for queries.
  URLParser p;
  p.Parse(uri);

  // extract and sanitize search queries.
  vector<string> queries;
  string q_str;
  for (const auto& arg : p.args()) {
    if (arg.first == "terms") {
      q_str = EscapeHtml(arg.second);
      boost::split(queries, q_str, boost::is_any_of(" "),
                   boost::token_compress_on);
      break;
    }
  }

  // handle empty queries by ending response.
  if (queries.empty()) {
    EndHTMLReponse(&ret);
    return ret;
  }

  // convert queries to lowercase for processing.
  for (auto& query : queries) {
    boost::to_lower(query);
  }

  // process queries to find matching documents.
  hw3::QueryProcessor qp(indices);
  auto matches = qp.ProcessQuery(queries);

  // build results section header.
  string result_count = matches.empty() ? "No" : std::to_string(matches.size());
  ret.AppendToBody("<p><br>" + result_count + " results found for <b>" + q_str
                   + "</b></p><p> </p>");

  // append matched documents as HTML list items.
  if (!matches.empty())  {
    ret.AppendToBody("<ul>");
    for (auto& match : matches) {
      ret.AppendToBody(GetMatchHTML(match.document_name, match.rank));
    }
    ret.AppendToBody("</ul>");
  }

  // finalize HTML response and return.
  EndHTMLReponse(&ret);
  return ret;
}

// generate HTML for a matched document with hyperlink.
static string GetMatchHTML(string doc_name, int rank) {
  string ret = "<li><a href=\"";
  size_t type_pos = doc_name.find_last_of('.');
  if (doc_name.substr(type_pos, 4) == ".org") {
    ret.append(doc_name);
  } else {
    ret.append("/static/" + doc_name);
  }
  ret.append("\">" + doc_name + "</a> [" + std::to_string(rank) + "]</li>");
  return ret;
}

// finalize HTML content and set response details.
static void EndHTMLReponse(HttpResponse* ret) {
  ret->AppendToBody("</body>\n""</html>");
  ret->set_content_type("text/html");
  ret->set_protocol("HTTP/1.1");
  ret->set_response_code(200);
  ret->set_message("OK");
}

}  // namespace hw4
