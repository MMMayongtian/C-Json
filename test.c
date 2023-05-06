#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "json.h"

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format) \
    do {\
        test_count++;\
        if (equality)\
            test_pass++;\
        else {\
            fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);\
            main_ret = 1;\
        }\
    } while(0)

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")

static void test_parse_null() {
    json_value value;
    value.type = JSON_TRUE;
    EXPECT_EQ_INT(JSON_PARSE_OK, json_parse(&value, "null"));
    EXPECT_EQ_INT(JSON_NULL, json_get_type(&value));
}

static void test_parse_expect_value() {
    json_value value;

    value.type = JSON_FALSE;
    EXPECT_EQ_INT(JSON_PARSE_EXPECT_VALUE, json_parse(&value, ""));
    EXPECT_EQ_INT(JSON_NULL, json_get_type(&value));

    value.type = JSON_FALSE;
    EXPECT_EQ_INT(JSON_PARSE_EXPECT_VALUE, json_parse(&value, " "));
    EXPECT_EQ_INT(JSON_NULL, json_get_type(&value));
}

static void test_parse_invalid_value() {
    json_value value;
    value.type = JSON_FALSE;
    EXPECT_EQ_INT(JSON_PARSE_INVALID_VALUE, json_parse(&value, "nul"));
    EXPECT_EQ_INT(JSON_NULL, json_get_type(&value));

    value.type = JSON_FALSE;
    EXPECT_EQ_INT(JSON_PARSE_INVALID_VALUE, json_parse(&value, "?"));
    EXPECT_EQ_INT(JSON_NULL, json_get_type(&value));
}

static void test_parse_root_not_singular() {
    json_value value;
    value.type = JSON_FALSE;
    EXPECT_EQ_INT(JSON_PARSE_ROOT_NOT_SINGULAR, json_parse(&value, "null x"));
    EXPECT_EQ_INT(JSON_NULL, json_get_type(&value));
}

static void test_parse() {
    test_parse_null();
    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
}

int main() {
    test_parse();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}