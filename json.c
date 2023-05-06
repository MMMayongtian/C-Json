#include "json.h"
#include <assert.h>
#include <errno.h>   /* errno, ERANGE */
#include <math.h>    /* HUGE_VAL */
#include <stdlib.h>
#include <string.h>

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')

typedef struct {
    const char* json;
}json_context;

/* ws = *(%x20 / %x09 / %x0A / %x0D) */
static void json_parse_whitespace(json_context* context) {
    const char *p = context->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
        p++;
    }
    context->json = p;
}

static int json_parse_literal(json_context* context, json_value* value, const char* literal, json_type type) {
    size_t len = strlen(literal);
    if (strncmp(context->json, literal, len)) {
        return JSON_PARSE_INVALID_VALUE;
    }
    context->json += len;
    value->type = type;
    return JSON_PARSE_OK;
}

/* null  = "null" */
static int json_parse_null(json_context* context, json_value* value) {
    /* EXPECT(context, 'n');
    if (context->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
        return JSON_PARSE_INVALID_VALUE;
    context->json += 3;
    value->type = JSON_NULL;
    return JSON_PARSE_OK; */

    const char cnull[5] = "null";
    if (strncmp(context->json, cnull, 4)) {
        return JSON_PARSE_INVALID_VALUE;
    }
    context->json += 4;
    value->type = JSON_NULL;
    return JSON_PARSE_OK;
}

/* false  = "false" */
static int json_parse_false(json_context* context, json_value* value) {
    const char cfalse[6] = "false";
    if(strncmp(context->json, cfalse, 5)) {
        return JSON_PARSE_INVALID_VALUE;
    }
    context->json += 5;
    value->type = JSON_FALSE;
    return JSON_PARSE_OK;
}

/* true  = "true" */
static int json_parse_true(json_context* context, json_value* value) {
    const char ctrue[5] = "true";
    if(strncmp(context->json, ctrue, 4)) {
        return JSON_PARSE_INVALID_VALUE;
    }
    context->json += 4;
    value->type = JSON_TRUE;
    return JSON_PARSE_OK;
}

static int json_parse_number(json_context* context, json_value* value) {
    const char* p;
    p = context->json;
    /* 负号 ... */
    if (*p == '-') p++;
    /* 整数 ... */
    if (*p == '0') p++;
    else {
        if(!ISDIGIT1TO9(*p)) return JSON_PARSE_INVALID_VALUE;
        while(ISDIGIT(*p)) p++;
    }
    /* 小数 ... */
    if (*p == '.') {
        p++;
        if (!ISDIGIT(*p)) return JSON_PARSE_INVALID_VALUE;
        while (ISDIGIT(*p)) p++;
    }
    /* 指数 ... */
    if (*p == 'e' || *p == 'E') {
        p++;
        if (*p == '+' || *p == '-') p++;
        if (!ISDIGIT(*p)) return JSON_PARSE_INVALID_VALUE;
        while (ISDIGIT(*p)) p++;
    }

    errno = 0;
    value->num = strtod(context->json, NULL);
    if (errno == ERANGE && (value->num == HUGE_VAL || value->num == -HUGE_VAL))
        return JSON_PARSE_NUMBER_TOO_BIG;
    value->type = JSON_NUMBER;
    context->json = p;
    return JSON_PARSE_OK;
}

/* value = null / false / true / numver */
static int json_parse_value(json_context* context, json_value* value) {
    switch (*context->json) {
        case 'n': return json_parse_literal(context, value, "null", JSON_NULL);
        case 'f': return json_parse_literal(context, value, "false", JSON_FALSE);
        case 't': return json_parse_literal(context, value, "true", JSON_TRUE);
        default: return json_parse_number(context, value);
        case '\0': return JSON_PARSE_EXPECT_VALUE;
    }
}

int json_parse(json_value* value, const char* json) {
    json_context context;
    int ret;
    
    assert(value != NULL);
    context.json = json;
    value->type = JSON_NULL;
    json_parse_whitespace(&context);
    ret = json_parse_value(&context, value);
    if (ret == JSON_PARSE_OK) {
        json_parse_whitespace(&context);
        if (*context.json != '\0') {
            value->type = JSON_NULL;
            return JSON_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

json_type json_get_type(const json_value* value) {
    assert(value != NULL);
    return value->type;
}
double json_get_number(const json_value* value) {
    assert(value != NULL && value->type == JSON_NUMBER);
    return value->num;
}