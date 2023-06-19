#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

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
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17g");
#define EXPECT_EQ_STRING(expect, actual, length) EXPECT_EQ_BASE(sizeof(expect) - 1 == (length) && memcmp(expect, actual, length) == 0, expect, actual, "%s")
#define EXPECT_TRUE(actual) EXPECT_EQ_BASE((actual) != 0, "true", "false", "%s")
#define EXPECT_FALSE(actual) EXPECT_EQ_BASE((actual) == 0, "false", "true", "%s")

static void test_parse_null() {
    json_value value;
    json_init(&value);
    json_set_boolean(&value, 0);
    EXPECT_EQ_INT(JSON_PARSE_OK, json_parse(&value, "null"));
    EXPECT_EQ_INT(JSON_NULL, json_get_type(&value));
    json_free(&value);
}

static void test_parse_false() {
    json_value value;
    json_init(&value);
    json_set_boolean(&value, 0);
    EXPECT_EQ_INT(JSON_PARSE_OK, json_parse(&value, "false"));
    EXPECT_EQ_INT(JSON_FALSE, json_get_type(&value));
    json_free(&value);
}

static void test_parse_true() {
    json_value value;
    json_init(&value);
    json_set_boolean(&value, 1);
    EXPECT_EQ_INT(JSON_PARSE_OK, json_parse(&value, "true"));
    EXPECT_EQ_INT(JSON_TRUE, json_get_type(&value));
    json_free(&value);
}

#define TEST_NUMBER(expect, json)\
    do {\
        json_value value;\
        json_init(&value);\
        EXPECT_EQ_INT(JSON_PARSE_OK, json_parse(&value, json));\
        EXPECT_EQ_INT(JSON_NUMBER, json_get_type(&value));\
        EXPECT_EQ_DOUBLE(expect, json_get_number(&value));\
        json_free(&value);\
    } while (0)

static void test_parse_number() {
    TEST_NUMBER(0.0, "0");
    TEST_NUMBER(0.0, "-0");
    TEST_NUMBER(0.0, "-0.0");
    TEST_NUMBER(1.0, "1");
    TEST_NUMBER(-1.0, "-1");
    TEST_NUMBER(1.5, "1.5");
    TEST_NUMBER(-1.5, "-1.5");
    TEST_NUMBER(3.1416, "3.1416");
    TEST_NUMBER(1E10, "1E10");
    TEST_NUMBER(1e10, "1e10");
    TEST_NUMBER(1E+10, "1E+10");
    TEST_NUMBER(1E-10, "1E-10");
    TEST_NUMBER(-1E10, "-1E10");
    TEST_NUMBER(-1e10, "-1e10");
    TEST_NUMBER(-1E+10, "-1E+10");
    TEST_NUMBER(-1E-10, "-1E-10");
    TEST_NUMBER(1.234E+10, "1.234E+10");
    TEST_NUMBER(1.234E-10, "1.234E-10");
    TEST_NUMBER(0.0, "1e-10000"); /* must underflow */

    TEST_NUMBER(1.0000000000000002, "1.0000000000000002"); /* the smallest number > 1 */
    TEST_NUMBER( 4.9406564584124654e-324, "4.9406564584124654e-324"); /* minimum denormal */
    TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
    TEST_NUMBER( 2.2250738585072009e-308, "2.2250738585072009e-308");  /* Max subnormal double */
    TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
    TEST_NUMBER( 2.2250738585072014e-308, "2.2250738585072014e-308");  /* Min normal positive double */
    TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
    TEST_NUMBER( 1.7976931348623157e+308, "1.7976931348623157e+308");  /* Max double */
    TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
}

#define TEST_STRING(expect, json)\
    do {\
        json_value value;\
        json_init(&value);\
        EXPECT_EQ_INT(JSON_PARSE_OK, json_parse(&value, json));\
        EXPECT_EQ_INT(JSON_STRING, json_get_type(&value));\
        EXPECT_EQ_STRING(expect, json_get_string(&value), json_get_string_length(&value));\
        json_free(&value);\
    } while(0)

static void test_parse_string() {
    TEST_STRING("", "\"\"");
    TEST_STRING("Hello", "\"Hello\"");
    TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
    TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
    TEST_STRING("Hello\0World", "\"Hello\\u0000World\"");
    TEST_STRING("\x24", "\"\\u0024\"");         /* Dollar sign U+0024 */
    TEST_STRING("\xC2\xA2", "\"\\u00A2\"");     /* Cents sign U+00A2 */
    TEST_STRING("\xE2\x82\xAC", "\"\\u20AC\""); /* Euro sign U+20AC */
    TEST_STRING("\xF0\x9D\x84\x9E", "\"\\uD834\\uDD1E\"");  /* G clef sign U+1D11E */
    TEST_STRING("\xF0\x9D\x84\x9E", "\"\\ud834\\udd1e\"");  /* G clef sign U+1D11E */
}

