#ifndef JSON_H__
#define JSON_H__

#include <stddef.h> /* size_t */

typedef enum { JSON_NULL, JSON_FALSE, JSON_TRUE, JSON_NUMBER, JSON_STRING, JSON_ARRAY, JSON_OBJECT } json_type;

typedef struct json_value json_value;
typedef struct json_member json_member;
/*
  ----------------------
  | ele  | size |      |
  |--------------      |
  | str  | len  | type |
  |--------------      |
  |   double    |      |
  ----------------------
*/

struct json_value {
    union {
        struct {
            json_member* mem;
            size_t msize;
        };
        struct {
            json_value* ele;
            size_t size; /*元素个数*/
        };
        struct {
            char* str;
            size_t len;
        };
        double num;
    };
    json_type type;
};

struct json_member {
    char* key; /* member key string */
    size_t keylen; /* key string length */
    json_value value; /* member value */
};

enum {
    JSON_PARSE_OK = 0,
    JSON_PARSE_EXPECT_VALUE,
    JSON_PARSE_INVALID_VALUE,
    JSON_PARSE_ROOT_NOT_SINGULAR,
    JSON_PARSE_NUMBER_TOO_BIG,
    JSON_PARSE_MISS_QUOTATION_MARK,
    JSON_PARSE_INVALID_STRING_ESCAPE,
    JSON_PARSE_INVALID_STRING_CHAR,
    JSON_PARSE_INVALID_UNICODE_HEX,
    JSON_PARSE_INVALID_UNICODE_SURROGATE,
    JSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET,
    JSON_PARSE_MISS_KEY,
    JSON_PARSE_MISS_COLON,
    JSON_PARSE_MISS_COMMA_OR_CURLY_BRACKET,
    JSON_STRINGIFY_OK,
    JSON_PARSE_STRINGIFY_INIT_SIZE
};

#define json_init(value)    do { (value)->type = JSON_NULL; } while(0)

int json_parse(json_value* value, const char* json);

void json_free(json_value* value);

json_type json_get_type(const json_value* value);

#define json_set_null(value) json_free(value)

int json_get_boolean(const json_value* value);
void json_set_boolean(json_value* value, int boolean);

double json_get_number(const json_value* value);
void json_set_number(json_value* value, double number);

const char* json_get_string(const json_value* value);
size_t json_get_string_length(const json_value* value);
void json_set_string(json_value* value, const char* str, size_t len);

size_t json_get_array_size(const json_value* value);
json_value* json_get_array_element(const json_value* value, size_t index);

size_t json_get_object_size(const json_value* value);
const char* json_get_object_key(const json_value* value, size_t index);
size_t json_get_object_key_length(const json_value* value, size_t index);
json_value* json_get_object_value(const json_value* value, size_t index);

int json_stringify(const json_value* value, char** json, size_t* length);

#endif /* JSON_H__ */
