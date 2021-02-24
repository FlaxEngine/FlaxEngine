/******************************************************************************
* Universal Analytics for C 
* Copyright (c) 2013, Analytics Pros
* 
* This project is free software, distributed under the BSD license. 
* Analytics Pros offers consulting and integration services if your firm needs 
* assistance in strategy, implementation, or auditing existing work.
******************************************************************************/

#include <ctype.h>
#include <assert.h>
#include <string.h>

#include "universal-analytics.h"

#define UA_MEM_MAGIC_UNSET 0xDEADC0DE
#define UA_MEM_MAGIC_CONFIG 0xADDED00
#define UA_DEFAULT_OPTION_QUEUE 1

static const char UA_ENDPOINT[] = "https://www.google-analytics.com/collect";
static const char UA_USER_AGENT_DEFAULT[] = "Analytics Pros - Universal Analytics for C";
static const char UA_PROTOCOL_VERSION[] = "1";



/* Define tracking type strings; these are protocol-constants for
 * Measurement Protocol v1. We may eventually migrate these to a separate 
 * protocol module. */
static inline int populateTypeNames(const char* types[]){
  types[UA_PAGEVIEW] = "pageview";
  types[UA_APPVIEW] = "screenview";
  types[UA_SCREENVIEW] = "screenview";
  types[UA_EVENT] = "event";
  types[UA_TRANSACTION] = "trans";
  types[UA_TRANSACTION_ITEM] = "item";
  types[UA_TIMING] = "timing";
  types[UA_SOCIAL] = "social";
  types[UA_EXCEPTION] = "exception";
  return UA_MAX_TYPES;
}



/* List of parameter names (strings) corresponding to our field indexes;
 * these are also protocol-constants for Measurement Protocol v1.
 * We may eventually migrate these to a separate protocol module. */