#if defined(_MSC_VER)
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect) == (actual), (size_t)expect, (size_t)actual, "%Iu")
#else
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect) == (actual), (size_t)expect, (size_t)actual, "%zu")
#endif

static void test_parse_array() {
    json_value value;

    json_init(&value);
    EXPECT_EQ_INT(JSON_PARSE_OK, json_parse(&value,"[ ]"));
    EXPECT_EQ_INT(JSON_ARRAY, json_get_type(&value));
    EXPECT_EQ_SIZE_T(0, json_get_array_size(&value));
    json_free(&value);

    json_init(&value);
    EXPECT_EQ_INT(JSON_PARSE_OK, json_parse(&value, "[ null, false, true, 123, \"abc\" ]"));
    EXPECT_EQ_INT(JSON_ARRAY, json_get_type(&value));
    EXPECT_EQ_SIZE_T(5, json_get_array_size(&value));
    EXPECT_EQ_INT(JSON_NULL,   json_get_type(json_get_array_element(&value, 0)));
    EXPECT_EQ_INT(JSON_FALSE,  json_get_type(json_get_array_element(&value, 1)));
    EXPECT_EQ_INT(JSON_TRUE,   json_get_type(json_get_array_element(&value, 2)));
    EXPECT_EQ_INT(JSON_NUMBER, json_get_type(json_get_array_element(&value, 3)));
    EXPECT_EQ_INT(JSON_STRING, json_get_type(json_get_array_element(&value, 4)));
    EXPECT_EQ_DOUBLE(123.0, json_get_number(json_get_array_element(&value, 3)));
    EXPECT_EQ_STRING("abc", json_get_string(json_get_array_element(&value, 4)), json_get_string_length(json_get_array_element(&value, 4)));
    json_free(&value);
    
    json_init(&value);
    EXPECT_EQ_INT(JSON_PARSE_OK, json_parse(&value, "[ [ ] , [ 0 ] , [ 0 , 1 ] , [ 0 , 1 , 2 ] ]"));
    EXPECT_EQ_INT(JSON_ARRAY, json_get_type(&value));
    EXPECT_EQ_SIZE_T(4, json_get_array_size(&value));
    for (int i = 0; i < 4; i++) {
        json_value* array = json_get_array_element(&value, i);
        EXPECT_EQ_INT(JSON_ARRAY, json_get_type(array));
        EXPECT_EQ_SIZE_T(i, json_get_array_size(array));
        for (int j = 0; j < i; j++) {
            json_value* ele = json_get_array_element(array, j);
            EXPECT_EQ_INT(JSON_NUMBER, json_get_type(ele));
            EXPECT_EQ_DOUBLE((double)j, json_get_number(ele));
        }
    }
    json_free(&value);
}

