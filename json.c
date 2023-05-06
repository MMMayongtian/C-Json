#include "json.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)

typedef struct {
    const char* json;
}json_context;

static void json_parse_whitespace(json_context* context) {
    const char *p = context->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
        p++;
    }
    context->json = p;
}

/* null  = "null" */
static int json_parse_null(json_context* context, json_value* value) {
    const char cnull[5] = "null";
    if (strncmp(context->json, cnull, 4)) {
        return JSON_PARSE_INVALID_VALUE;
    }
    context->json += 4;
    value->type = JSON_NULL;
    return JSON_PARSE_OK;
}

/* value = null / false / true */
static int json_parse_value(json_context* context, json_value* value) {
    switch (*context->json) {
        case 'n': return json_parse_null(context, value);
        case '\0': return JSON_PARSE_EXPECT_VALUE;
        default: return LEPT_PARSE_INVALID_VALUE;
    }
}

int json_parse(json_value* value, const char* json) {
    json_context context;
    assert(value != NULL);
    context.json = json;
    value->type = LEPT_NULL;
    lept_parse_whitespace(&context);
    return lept_parse_value(&context, value);
}

json_type json_get_type(const json_value* value) {
    assert(value != NULL);
    return value->type;
}
double json_get_number(const json_value* value) {
    assert(value != NULL && value->type == JSON_NUMBER);
    return value->num;
}