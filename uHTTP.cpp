#include "uHTTP.h"

/**
 *	uHTTP constructor
 **/
uHTTP::uHTTP() : EthernetServer(80){
	uHTTP(80);
}

/**
 *	uHTTP constructor
 *
 *	@param uint16_t port
 **/
uHTTP::uHTTP(uint16_t port) : EthernetServer(port){
	//__name = F("uHTTP");
	__uri = new char[uHTTP_URI_SIZE];
  	__query = new char[uHTTP_QUERY_SIZE];
  	__body = new char[uHTTP_BODY_SIZE];
}

/**
 *	uHTTP destructor
 **/
uHTTP::~uHTTP(){
  	delete [] __uri;
  	delete [] __query;
  	delete [] __body;
}

/**
 *	Process HTTP request
 *
 *	@return EthernetClient client
 **/
EthernetClient uHTTP::available(){
	EthernetClient client;

	memset(__uri, 0, sizeof(__uri));
    memset(__query, 0, sizeof(__query));
    memset(__body, 0, sizeof(__body));

	if(client = EthernetServer::available()){
		uint8_t cursor = 0, cr = 0;
		char buffer[uHTTP_BUFFER_SIZE] = {0};
		bool sub = false;

		enum state_t {METHOD, URI, QUERY, PROTO, KEY, VALUE, BODY};
    	state_t state = METHOD;

    	enum header_t {START, AUTHORIZATION, CONTENT_TYPE, CONTENT_LENGTH, ORIGIN};
    	header_t header = START;

		while(client.connected() && client.available()){
      		char c = client.read();

      		(c == '\r' || c == '\n') ? cr++ : cr = 0;
      		
      		switch(state){
      			case METHOD:
            		if(c == ' '){
              			if(strncmp_P(buffer, PSTR("OP"), 2) == 0) __method = uHTTP_METHOD_OPTIONS;
              			else if(strncmp_P(buffer, PSTR("HE"), 2) == 0) __method = uHTTP_METHOD_HEAD;
              			else if(strncmp_P(buffer, PSTR("PO"), 2) == 0) __method = uHTTP_METHOD_POST;
              			else if(strncmp_P(buffer, PSTR("PU"), 2) == 0) __method = uHTTP_METHOD_PUT;
              			else if(strncmp_P(buffer, PSTR("PA"), 2) == 0) __method = uHTTP_METHOD_PATCH;
              			else if(strncmp_P(buffer, PSTR("DE"), 2) == 0) __method = uHTTP_METHOD_DELETE;
              			else if(strncmp_P(buffer, PSTR("TR"), 2) == 0) __method = uHTTP_METHOD_TRACE;
              			else if(strncmp_P(buffer, PSTR("CO"), 2) == 0) __method = uHTTP_METHOD_CONNECT;
              			else __method = uHTTP_METHOD_GET;
              			state = URI; 
              			cursor = 0; 
            		}else if(cursor < uHTTP_METHOD_SIZE - 1){
            			buffer[cursor++] = c; 
            			buffer[cursor] = '\0'; 
            		}
            		break;
            	case URI:
            		if(c == ' '){
            			state = PROTO;
            			cursor = 0;
            		}else if(c == '?'){
            			state = QUERY;
            			cursor = 0;
            		}else if(cursor < uHTTP_URI_SIZE - 1){
            			__uri[cursor++] = c;
            			__uri[cursor] = '\0';
            		}
            		break;
            	case QUERY:
            		if(c == ' '){
            			state = PROTO;
            			cursor = 0;
            		}else if(cursor < uHTTP_QUERY_SIZE - 1){
            			__query[cursor++] = c;
            			__query[cursor] = '\0';
            		}
            		break;
            	case PROTO:
            		if(cr == 2){ state = KEY; cursor = 0; }
            		break;
            	case KEY:
            		if (cr == 4){ state = BODY; cursor = 0; }
            		else if(c == ' '){ state = VALUE; cursor = 0; }
            		else if(c != ':' && cursor < uHTTP_BUFFER_SIZE){ buffer[cursor++] = c; buffer[cursor] = '\0'; }
            		break;
            	case VALUE:
            		if(cr == 2){
            			switch(header){
                			case AUTHORIZATION:
                  				strncpy(__head.auth, buffer, uHTTP_AUTH_SIZE);
                  				break;
                			case CONTENT_TYPE:
                				strncpy(__head.type, buffer, uHTTP_TYPE_SIZE);
                  				break;
                			case ORIGIN:
                  				strncpy(__head.orig, buffer, uHTTP_ORIG_SIZE);
                  				break;
                  			case CONTENT_LENGTH:
                  				__head.length = atoi(buffer);
                  				break;
                  			break;
              			}
            			state = KEY; header = START; cursor = 0; sub = false;
            		}else{
            			if(header == START){
                			if(strncmp_P(buffer, PSTR("Auth"), 4) == 0) header = AUTHORIZATION;
                			else if(strncmp_P(buffer, PSTR("Content-T"), 9) == 0) header = CONTENT_TYPE;
                			else if(strncmp_P(buffer, PSTR("Content-L"), 9) == 0) header = CONTENT_LENGTH;
              			}

              			// Fill buffer
              			if(cursor < uHTTP_BUFFER_SIZE - 1 && c != '\r' && c!= '\n'){
                			switch(header){
                  				case AUTHORIZATION:
                    				if(sub){ buffer[cursor++] = c; buffer[cursor] = '\0'; }
                    				else if(c == ' ') sub = true;
                    				break;
                  				case CONTENT_TYPE:
                  				case CONTENT_LENGTH:
                    				buffer[cursor++] = c; buffer[cursor] = '\0';
                    				break;
                			}
              			}
            		}
            		break;
            	case BODY:
            		if(cr == 2 || __head.length == 0) client.flush();
            		else if(cursor < uHTTP_BODY_SIZE - 1){ __body[cursor++] = c; __body[cursor] = '\0'; }
            		break;
      		}
      	}
	}
	return client;
}