static void test_parse_object() {
    json_value value;

    json_init(&value);
    EXPECT_EQ_INT(JSON_PARSE_OK, json_parse(&value, " { } "));
    EXPECT_EQ_INT(JSON_OBJECT, json_get_type(&value));
    EXPECT_EQ_SIZE_T(0, json_get_object_size(&value));
    json_free(&value);

    json_init(&value);
    EXPECT_EQ_INT(JSON_PARSE_OK, json_parse(&value,
        " { "
        "\"n\" : null , "
        "\"f\" : false , "
        "\"t\" : true , "
        "\"i\" : 123 , "
        "\"s\" : \"abc\", "
        "\"a\" : [ 1, 2, 3 ],"
        "\"o\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }"
        " } "
    ));
    EXPECT_EQ_INT(JSON_OBJECT, json_get_type(&value));
    EXPECT_EQ_SIZE_T(7, json_get_object_size(&value));
    EXPECT_EQ_STRING("n", json_get_object_key(&value, 0), json_get_object_key_length(&value, 0));
    EXPECT_EQ_INT(JSON_NULL,   json_get_type(json_get_object_value(&value, 0)));
    EXPECT_EQ_STRING("f", json_get_object_key(&value, 1), json_get_object_key_length(&value, 1));
    EXPECT_EQ_INT(JSON_FALSE,  json_get_type(json_get_object_value(&value, 1)));
    EXPECT_EQ_STRING("t", json_get_object_key(&value, 2), json_get_object_key_length(&value, 2));
    EXPECT_EQ_INT(JSON_TRUE,   json_get_type(json_get_object_value(&value, 2)));
    EXPECT_EQ_STRING("i", json_get_object_key(&value, 3), json_get_object_key_length(&value, 3));
    EXPECT_EQ_INT(JSON_NUMBER, json_get_type(json_get_object_value(&value, 3)));
    EXPECT_EQ_DOUBLE(123.0, json_get_number(json_get_object_value(&value, 3)));
    EXPECT_EQ_STRING("s", json_get_object_key(&value, 4), json_get_object_key_length(&value, 4));
    EXPECT_EQ_INT(JSON_STRING, json_get_type(json_get_object_value(&value, 4)));
    EXPECT_EQ_STRING("abc", json_get_string(json_get_object_value(&value, 4)), json_get_string_length(json_get_object_value(&value, 4)));
    EXPECT_EQ_STRING("a", json_get_object_key(&value, 5), json_get_object_key_length(&value, 5));
    EXPECT_EQ_INT(JSON_ARRAY, json_get_type(json_get_object_value(&value, 5)));
    EXPECT_EQ_SIZE_T(3, json_get_array_size(json_get_object_value(&value, 5)));
    for (size_t i = 0; i < 3; i++) {
        json_value* e = json_get_array_element(json_get_object_value(&value, 5), i);
        EXPECT_EQ_INT(JSON_NUMBER, json_get_type(e));
        EXPECT_EQ_DOUBLE(i + 1.0, json_get_number(e));
    }
    EXPECT_EQ_STRING("o", json_get_object_key(&value, 6), json_get_object_key_length(&value, 6));
    {
        json_value* o = json_get_object_value(&value, 6);
        EXPECT_EQ_INT(JSON_OBJECT, json_get_type(o));
        for (size_t i = 0; i < 3; i++) {
            json_value* ov = json_get_object_value(o, i);
            EXPECT_TRUE('1' + i == json_get_object_key(o, i)[0]);
            EXPECT_EQ_SIZE_T(1, json_get_object_key_length(o, i));
            EXPECT_EQ_INT(JSON_NUMBER, json_get_type(ov));
            EXPECT_EQ_DOUBLE(i + 1.0, json_get_number(ov));
        }
    }
    json_free(&value);
}

#define TEST_ERROR(error, json)\
    do {\
        json_value value;\
        json_init(&value);\
        value.type = JSON_FALSE;\
        EXPECT_EQ_INT(error, json_parse(&value, json));\
        EXPECT_EQ_INT(JSON_NULL, json_get_type(&value));\
        json_free(&value);\
    } while(0)

static void test_parse_expect_value() {
    TEST_ERROR(JSON_PARSE_EXPECT_VALUE, "");
    TEST_ERROR(JSON_PARSE_EXPECT_VALUE, " ");
}

static void test_parse_invalid_value() {
    TEST_ERROR(JSON_PARSE_INVALID_VALUE, "nul");
    TEST_ERROR(JSON_PARSE_INVALID_VALUE, "?");
    /* invalid number */
    TEST_ERROR(JSON_PARSE_INVALID_VALUE, "+0");
    TEST_ERROR(JSON_PARSE_INVALID_VALUE, "+1");
    TEST_ERROR(JSON_PARSE_INVALID_VALUE, ".123"); /* at least one digit before '.' */
    TEST_ERROR(JSON_PARSE_INVALID_VALUE, "1.");   /* at least one digit after '.' */
    TEST_ERROR(JSON_PARSE_INVALID_VALUE, "INF");
    TEST_ERROR(JSON_PARSE_INVALID_VALUE, "inf");
    TEST_ERROR(JSON_PARSE_INVALID_VALUE, "NAN");
    TEST_ERROR(JSON_PARSE_INVALID_VALUE, "nan");

        /* invalid value in array */
    TEST_ERROR(JSON_PARSE_INVALID_VALUE, "[1,]");
    TEST_ERROR(JSON_PARSE_INVALID_VALUE, "[\"a\", nul]");
}

static void test_parse_root_not_singular() {
    TEST_ERROR(JSON_PARSE_ROOT_NOT_SINGULAR, "null x");

    /* invalid number */
    TEST_ERROR(JSON_PARSE_ROOT_NOT_SINGULAR, "0123"); /* after zero should be '.' or nothing */
    TEST_ERROR(JSON_PARSE_ROOT_NOT_SINGULAR, "0x0");
    TEST_ERROR(JSON_PARSE_ROOT_NOT_SINGULAR, "0x123");
}

