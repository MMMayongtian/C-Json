#ifndef JSON_H__
#define JSON_H__

typedef enum { JSON_NULL, JSON_FALSE, JSON_TRUE, JSON_NUMBER, JSON_STRING, JSON_ARRAY, JSON_OBJECT} json_type;

typedef struct {
    double num;
    json_type type;
} json_value;

enum {
    JSON_PARSE_OK = 0,
    JSON_PARSE_EXPECT_VALUE,
    JSON_PARSE_INVALID_VALUE,
    JSON_PARSE_ROOT_NOT_SINGULAR
};

int json_parse(json_value* value, const char* json);
json_type json_get_type(const json_value* value);

#endif /* JSON_H__ */