/**
 *	Get request method
 *
 *	@return uint8_t method
 **/
uint8_t uHTTP::method(){
	return __method;
}

/**
 *	Check if request method is equals to type passed
 *
 *	@param uint8_t type
 *	@return boolean
 **/
bool uHTTP::method(uint8_t type){
	return (__method == type) ? true : false;
}

/**
 *	Get request uri
 *
 *	@return *char uri
 **/
char *uHTTP::uri(){
	return __uri;
}

/**
 *	Get request uri segment
 *
 *	@param uint8_t index
 *	@return *char segment
 **/
char* uHTTP::uri(uint8_t index){
	char *act, *segment, *ptr;
	char buffer[uHTTP_URI_SIZE];
	uint8_t i;
	strcpy(buffer, __uri);
	for(i = 1, act = buffer; i <= index; i++, act = NULL) {
		segment = strtok_rP(act, PSTR("/"), &ptr);
		if(segment == NULL) break;
	}
	return segment;
}

/**
 *	Check if request uri is equas to passed uri
 *
 *	@param char *uri
 *	@return boolean
 **/
bool uHTTP::uri(const char *uri){
	return (strcmp(this->uri(), uri) == 0) ? true : false;
}

/**
 *	Check if request uri is equas to passed uri
 *
 *	@param const __FlashStringHelper *uri
 *	@return boolean
 **/
bool uHTTP::uri(const __FlashStringHelper *uri){
	return (strcmp_P(this->uri(), (const PROGMEM char *) uri) == 0) ? true : false;
}

/**
 *	Check if request uri segment is equas to passed uri
 *
 *	@param uint8_t index
  *	@param char *uri
 *	@return boolean
 **/
bool uHTTP::uri(uint8_t index, const char *uri){
	return (strcmp(this->uri(index), uri) == 0) ? true : false;
}

/**
 *	Check if request uri segment is equas to passed uri
 *
 *	@param uint8_t index
 *	@param const __FlashStringHelper *uri
 *	@return boolean
 **/
bool uHTTP::uri(uint8_t index, const __FlashStringHelper *uri){
	return (strcmp_P(this->uri(index), (const PROGMEM char *) uri) == 0) ? true : false;
}

/**
 *	Get request query string
 *
 *	@return *char query
 **/
char *uHTTP::query(){
	return __query;
}

/**
 *	Get request query string value by key
 *
 *	@param const __FlashStringHelper *key
 *	@return *char query value
 **/
char *uHTTP::query(const char *key){
	return parse(key, __query, F("&"));
}

/**
 *	Get request query string value by key
 *
 *	@param const __FlashStringHelper *key
 *	@return *char query value
 **/
char *uHTTP::query(const __FlashStringHelper *key){
	return parse(key, __query, F("&"));
}

/**
 *	Get request headers
 *
 *	@return header_t header
 **/
header_t uHTTP::head(){
	return __head;
}

/**
 *	Get request body
 *
 *	@return *char body
 **/
char *uHTTP::body(){
	return __body;
}

/**
 *	Get request data value by key
 *
 *	@param const __FlashStringHelper *key
 *	@return *char value
 **/
char *uHTTP::data(const char *key){
	return parse(key, __body, F("&"));
}

/**
 *	Get request data value by key
 *
 *	@param const __FlashStringHelper *key
 *	@return *char value
 **/
char *uHTTP::data(const __FlashStringHelper *key){
	return parse(key, __body, F("&"));
}

/**
 *	Find needle on haystack
 *
 *	@param const char *needle
 *	@param char *haystack
 *	@param __FlashStringHelper *separator
 *	@return *char value
 **/
char *uHTTP::parse(const char *needle, char *haystack, const __FlashStringHelper *sep){
	char *act, *sub, *ptr;
	char buffer[uHTTP_BUFFER_SIZE];
	strcpy(buffer, haystack);
	for(act = buffer; strncmp(sub, needle, strlen(needle)); act = NULL){
		sub = strtok_rP(act, (const PROGMEM char *) sep, &ptr);
		if(sub == NULL) break;
	}
	return (sub) ? strchr(sub, '=') + 1 : NULL;
}

/**
 *	Find needle on haystack
 *
 *	@param const __FlashStringHelper *needle
 *	@param char *haystack
 *	@param __FlashStringHelper *separator
 *	@return *char value
 **/
char *uHTTP::parse(const __FlashStringHelper *needle, char *haystack, const __FlashStringHelper *sep){
	char *act, *sub, *ptr;
	char buffer[uHTTP_BUFFER_SIZE];
	strcpy(buffer, haystack);
	for(act = buffer; strncmp_P(sub, (const PROGMEM char *) needle, strlen_P((const PROGMEM char *) needle)); act = NULL){
		sub = strtok_rP(act, (const PROGMEM char *) sep, &ptr);
		if(sub == NULL) break;
	}
	return (sub) ? strchr(sub, '=') + 1 : NULL;
}