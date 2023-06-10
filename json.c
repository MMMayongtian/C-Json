#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "json.h"
#include <assert.h>
#include <errno.h>   /* errno, ERANGE */
#include <math.h>    /* HUGE_VAL */
#include <stdlib.h>  /* NULL, malloc(), realloc(), free(), strtod() */
#include <string.h>  /* memcpy() */

#ifndef JSON_PARSE_STACK_INIT_SIZE
#define JSON_PARSE_STACK_INIT_SIZE 256
#endif

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); /*c->json++;*/ } while(0)

#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')
#define ISHEX(ch)           ISDIGIT(ch) || ((ch) >= 'a' && (ch) <= 'f') || ((ch) >= 'A' && (ch) <= 'F')

#define PUTC(c, ch)         do { *(char*)json_context_push(c, sizeof(char)) = (ch); } while(0)

typedef struct {
    const char* json;
    char* stack;
    size_t size; /* 当前容量 */
    size_t top;  /* 栈顶 */
}json_context;


/* stack */
static void* json_context_push(json_context* context, size_t size) {
    void* ret;
    assert(size > 0);
    if (context->top + size >= context->size) {
        if (context->size == 0){ /* init */
            context->size = JSON_PARSE_STACK_INIT_SIZE;
        }
        while (context->top + size >= context->size) {/* expand 1.5 times */
            context->size += context->size >> 1;
        }
        context->stack = (char*) realloc(context->stack, context->size);
    }
    ret = context->stack + context->top;
    context->top += size;
    return ret;
}

static void* json_context_pop(json_context* context, size_t size) {
    assert(context->top >= size);
    return context->stack + (context->top -= size);
}

/* ws = *(%x20 / %x09 / %x0A / %x0D) */
static void json_parse_whitespace(json_context* context) {
    const char *p = context->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
        p++;
    }
    context->json = p;
}