static inline void populateParameterNames(const char* params[], const char* custom_params){
  int i, j;
  char* cur;
  params[UA_TRACKING_ID] = "tid";
  params[UA_CLIENT_ID] = "cid";
  params[UA_USER_ID] = "uid";
  params[UA_ANONYMIZE_IP] = "aip";
  params[UA_TRACKING_TYPE] = "t";
  params[UA_DOCUMENT_PATH] = "dp";
  params[UA_DOCUMENT_TITLE] = "dt";
  params[UA_DOCUMENT_LOCATION] = "dl";
  params[UA_DOCUMENT_HOSTNAME] = "dh";
  params[UA_DOCUMENT_REFERRER] = "dr";
  params[UA_DOCUMENT_ENCODING] = "de";
  params[UA_QUEUE_TIME_MS] = "qt";
  params[UA_CACHE_BUSTER] = "z";
  params[UA_SESSION_CONTROL] = "sc";
  params[UA_CAMPAIGN_NAME] = "cn";
  params[UA_CAMPAIGN_SOURCE] = "cs";
  params[UA_CAMPAIGN_MEDIUM] = "cm";
  params[UA_CAMPAIGN_KEYWORD] = "ck";
  params[UA_CAMPAIGN_CONTENT] = "cc";
  params[UA_CAMPAIGN_ID] = "ci";
  params[UA_SCREEN_RESOLUTION] = "sr";
  params[UA_VIEWPORT_SIZE] = "vp";
  params[UA_SCREEN_COLORS] = "sd";
  params[UA_USER_LANGUAGE] = "ul";
  params[UA_USER_AGENT] = "ua";
  params[UA_APP_NAME] = "an";
  params[UA_APP_ID] = "aid";
  params[UA_APP_VERSION] = "av";
  params[UA_APP_INSTALLER_ID] = "aiid";
  params[UA_CONTENT_DESCRIPTION] = "cd";
  params[UA_SCREEN_NAME] = "cd";
  params[UA_EVENT_CATEGORY] = "ec";
  params[UA_EVENT_ACTION] = "ea";
  params[UA_EVENT_LABEL] = "el";
  params[UA_EVENT_VALUE] = "ev";
  params[UA_NON_INTERACTIVE] = "ni";
  params[UA_SOCIAL_ACTION] = "sa";
  params[UA_SOCIAL_NETWORK] = "sn";
  params[UA_SOCIAL_TARGET] = "st";
  params[UA_EXCEPTION_DESCRIPTION] = "exd";
  params[UA_EXCEPTION_FATAL] = "exf";
  params[UA_TRANSACTION_ID] = "ti";
  params[UA_TRANSACTION_AFFILIATION] = "ta";
  params[UA_TRANSACTION_REVENUE] = "tr";
  params[UA_TRANSACTION_SHIPPING] = "ts";
  params[UA_TRANSACTION_TAX] = "tt";
  params[UA_TRANSACTION_CURRENCY] = "cu";
  params[UA_CURRENCY_CODE] = "cu"; /* for compatibility with Google's naming convention */
  params[UA_ITEM_CODE] = "ic" ;
  params[UA_ITEM_NAME] = "in";
  params[UA_ITEM_VARIATION] = "iv"; /* a more literal acronym/alias */
  params[UA_ITEM_CATEGORY] = "iv"; /* for compatibility with Google's naming convention */
  params[UA_ITEM_PRICE] = "ip";
  params[UA_ITEM_QUANTITY] = "iq";
  params[UA_TIMING_CATEGORY] = "utc";
  params[UA_TIMING_VARIABLE] = "utv";
  params[UA_TIMING_LABEL] = "utl";
  params[UA_TIMING_TIME] = "utt";
  params[UA_TIMING_DNS] = "dns";
  params[UA_TIMING_PAGE_LOAD] = "pdt";
  params[UA_TIMING_REDIRECT] = "rrt";
  params[UA_TIMING_TCP_CONNECT] = "tcp";
  params[UA_TIMING_SERVER_RESPONSE] = "srt";
  params[UA_VERSION_NUMBER] = "v";
  params[UA_ADWORDS_ID] = "gclid";
  params[UA_DISPLAY_AD_ID] = "dclid";
  params[UA_LINK_ID] = "linkid";
  params[UA_JAVA_ENABLED] = "je";
  params[UA_FLASH_VERSION] = "fl";
 
  /* Populate dimension space */
  for(i = 0; i < UA_MAX_CUSTOM_DIMENSION; i++){
    cur = (char*) (custom_params + (i * UA_CUSTOM_PARAM_LEN));
    sprintf(cur, "cd%d", i + 1);
    params[ i + UA_START_CDIMENSIONS ] = cur; /* link parameter name */
  }

  /* Populate metric space */
  for(j = 0; j < UA_MAX_CUSTOM_METRIC; j++){
    cur = (char*) (custom_params + ((i + j) * UA_CUSTOM_PARAM_LEN));
    sprintf(cur, "cm%d", j + 1);
    params[ j + UA_START_CMETRICS ] = cur; /* link parameter name */
  }
  
}


/* Retrieve a field name (pointer) by its ID 
 * (and appropriate offset for custom parameters */
static inline const char* getOptionName(const char* field_names[], trackingField_t field, unsigned int slot_id){
  switch(field){
    case UA_CUSTOM_METRIC: 
      return field_names[ UA_START_CMETRICS + slot_id - 1 ];
    case UA_CUSTOM_DIMENSION:
      return field_names[ UA_START_CDIMENSIONS + slot_id - 1 ];
    default:
      return field_names[ field ];
  }
}

static inline int getFieldPosition(trackingField_t field, unsigned int slot_id){
  switch(field){
    case UA_CUSTOM_METRIC:
      return UA_START_CMETRICS + slot_id - 1;
    case UA_CUSTOM_DIMENSION:
      return UA_START_CDIMENSIONS + slot_id - 1;
    default:
      return field;
  }
}



/* Retrieve the tracking-type parameter name (pointer) */
static inline const char* getTrackingType(UATracker_t* tracker, trackingType_t type){
  assert(NULL != tracker);
  assert((*tracker).__configured__ == UA_MEM_MAGIC_CONFIG);

  return tracker->map_types[ type ];
}

/* Void all memory allocated to tracking parameters (pointers) */
static inline void initParameterState(UAParameter_t params[], unsigned int howmany){
  memset(params, 0, howmany * (sizeof(UAParameter_t)));
}

/* Void a tracker's memory */
void cleanTracker(UATracker_t* tracker){
  assert(NULL != tracker);
  if((*tracker).__configured__ == UA_MEM_MAGIC_CONFIG){
    HTTPcleanup(& tracker->queue); // run any queued requests...
  }
  memset(tracker, 0, sizeof(UATracker_t)); 
}