static void test_parse_number_too_big() {
    TEST_ERROR(JSON_PARSE_NUMBER_TOO_BIG, "1e309");
    TEST_ERROR(JSON_PARSE_NUMBER_TOO_BIG, "-1e309");
}

static void test_parse_missing_quotation_mark() {
    TEST_ERROR(JSON_PARSE_MISS_QUOTATION_MARK, "\"");
    TEST_ERROR(JSON_PARSE_MISS_QUOTATION_MARK, "\"abc");
}

static void test_parse_invalid_string_escape() {
    TEST_ERROR(JSON_PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
    TEST_ERROR(JSON_PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
    TEST_ERROR(JSON_PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
    TEST_ERROR(JSON_PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
}

static void test_parse_invalid_string_char() {
    TEST_ERROR(JSON_PARSE_INVALID_STRING_CHAR, "\"\x01\"");
    TEST_ERROR(JSON_PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
}

static void test_parse_invalid_unicode_hex() {
    TEST_ERROR(JSON_PARSE_INVALID_UNICODE_HEX, "\"\\u\"");
    TEST_ERROR(JSON_PARSE_INVALID_UNICODE_HEX, "\"\\u0\"");
    TEST_ERROR(JSON_PARSE_INVALID_UNICODE_HEX, "\"\\u01\"");
    TEST_ERROR(JSON_PARSE_INVALID_UNICODE_HEX, "\"\\u012\"");
    TEST_ERROR(JSON_PARSE_INVALID_UNICODE_HEX, "\"\\u/000\"");
    TEST_ERROR(JSON_PARSE_INVALID_UNICODE_HEX, "\"\\uG000\"");
    TEST_ERROR(JSON_PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
    TEST_ERROR(JSON_PARSE_INVALID_UNICODE_HEX, "\"\\u0G00\"");
    TEST_ERROR(JSON_PARSE_INVALID_UNICODE_HEX, "\"\\u00/0\"");
    TEST_ERROR(JSON_PARSE_INVALID_UNICODE_HEX, "\"\\u00G0\"");
    TEST_ERROR(JSON_PARSE_INVALID_UNICODE_HEX, "\"\\u000/\"");
    TEST_ERROR(JSON_PARSE_INVALID_UNICODE_HEX, "\"\\u000G\"");
    TEST_ERROR(JSON_PARSE_INVALID_UNICODE_HEX, "\"\\u 123\"");

}

static void test_parse_invalid_unicode_surrogate() {
    TEST_ERROR(JSON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\"");
    TEST_ERROR(JSON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uDBFF\"");
    TEST_ERROR(JSON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\\\\"");
    TEST_ERROR(JSON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uDBFF\"");
    TEST_ERROR(JSON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uE000\"");
}

static void test_parse_miss_comma_or_square_bracket() {
    TEST_ERROR(JSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1");
    TEST_ERROR(JSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1}");
    TEST_ERROR(JSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1 2");
    TEST_ERROR(JSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[[]");
}

static void test_parse_miss_key() {
    TEST_ERROR(JSON_PARSE_MISS_KEY, "{:1,");
    TEST_ERROR(JSON_PARSE_MISS_KEY, "{1:1,");
    TEST_ERROR(JSON_PARSE_MISS_KEY, "{true:1,");
    TEST_ERROR(JSON_PARSE_MISS_KEY, "{false:1,");
    TEST_ERROR(JSON_PARSE_MISS_KEY, "{null:1,");
    TEST_ERROR(JSON_PARSE_MISS_KEY, "{[]:1,");
    TEST_ERROR(JSON_PARSE_MISS_KEY, "{{}:1,");
    TEST_ERROR(JSON_PARSE_MISS_KEY, "{\"a\":1,");
}

static void test_parse_miss_colon() {
    TEST_ERROR(JSON_PARSE_MISS_COLON, "{\"a\"}");
    TEST_ERROR(JSON_PARSE_MISS_COLON, "{\"a\",\"b\"}");
}

static void test_parse_miss_comma_or_curly_bracket() {
    TEST_ERROR(JSON_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1");
    TEST_ERROR(JSON_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1]");
    TEST_ERROR(JSON_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1 \"b\"");
    TEST_ERROR(JSON_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":{}");
}

/* 可称为往返（roundtrip）测试 */
#define TEST_ROUNDTRIP(json)\
    do {\
        json_value value;\
        char* json2;\
        size_t length;\
        json_init(&value);\
        EXPECT_EQ_INT(JSON_PARSE_OK, json_parse(&value, json));\
        EXPECT_EQ_INT(JSON_STRINGIFY_OK, json_stringify(&value, &json2, &length));\
        EXPECT_EQ_STRING(json, json2, length);\
        json_free(&value);\
        free(json2);\
    } while(0)

static void test_stringify_number() {
    TEST_ROUNDTRIP("0");
    TEST_ROUNDTRIP("-0");
    TEST_ROUNDTRIP("1");
    TEST_ROUNDTRIP("-1");
    TEST_ROUNDTRIP("1.5");
    TEST_ROUNDTRIP("-1.5");
    TEST_ROUNDTRIP("3.25");
    // TEST_ROUNDTRIP("1e+20");
    // TEST_ROUNDTRIP("1.234e+20");
    // TEST_ROUNDTRIP("1.234e-20");

    TEST_ROUNDTRIP("1.0000000000000002"); /* the smallest number > 1 */
    TEST_ROUNDTRIP("4.9406564584124654e-324"); /* minimum denormal */
    TEST_ROUNDTRIP("-4.9406564584124654e-324");
    TEST_ROUNDTRIP("2.2250738585072009e-308");  /* Max subnormal double */
    TEST_ROUNDTRIP("-2.2250738585072009e-308");
    TEST_ROUNDTRIP("2.2250738585072014e-308");  /* Min normal positive double */
    TEST_ROUNDTRIP("-2.2250738585072014e-308");
    TEST_ROUNDTRIP("1.7976931348623157e+308");  /* Max double */
    TEST_ROUNDTRIP("-1.7976931348623157e+308");
}

static void test_stringify_string() {
    TEST_ROUNDTRIP("\"\"");
    TEST_ROUNDTRIP("\"Hello\"");
    TEST_ROUNDTRIP("\"Hello\\nWorld\"");
    TEST_ROUNDTRIP("\"\\\" \\\\ / \\b \\f \\n \\r \\t\"");
    TEST_ROUNDTRIP("\"Hello\\u0000World\"");
}

static void test_stringify_array() {
    TEST_ROUNDTRIP("[]");
    TEST_ROUNDTRIP("[null,false,true,123,\"abc\",[1,2,3]]");
}

static void test_stringify_object() {
    TEST_ROUNDTRIP("{}");
    TEST_ROUNDTRIP("{\"n\":null,\"f\":false,\"t\":true,\"i\":123,\"s\":\"abc\",\"a\":[1,2,3],\"o\":{\"1\":1,\"2\":2,\"3\":3}}");
}

static void test_stringify() {
    TEST_ROUNDTRIP("null");
    TEST_ROUNDTRIP("false");
    TEST_ROUNDTRIP("true");
    test_stringify_number();
    test_stringify_string();
    test_stringify_array();
    test_stringify_object();
}

static void test_parse() {
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_number();
    test_parse_string();
    test_parse_array();
    test_parse_object();
    test_stringify();

    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
    test_parse_number_too_big();
    test_parse_missing_quotation_mark();
    test_parse_invalid_string_escape();
    test_parse_invalid_string_char();
    test_parse_invalid_string_char();
    test_parse_invalid_unicode_hex();
    test_parse_invalid_unicode_surrogate();
    test_parse_miss_comma_or_square_bracket();
    test_parse_miss_key();
    test_parse_miss_colon();
    test_parse_miss_comma_or_curly_bracket();
}

static void test_access_null() {
    json_value value;
    json_init(&value);
    json_set_null(&value);
    EXPECT_EQ_INT(JSON_NULL, json_get_type(&value));
    json_free(&value);
}  

static void test_access_boolean() {
    json_value value;
    json_init(&value);
    json_set_boolean(&value, 1);
    EXPECT_TRUE(json_get_boolean(&value));
    json_set_boolean(&value, 0);
    EXPECT_FALSE(json_get_boolean(&value));
    json_free(&value);
}

static void test_access_number() {
    json_value value;
    json_init(&value);
    json_set_number(&value, 1.1);
    EXPECT_EQ_DOUBLE(1.1, json_get_number(&value));
}

static void test_access_string() {
    json_value value;
    json_init(&value);
    json_set_string(&value, "", 0);
    EXPECT_EQ_STRING("", json_get_string(&value), json_get_string_length(&value));
    json_set_string(&value, "Hello", 5);
    EXPECT_EQ_STRING("Hello", json_get_string(&value), json_get_string_length(&value));
    json_free(&value);
}

static void test_access() {
    test_access_null();
    test_access_boolean();
    test_access_number();
    test_access_string();
}

int main() {
#ifdef _WINDOWS
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    
    test_parse();
    test_access();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}