static int json_parse_literal(json_context* context, json_value* value, const char* literal, json_type type) {
    EXPECT(context, literal[0]);
    size_t len = strlen(literal);
    if (strncmp(context->json, literal, len)) {
        return JSON_PARSE_INVALID_VALUE;
    }
    context->json += len;
    value->type = type;
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

static const char* json_parse_hex4(const char* p,  unsigned* u) {
    *u = 0;
    for (int i = 0; i < 4; i++) {
        char ch = *p++;
        *u <<= 4;
        if      (ch >= '0' && ch <= '9') *u |= ch - '0';
        else if (ch >= 'a' && ch <= 'f') *u |= ch - ('a' - 10);
        else if (ch >= 'A' && ch <= 'F') *u |= ch - ('A' - 10);
        else return NULL;
    }
    return p;

    /* 也可直接使用 strtol() 函数 但是需要检测特使情况*/
}

static void json_encode_utf8(json_context* c, unsigned u) {
    if      (u <= 0x7F){
        PUTC(c, u & 0xFF);
    }
    else if (u <= 0x7FF) {
        PUTC(c, 0xC0 | ((u >> 6) & 0XFF));
        PUTC(c, 0x80 | ( u       & 0x3F));
    }
    else if (u <= 0xFFFF) {
        PUTC(c, 0xE0 | ((u >> 12) & 0xFF));
        PUTC(c, 0x80 | ((u >> 6 ) & 0x3F));
        PUTC(c, 0x80 | ( u        & 0x3F));
    }
    else {
        assert(u <= 0x10FFFF);
        PUTC(c, 0xF0 | ((u >> 18) & 0xFF));
        PUTC(c, 0x80 | ((u >> 12) & 0x3F));
        PUTC(c, 0x80 | ((u >>  6) & 0x3F));
        PUTC(c, 0x80 | ( u        & 0x3F));
    }
}

#define STRING_ERROR(c, ret)   do { c->top = head; return ret; } while(0)

static int json_parse_string(json_context* context, json_value* value) {
    size_t head;
    size_t len;
    unsigned u, u2;
    const char* p;

    head = context->top;
    EXPECT(context, '\"');
    p = context->json + 1;

    while (1) {
        char ch = *p++;
        switch(ch) {
            case '\"': 
                len = context->top - head;
                json_set_string(value, (const char*)json_context_pop(context, len), len);
                context->json = p;
                return JSON_PARSE_OK;
            case '\\':
                switch(*p++) {
                    case '\"': PUTC(context, '\"'); break;
                    case '\\': PUTC(context, '\\'); break;
                    case '/':  PUTC(context, '/' ); break;
                    case 'b':  PUTC(context, '\b'); break;
                    case 'f':  PUTC(context, '\f'); break;
                    case 'n':  PUTC(context, '\n'); break;
                    case 'r':  PUTC(context, '\r'); break;
                    case 't':  PUTC(context, '\t'); break;
                    case 'u':  
                        if (!(p = json_parse_hex4(p, &u)))
                            STRING_ERROR(context, JSON_PARSE_INVALID_UNICODE_HEX);
                        if (u >= 0xD800 && u <= 0xDBFF) { /* surrogate pair */
                            if (*p++ != '\\')
                                STRING_ERROR(context, JSON_PARSE_INVALID_UNICODE_SURROGATE);
                            if (*p++ != 'u')
                                STRING_ERROR(context, JSON_PARSE_INVALID_UNICODE_SURROGATE);
                            if (!(p = json_parse_hex4(p, &u2)))
                                STRING_ERROR(context, JSON_PARSE_INVALID_UNICODE_HEX);
                            if (u2 < 0xDC00 || u2 > 0xDFFF)
                                STRING_ERROR(context, JSON_PARSE_INVALID_UNICODE_SURROGATE);
                            u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
                        }
                        json_encode_utf8(context, u);
                        break;
                    default:
                        STRING_ERROR(context, JSON_PARSE_INVALID_STRING_ESCAPE);
                }
                break;
            case '\0':
                STRING_ERROR(context, JSON_PARSE_MISS_QUOTATION_MARK);
            default:
                if((unsigned char) ch < 0x20) 
                    STRING_ERROR(context, JSON_PARSE_INVALID_STRING_CHAR);
                PUTC(context, ch);
        }
    }
}

static int json_parse_value(json_context* context, json_value* value); /* 前向声明 */

static int json_parse_array(json_context* context, json_value* value) {
    size_t size = 0;
    int ret;

    EXPECT(context, '[');
    context->json++;
    json_parse_whitespace(context);
    if(*context->json == ']') {
        context->json++;
        value->type = JSON_ARRAY;
        value->size = 0;
        value->ele = NULL;
        return JSON_PARSE_OK;
    }
    while(1) {
        /* json_value* ele = json_context_push(context, sizeof(json_value));
         bug! json_parse_value() 会调用 json_context_push() 
         栈满时会 realloc() 扩容, 最初 ele 失效 成为悬挂指针 
         使用 C++ STL 时同样要注意 迭代器(iterator) 在修改容器内容后可能会失效*/
        json_value ele;
        json_init(&ele);
        if ((ret = json_parse_value(context, &ele)) != JSON_PARSE_OK)
            return ret;
        memcpy(json_context_push(context, sizeof(json_value)), &ele, sizeof(json_value));
        size++;
        json_parse_whitespace(context);
        if (*context->json ==',') {
            context->json++;
            json_parse_whitespace(context);
        }
        else if(*context->json ==']') {
            context->json++;
            value->type = JSON_ARRAY;
            value->size = size;
            size *= sizeof(json_value);
            value->ele = (json_value*)malloc(size);
            memcpy(value->ele, json_context_pop(context, size), size);
            return JSON_PARSE_OK;
        }
        else {
            ret = JSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
            break;
        }
    }
    /* Pop and free values on the stack */
    
}
/* value = null / false / true / numver */
static int json_parse_value(json_context* context, json_value* value) {
    switch (*context->json) {
        case 'n':  return json_parse_literal(context, value, "null", JSON_NULL);
        case 'f':  return json_parse_literal(context, value, "false", JSON_FALSE);
        case 't':  return json_parse_literal(context, value, "true", JSON_TRUE);
        default:   return json_parse_number(context, value);
        case '"':  return json_parse_string(context, value);
        case '[':  return json_parse_array(context, value);
        case '\0': return JSON_PARSE_EXPECT_VALUE;
    }
}

int json_parse(json_value* value, const char* json) {
    json_context context;
    int ret;
    assert(value != NULL);
    context.json = json;
    context.stack = NULL;
    context.size = context.top = 0;
    json_init(value);
    json_parse_whitespace(&context);
    ret = json_parse_value(&context, value);
    if (ret == JSON_PARSE_OK) {
        json_parse_whitespace(&context);
        if (*context.json != '\0') {
            value->type = JSON_NULL;
            return JSON_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    assert(context.top == 0);
    free(context.stack);
    return ret;
}

void json_free(json_value* value) {
    assert(value != NULL);
    /* JSON_STRING => JSON_NULL 避免重复释放 */
    if (value->type == JSON_STRING)
        free(value->str);
    else if (value->type == JSON_ARRAY)
        free(value->ele);
    value->type = JSON_NULL;
}

int json_get_boolean(const json_value* value) {
    assert(value != NULL && (value->type == JSON_TRUE || value->type == JSON_FALSE));
    return value->num;
}

void json_set_boolean(json_value* value, int boolean) {
    assert(value != NULL);
    if(boolean == 0)
        value->type = JSON_FALSE;
    else
        value->type = JSON_TRUE;
    value->num = boolean;
}


const char* json_get_string(const json_value* value) {
    assert(value != NULL && value->type == JSON_STRING);
    return value->str;
}

size_t json_get_string_length(const json_value* value) {
    assert(value != NULL && value->type == JSON_STRING);
    return value->len;
}

void json_set_string(json_value* value, const char* str, size_t len) {
    /* 非空指针(有具体字符串) 或零长度字符串均合法 */
    assert(value != NULL && (str != NULL || len == 0));
    json_free(value);
    value->str = (char*)malloc(len + 1);
    memcpy(value->str, str, len);
    value->str[len] = '\0';
    value->len = len;
    value->type = JSON_STRING;
}

double json_get_number(const json_value* value) {
    assert(value != NULL && value->type == JSON_NUMBER);
    return value->num;
}

void json_set_number(json_value* value, double number) {
    assert(value != NULL);
    value->num = number;
    value->type = JSON_NUMBER;
}

json_type json_get_type(const json_value* value) {
    assert(value != NULL);
    return value->type;
}

size_t json_get_array_size(const json_value* value) {
    assert(value != NULL && value->type ==JSON_ARRAY);
    return value->size;
}

json_value* json_get_array_element(const json_value* value, size_t index) {
    assert(value != NULL && value->type == JSON_ARRAY);
    assert(index < value->size);
    return &((value->ele)[index]);
}