/* Clean out ephemeral state & query cache */
static inline void resetQuery(UATracker_t* tracker){
  initParameterState(tracker->ephemeral_parameters, UA_MAX_PARAMETERS);
  memset(tracker->query, 0, UA_MAX_QUERY_LEN);
  tracker->query_len = 0;
}






/* Define a single parameter's name/value/slot */
static inline void setParameterCore(const char* field_names[], UAParameter_t params[], trackingField_t field, unsigned int slot_id, const char* value){
  int position = getFieldPosition(field, slot_id);
  const char* name = getOptionName(field_names, field, slot_id);
  assert(NULL != name);
  params[ position ].field = field;
  params[ position ].name = name;
  params[ position ].value = (char*) value;
  params[ position ].slot_id = slot_id;
}

/* Populate several parameters (pointers) given a set of options */
static inline void setParameterList(const char* field_names[], UAParameter_t params[], UAOptionNode_t options[], unsigned int howmany){
  unsigned int i;
  for(i = 0; i < howmany; i++){
    if(options[i].field < 1){
      /* Only populate legitimate fields... skip the bad ones (or NULL) */
      continue;
    }
    setParameterCore(field_names, params, options[i].field, options[i].slot_id, options[i].value);
  }
}


/* Populate several lifetime/permanent or temporary/ephemeral values based on scope */
static inline void setParameterStateList(UATracker_t* tracker, stateScopeFlag_t flag, UAOptionNode_t options[]){
  assert(NULL != tracker);
  assert((*tracker).__configured__ == UA_MEM_MAGIC_CONFIG);
  switch(flag){
    case UA_PERMANENT: 
      setParameterList(tracker->map_parameters, tracker->lifetime_parameters, options, UA_MAX_PARAMETERS);
      break;
    case UA_EPHEMERAL:
      setParameterList(tracker->map_parameters, tracker->ephemeral_parameters, options, UA_MAX_PARAMETERS);
      break;
  }
}


/* Populate a single lifetime/permanent or temporary/ephemeral value based on scope */
static inline void setParameterState(UATracker_t* tracker, stateScopeFlag_t flag, trackingField_t field, unsigned int slot_id, const char* value ){
  assert(NULL != tracker);
  assert((*tracker).__configured__ == UA_MEM_MAGIC_CONFIG);
  switch(flag){
    case UA_PERMANENT: 
      setParameterCore(tracker->map_parameters, tracker->lifetime_parameters, field, slot_id, value);
      break;
    case UA_EPHEMERAL:
      setParameterCore(tracker->map_parameters, tracker->ephemeral_parameters, field, slot_id, value);
      break;
  }
}

void setTrackerOption(UATracker_t* tracker, UATrackerOption_t option, int value){
  assert(NULL != tracker);
  assert(UA_MAX_TRACKER_OPTION > (int) option);
  assert(0 <= option);
  tracker->options[ option ] = value;
}

int getTrackerOption(UATracker_t* tracker, UATrackerOption_t option){
  assert(NULL != tracker);
  return tracker->options[ option ];
}


/* Set up an already-allocated tracker
 *  - Clear out the whole tracker space
 *  - Populate parameter names
 *  - Define lifetime tracker values
 */
void initTracker(UATracker_t* tracker, const char* trackingId, const char* clientId, const char* userId){
  assert(NULL != tracker);
  cleanTracker(tracker);
  
  (*tracker).__configured__ = UA_MEM_MAGIC_CONFIG;

  tracker->user_agent = (char*) UA_USER_AGENT_DEFAULT;
  
  populateTypeNames(tracker->map_types);
  populateParameterNames(tracker->map_parameters, tracker->map_custom);

  memset(& tracker->query, 0, UA_MAX_QUERY_LEN);

  HTTPsetup(& tracker->queue);


  setParameterCore(tracker->map_parameters, tracker->lifetime_parameters, UA_VERSION_NUMBER, 0, UA_PROTOCOL_VERSION);
  setParameterCore(tracker->map_parameters, tracker->lifetime_parameters, UA_TRACKING_ID, 0, trackingId);
  setParameterCore(tracker->map_parameters, tracker->lifetime_parameters, UA_CLIENT_ID, 0, clientId);
  setParameterCore(tracker->map_parameters, tracker->lifetime_parameters, UA_USER_ID, 0, userId);

  setTrackerOption(tracker, UA_OPTION_QUEUE, UA_DEFAULT_OPTION_QUEUE);

}

