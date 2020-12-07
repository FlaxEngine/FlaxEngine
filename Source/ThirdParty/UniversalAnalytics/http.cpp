/******************************************************************************
* Universal Analytics for C 
* Copyright (c) 2013, Analytics Pros
* 
* This project is free software, distributed under the BSD license. 
* Analytics Pros offers consulting and integration services if your firm needs 
* assistance in strategy, implementation, or auditing existing work.
******************************************************************************/

#include "http.h"
#include <assert.h>
#include <string.h>

/* This module provides an abstraction over CURL's HTTP methods,
 * in hope of decoupling HTTP processing from the rest of our tracking
 * logic, possibly supporting alternative HTTP libraries in the future.
 */


/* Data handler for CURL to silence it's default output */
size_t curl_null_data_handler(char *ptr, size_t size, size_t nmemb, void *userdata){
#if UA_DEBUG
    printf("Processing response 0x%lx\n", (unsigned long int) userdata);
#endif
  return (nmemb * size);
}

static inline unsigned int _minimum(unsigned int a, unsigned int b){
  return (b < a) ? b : a;
}


/* Sequentially de-queue requests */
int curl_multi_unload(CURLM* handler, CURL* requests[], unsigned int total){
  int count = _minimum(UA_MAX_QUERY_QUEUE, total);
  int i = count;

#if UA_DEBUG
    printf("Processing %d requests...\n", count);
#endif

  while(count){
    curl_multi_perform(handler, & count);
  }

  while(i--){
    curl_easy_cleanup(requests[ i ]);
    requests[ i ] = NULL;
  }

  return count;
}

/* Unload the queue and free any related memory
 *  - Process queued requests
 *  - Trigger CURL's native cleanup (free()) on its resources
 */
void HTTPcleanup(HTTPQueue_t* queue){
  if(NULL != queue->handler){
    curl_multi_unload(queue->handler, queue->requests, queue->count);
    curl_multi_cleanup(queue->handler);
    queue->handler = NULL;
  }
  queue->count = 0;
}

/* Prepare the CURL Multi handler */
void HTTPsetup(HTTPQueue_t* queue){
  queue->count = 0;
  queue->handler = curl_multi_init();
  curl_multi_setopt(queue->handler, CURLMOPT_PIPELINING, 1);
}

/* Process queued requests (but don't reset) */
void HTTPflush(HTTPQueue_t* queue){
  assert(NULL != queue->handler);
  if(0 < queue->count){
    curl_multi_unload(queue->handler, queue->requests, queue->count);
  }
  queue->count = 0;
}


/* Enqueue a POST using CURL */
int HTTPenqueue(HTTPQueue_t* queue, const char* endpoint, const char* useragent, const char* query, unsigned int query_len){
  
  assert(NULL != queue);
  assert(NULL != query);
  assert(NULL != endpoint);
  assert(NULL != useragent);

  CURLM* handler = queue->handler;
  CURL* curl = curl_easy_init();

#if UA_DEBUG
    printf("Queueing: \n\t- %s\n\t- %s\n\t- %s\n", endpoint, useragent, query);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
#endif

  unsigned int queue_capacity = (UA_MAX_QUERY_QUEUE - queue->count);

  if(queue_capacity == 0){
     HTTPflush(queue); // Process queued requests if no space remains
  }

  curl_easy_setopt(curl, CURLOPT_URL, endpoint);
  curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, query);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, useragent);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_null_data_handler);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, curl);

  curl_multi_add_handle(handler, curl);

  queue->requests[ queue->count++ % UA_MAX_QUERY_QUEUE ] = curl; 

  return queue->count;
}