/* Allocate space for a tracker & initialize it */
UATracker_t* createTracker(const char* trackingId, const char* clientId, const char* userId){
  UATracker_t* new_tracker = (UATracker_t*)malloc(sizeof(UATracker_t));
  initTracker(new_tracker, trackingId, clientId, userId);
  return new_tracker;
}


/* Clear and de-allocate a tracker */
void removeTracker(UATracker_t* tracker){
  assert(NULL != tracker);
  cleanTracker(tracker);
  free(tracker);
}

/* Wrapper: set up lifetime options on a tracker */
void setParameters(UATracker_t* tracker, UAOptions_t* opts){
  setParameterStateList(tracker, UA_PERMANENT, opts->options);
}

/* Wrapper: set up a single lifetime option on a tracker */
void setParameter(UATracker_t* tracker, trackingField_t field, unsigned int slot_id, const char* value){
  setParameterState(tracker, UA_PERMANENT, field, slot_id, value);
}

/* Retrieve name and value for a given index (transcending ephemeral state to lifetime, if needed) */
void getCurrentParameterValue(UATracker_t* tracker, unsigned int index, const char** name, const char** value){
  assert(NULL != tracker);
  
  (*name) = tracker->ephemeral_parameters[ index ].name;
  (*value) = tracker->ephemeral_parameters[ index ].value;
  if(NULL == (*name) || NULL == (*value)){
    (*name) = tracker->lifetime_parameters[ index ].name;
    (*value) = tracker->lifetime_parameters[ index ].value;
  }
}

/* Construct a query-string based on tracker state */
unsigned int assembleQueryString(UATracker_t* tracker, char* query, unsigned int offset){
  unsigned int i;
  const char* name;
  const char* value;
  int name_len;
  char* current;
  unsigned int value_len;

  // Shortcut for client_id for more readable assertion below
  const char* client_id = tracker->map_parameters[UA_CLIENT_ID];


  for(i = 0; i < UA_MAX_PARAMETERS; i++){
    
    getCurrentParameterValue(tracker, i, & name, & value);

    // Validate parameters given
    assert((name == client_id) ? (value != NULL) : 1);
    if(NULL == name || NULL == value)  continue;

    name_len = (int)strlen(name);
    value_len = (int)strlen(value);

    if(i > 0){
      strncpy(query + offset, "&", 1);
      offset++;
    }

    strncpy(query + offset, name, name_len);
    strncpy(query + offset + name_len, "=", 1);

    /* Fill in the encoded values */
    current = (query + offset + name_len + 1);
    value_len = encodeURIComponent(value, current, value_len, (UA_MAX_QUERY_LEN - ((unsigned int)(current - query))));
    
    
    offset += (name_len + value_len + 1);
  }
  return offset; 
}


/* Assemble a query from a tracker and send it through CURL */
void queueTracking(UATracker_t* tracker){
  assert(NULL != tracker);
  assert((*tracker).__configured__ == UA_MEM_MAGIC_CONFIG);

  unsigned int query_len;
  char* query = tracker->query;
  memset(query, 0, UA_MAX_QUERY_LEN);
  query_len = assembleQueryString(tracker, query, 0);

  HTTPenqueue(& tracker->queue, UA_ENDPOINT, tracker->user_agent, query, query_len); 
}

/* Prepare ephemeral state on a tracker and dispatch its query */
void sendTracking(UATracker_t* tracker, trackingType_t type, UAOptions_t* opts){
  assert(NULL != tracker);
  assert((*tracker).__configured__ == UA_MEM_MAGIC_CONFIG);


  if(NULL != opts){
    setParameterStateList(tracker, UA_EPHEMERAL, opts->options);
  }

  setParameterState(tracker, 
      UA_EPHEMERAL, UA_TRACKING_TYPE, 0, 
      getTrackingType(tracker, type)
  );

  queueTracking(tracker);
  
  if(getTrackerOption(tracker, UA_OPTION_QUEUE) == 0){
    HTTPflush(& tracker->queue);
  }
  
  
  resetQuery(tracker); 

}


