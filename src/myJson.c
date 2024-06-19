/******************************
*  Github:yichenyiyan  QAQ    *
*  	version 1				  *
*  							  *
*  							  *
*******************************/

#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include <stdio.h>
#include "../include/myJson.h"
#include <assert.h> 
#include <stdlib.h>  /* strtod */
#include <string.h>
#include <errno.h> /* errno, ERANGE */
#include <math.h>  /* HUGE_VAL */


#ifndef JSON_PARSE_STACK_INIT_SIZE
#define JSON_PARSE_STACK_INIT_SIZE 256
#endif

#ifndef JSON_PARSE_STRINGIFY_INIT_SIZE
#define JSON_PARSE_STRINGIFY_INIT_SIZE 256
#endif

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); (c)->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')
#define PUTC(c, ch)         do { *(char*)myjson_context_push(c, sizeof(char)) = (ch); } while(0)
#define STRING_ERROR(ret)   do { c->top = head; return ret; } while(0)
#define PUTS(c, s, len)     memcpy(myjson_context_push(c, len), s, len)


struct _json_context {
    const char* json;
    char* stack;
    size_t size, top; //栈的容量、栈顶地址索引
};

typedef struct _json_context json_context;

//为栈重新分配内存在原有的大小上多开辟size字节的大小，并将栈顶地址前移
static void*
myjson_context_push(json_context* c, size_t size){
    void* ret;
    assert(size > 0);
    if(c->top + size >= c->size){
        if(c->size == 0)
            c->size = JSON_PARSE_STACK_INIT_SIZE;
        while(c->top + size >= c->size)
            c->size += c->size >> 1; // 当前空间大小增加当前空间大小的一半
        c->stack = (char*)realloc(c->stack, c->size);
    }
    ret = c->stack + c->top;
    c->top += size;

    return ret;
}


//弹出栈对应大小的内存并将指针后移
static void*
myjson_context_pop(json_context* c, size_t size){
    assert(c->top >= size);

    return c->stack + (c->top -= size);
}

//解析空格符
static void
myjson_parse_whitespace(json_context* c){
    const char* p = c->json;
    //所谓空白，是由零或多个空格符（space U+0020）、制表符（tab U+0009）、换行符（LF U+000A）、回车符（CR U+000D）所组成
    while(*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

//解析字面量
static myjson_err
myjson_parse_literal(json_context* c, myjson_val* v, const char* literal, myjson_type type){
    size_t i;
    EXPECT(c, literal[0]);
    for(i = 0; literal[i + 1]; ++i)
        if(c->json[i] != literal[i+1])
            return JSON_PARSE_INVALID_VALUE;
        
    c->json += i;
    v->type = type;
    return JSON_PARSE_OK;
}

//解析json数据
static myjson_err
myjson_parse_number(json_context* c, myjson_val* v){
    const char* p = c->json;
    if(*p == '-') p++;
    if(*p == '0') p++;
    else {
        if(!ISDIGIT1TO9(*p)) return JSON_PARSE_INVALID_VALUE;
        for(p++; ISDIGIT(*p); p++);
    }

    if(*p == '.') {
        p++;
        if(!ISDIGIT(*p)) return JSON_PARSE_INVALID_VALUE;
        for(p++; ISDIGIT(*p); p++);
    }

    if(*p == 'e' || *p == 'E') {
        p++;
        if(*p == '+' || *p == '-') p++;
        if(!ISDIGIT(*p)) return JSON_PARSE_INVALID_VALUE;
        for(p++; ISDIGIT(*p); p++);
    }

    errno = 0;
    v->u.n = strtod(c->json, NULL);
    if(errno == ERANGE && (v->u.n == HUGE_VAL || v->u.n == -HUGE_VAL))
        return JSON_PARSE_NUMBER_TOO_BIG;
    
    v->type = JSON_NUMBER;
    c->json = p;

    return JSON_PARSE_OK;
}

// 解析字符十六进制数字
static const char* 
myjson_parse_hex4(const char* p, unsigned* u) {
    *u = 0;
    for(int i = 0; i < 4; ++i){
        char ch = *p++;
        *u <<= 4;
        if(ch >= '0' && ch <= '9') *u |= ch - '0';
        else if(ch >= 'A' && ch <= 'F') *u |= ch - ('A' - 10);
        else if(ch >= 'a' && ch <= 'f') *u |= ch - ('a' - 10);
        else return NULL;
    }

    return p;
}


/*根据以下 UTF-8 编码表实现

UTF-8 的编码单元为 8 位（1 字节），每个码点编码成 1 至 4 个字节。
它的编码方式很简单，按照码点的范围，把码点的二进位分拆成 1 至最多 4 个字节：

| 码点范围            | 码点位数  | 字节1     | 字节2    | 字节3    | 字节4     |
|:------------------:|:--------:|:--------:|:--------:|:--------:|:--------:|
| U+0000 ~ U+007F    | 7        | 0xxxxxxx |
| U+0080 ~ U+07FF    | 11       | 110xxxxx | 10xxxxxx |
| U+0800 ~ U+FFFF    | 16       | 1110xxxx | 10xxxxxx | 10xxxxxx |
| U+10000 ~ U+10FFFF | 21       | 11110xxx | 10xxxxxx | 10xxxxxx | 10xxxxxx |
*/
//转化成utf8编码格式
static void 
myjson_encode_utf8(json_context* c, unsigned u) {
    if (u <= 0x7F) 
        PUTC(c, u & 0xFF);
    else if (u <= 0x7FF) {
        PUTC(c, 0xC0 | ((u >> 6) & 0xFF));
        PUTC(c, 0x80 | ( u       & 0x3F));
    }
    else if (u <= 0xFFFF) {
        PUTC(c, 0xE0 | ((u >> 12) & 0xFF));
        PUTC(c, 0x80 | ((u >>  6) & 0x3F));
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



static myjson_err 
myjson_parse_string_raw(json_context* c, char** str, size_t* len) {
    size_t head = c->top;
    unsigned u, u2;
    const char* p;
    EXPECT(c, '\"');
    p = c->json;
    for (;;) {
        char ch = *p++;
        switch (ch) {
            case '\"':
                *len = c->top - head;
                *str = myjson_context_pop(c, *len);
                c->json = p;
                return JSON_PARSE_OK;
            case '\\':
                switch (*p++) {
                    case '\"': PUTC(c, '\"'); break;
                    case '\\': PUTC(c, '\\'); break;
                    case '/':  PUTC(c, '/' ); break;
                    case 'b':  PUTC(c, '\b'); break;
                    case 'f':  PUTC(c, '\f'); break;
                    case 'n':  PUTC(c, '\n'); break;
                    case 'r':  PUTC(c, '\r'); break;
                    case 't':  PUTC(c, '\t'); break;
                    case 'u':
                        if (!(p = myjson_parse_hex4(p, &u)))
                            STRING_ERROR(JSON_PARSE_INVALID_UNICODE_HEX);
                        // "\u(0xD800~0xDBFF)\u(hex16_digit)"   format: "\uH\uL"
                        // codepoint = 0x10000 + (H − 0xD800) × 0x400 + (L − 0xDC00)
                        if (u >= 0xD800 && u <= 0xDBFF) { /* surrogate pair */
                            if (*p++ != '\\')
                                STRING_ERROR(JSON_PARSE_INVALID_UNICODE_SURROGATE);
                            if (*p++ != 'u')
                                STRING_ERROR(JSON_PARSE_INVALID_UNICODE_SURROGATE);
                            if (!(p = myjson_parse_hex4(p, &u2)))
                                STRING_ERROR(JSON_PARSE_INVALID_UNICODE_HEX);
                            if (u2 < 0xDC00 || u2 > 0xDFFF)
                                STRING_ERROR(JSON_PARSE_INVALID_UNICODE_SURROGATE);
                            u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
                        }
                        //此解析器只支持UTF-8
                        myjson_encode_utf8(c, u);
                        break;
                    default:
                        STRING_ERROR(JSON_PARSE_INVALID_STRING_ESCAPE);
                }
                break;
            case '\0':
                STRING_ERROR(JSON_PARSE_MISS_QUOTATION_MARK);
            default:
                if ((unsigned char)ch < 0x20)
                    STRING_ERROR(JSON_PARSE_INVALID_STRING_CHAR);
                PUTC(c, ch);
        }
    }
}

//解析json字符串
static myjson_err
myjson_parse_string(json_context* c, myjson_val* v){
    myjson_err ret;
    char* s;
    size_t len;
    if((ret = myjson_parse_string_raw(c, &s, &len)) == JSON_PARSE_OK)
        set_myjson_string(v, s, len);
    return ret;
}

static myjson_err
myjson_parse_val(json_context*, myjson_val*);


//解析json数组
static myjson_err
myjson_parse_array(json_context* c, myjson_val* v){
    size_t size = 0;
    size_t i; 
    myjson_err ret;
    EXPECT(c, '[');
    myjson_parse_whitespace(c);
    if (*c->json == ']') {
        c->json++;
        v->type = JSON_ARRAY;
        v->u.a.size = 0;
        v->u.a.e = NULL;
        return JSON_PARSE_OK;
    }

    for (;;) {
        myjson_val e;
        MYJSON_INIT(&e);
        if ((ret = myjson_parse_val(c, &e)) != JSON_PARSE_OK)
            break;
        memcpy(myjson_context_push(c, sizeof(myjson_val)), &e, sizeof(myjson_val));
        size++;//mainly function: to record push number
        myjson_parse_whitespace(c);
        if (*c->json == ','){
            *c->json++;
            myjson_parse_whitespace(c);
        }
            
        else if (*c->json == ']'){
            c->json++;
            v->type = JSON_ARRAY;
            v->u.a.size = size;
            size *= sizeof(myjson_val);
            memcpy(v->u.a.e = (myjson_val*)malloc(size), myjson_context_pop(c, size), size);
            return JSON_PARSE_OK;
        }
        else{
            ret = JSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
            break;
        }
    }

    /* Pop and free values on the stack */
    for (i = 0; i < size; i++)
        myjson_free((myjson_val*)myjson_context_pop(c, sizeof(myjson_val)));
    return ret;
}


/*
member = string ws %x3A ws value
object = %x7B ws [ member *( ws %x2C ws member ) ] ws %x7D
*/
static myjson_err
myjson_parse_object(json_context* c, myjson_val* v){
    size_t i, size;
    myjson_member m;
    myjson_err ret;
    EXPECT(c, '{');
    myjson_parse_whitespace(c);
    if (*c->json == '}'){
        c->json++;
        v->type = JSON_OBJECT;
        v->u.o.m = 0;
        v->u.o.size = 0;
        return JSON_PARSE_OK;
    }
    m.k = NULL;
    size = 0;
    for (;;) {
        char* str;
        MYJSON_INIT(&m.v);
        //parse key
        if (*c->json != '"'){
            ret = JSON_PARSE_MISS_KEY;
            break;
        }
        if ((ret = myjson_parse_string_raw(c, &str, &m.klen)) != JSON_PARSE_OK)
            break;

        memcpy(m.k = (char*)malloc(m.klen + 1), str, m.klen);
        m.k[m.klen] = '\0';
        //parse ws colon ws
        myjson_parse_whitespace(c);
        if (*c->json != ':'){
            ret = JSON_PARSE_MISS_COLON;
            break;
        }
        c->json++;
        myjson_parse_whitespace(c);
        //parse value
        if((ret = myjson_parse_val(c, &m.v)) != JSON_PARSE_OK)
            break;
        memcpy((myjson_context_push(c, sizeof(myjson_member))), &m, sizeof(myjson_member));
        size++;
        m.k = NULL;
        
        // parse ws [comma | right-curly-brace] ws 
        myjson_parse_whitespace(c);
        if(*c->json == ','){
            c->json++;
            myjson_parse_whitespace(c);
        }
        else if (*c->json == '}'){
            size_t s = sizeof(myjson_member) * size;
            c->json++;
            v->type = JSON_OBJECT;
            v->u.o.size = size;
            memcpy((v->u.o.m = (myjson_member*)malloc(s)), myjson_context_pop(c, s), s);

            return JSON_PARSE_OK;
        }
        else{
            ret = JSON_PARSE_MISS_COMMA_OR_CURLY_BRACKET;
            break;
        }
    }

    //pop and free members on the stack
    free(m.k);
    for (i = 0; i < size; ++i){
        myjson_member* m = (myjson_member*)myjson_context_pop(c, sizeof(myjson_member));
        free(m->k);
        myjson_free(&m->v);
    }
    
    v->type = JSON_NULL;

    return ret;
}


static myjson_err
myjson_parse_val(json_context* c, myjson_val* v){
    switch (*c->json){
    case 't' : return myjson_parse_literal(c, v, "true", JSON_TRUE);
    case 'f' : return myjson_parse_literal(c, v, "false", JSON_FALSE);
    case 'n' : return myjson_parse_literal(c, v, "null", JSON_NULL);
    case '\0' : return JSON_PARSE_EXPECT_VALUE;
    case '"' : return myjson_parse_string(c, v);
    case '[':  return myjson_parse_array(c, v);
    case '{':  return myjson_parse_object(c, v);
    default : return myjson_parse_number(c, v);;
    }
}





/*
rfc7159 JSON语法：

JSON-text = ws value ws
ws = *(%x20 / %x09 / %x0A / %x0D)
*/


myjson_err
myjson_parse(myjson_val* v, const char* json){
    json_context c;
    myjson_err ret;
    assert(v != NULL);
    c.json = json;
    c.stack = NULL;
    c.size = 0;
    c.top = 0;
    MYJSON_INIT(v);
    myjson_parse_whitespace(&c);
    if((ret = myjson_parse_val(&c, v)) == JSON_PARSE_OK){
        myjson_parse_whitespace(&c);
        if(*c.json != '\0'){
            v->type = JSON_NULL;
            ret = JSON_PARSE_ROOT_NOT_SINGULAR;
        }
    }

    assert(c.top == 0);
    free(c.stack);

    return ret;
}

static void
myjson_stringify_string(json_context* c, const char* s, size_t len){
    static const char hex_digits[] = 
        { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    size_t i, size;
    char* head, *p;
    assert(s != NULL);
    p = head = myjson_context_push(c, size = len * 6 + 2); /* "\u00xx..." */
    *p++ = '"';
    for (i = 0; i < len; i++) {
        unsigned char ch = (unsigned char)s[i];
        switch (ch) {
            case '\"': *p++ = '\\'; *p++ = '\"'; break;
            case '\\': *p++ = '\\'; *p++ = '\\'; break;
            case '\b': *p++ = '\\'; *p++ = 'b';  break;
            case '\f': *p++ = '\\'; *p++ = 'f';  break;
            case '\n': *p++ = '\\'; *p++ = 'n';  break;
            case '\r': *p++ = '\\'; *p++ = 'r';  break;
            case '\t': *p++ = '\\'; *p++ = 't';  break;
            default:
                if (ch < 0x20) {
                    *p++ = '\\'; *p++ = 'u'; *p++ = '0'; *p++ = '0';
                    *p++ = hex_digits[ch >> 4];
                    *p++ = hex_digits[ch & 15];
                }
                else
                    *p++ = s[i];
        }
    }
    *p++ = '"';
    c->top -= size - (p - head);
}


static void
myjson_stringify_val(json_context* c, const myjson_val* v){
    size_t i;
    switch (v->type){
        case JSON_NULL:   PUTS(c, "null", 4); break;
        case JSON_FALSE:  PUTS(c, "false", 5); break;
        case JSON_TRUE:   PUTS(c, "true", 4); break;
        case JSON_NUMBER: c->top -= 32 - sprintf(myjson_context_push(c, 32), "%.17g", v->u.n); break;
        case JSON_STRING: myjson_stringify_string(c, v->u.s.s, v->u.s.len); break;
        case JSON_ARRAY: 
        {
            PUTC(c, '[');
            for (i = 0; i < v->u.a.size; i++) {
                if (i > 0)
                    PUTC(c, ','); //细节','
                myjson_stringify_val(c, &v->u.a.e[i]);
            }
            PUTC(c, ']');
            break;
        }
            break;
        case JSON_OBJECT: 
        {
            PUTC(c, '{');
            for (i = 0; i < v->u.o.size; i++) {
                if (i > 0)
                    PUTC(c, ','); 
                myjson_stringify_string(c, v->u.o.m[i].k, v->u.o.m[i].klen);
                PUTC(c, ':');
                myjson_stringify_val(c, &v->u.o.m[i].v);
            }
            PUTC(c, '}');
        }
            break;
        default: 
            assert(0 && "invalid type"); 
            break;
    }
}




char* 
myjson_stringify(const myjson_val* v, size_t* len){
    json_context c;
    assert(v != NULL);
    c.stack = (char*)malloc(c.size = JSON_PARSE_STRINGIFY_INIT_SIZE);
    c.top = 0;
    myjson_stringify_val(&c, v);
    if(len)
        *len = c.top;
    PUTC(&c, '\0');

    return c.stack;
}


void 
myjson_copy(myjson_val* dst, const myjson_val* src){
    assert(src != NULL && dst != NULL && src != dst);
    switch (src->type) {
        case JSON_STRING:
            set_myjson_string(dst, src->u.s.s, src->u.s.len);
            break;
        case JSON_ARRAY:
            //..
            break;
        case JSON_OBJECT:
            //..
            break;
        default:
            myjson_free(dst);
            memcpy(dst, src, sizeof(myjson_val));
            break;
    }
}


void 
myjson_move(myjson_val* dst, myjson_val* src){
    assert(dst != NULL && src != NULL && src != dst);
    myjson_free(dst);
    memcpy(dst, src, sizeof(myjson_val));
    MYJSON_INIT(src);
}


void 
myjson_swap(myjson_val* lhs, myjson_val* rhs){
    assert(lhs != NULL && rhs != NULL);
    if (lhs != rhs) {
        myjson_val temp;
        memcpy(&temp, lhs, sizeof(myjson_val));
        memcpy(lhs,   rhs, sizeof(myjson_val));
        memcpy(rhs, &temp, sizeof(myjson_val));
    }
}


myjson_compare_err 
myjson_is_equal(const myjson_val* lhs, const myjson_val* rhs) {
    size_t i;
    assert(lhs != NULL && rhs != NULL);
    if (lhs->type != rhs->type)
        return 0;
    switch (lhs->type) {
        case JSON_STRING:
            return lhs->u.s.len == rhs->u.s.len && 
                memcmp(lhs->u.s.s, rhs->u.s.s, lhs->u.s.len) == 0 ? COMPARE_YEAH : COMPARE_NO;
        case JSON_NUMBER:
            return lhs->u.n == rhs->u.n ? COMPARE_YEAH : COMPARE_NO;
        case JSON_ARRAY:
            if (lhs->u.a.size != rhs->u.a.size)
                return COMPARE_NO;
            for (i = 0; i < lhs->u.a.size; i++)
                if (!myjson_is_equal(&lhs->u.a.e[i], &rhs->u.a.e[i]))
                    return COMPARE_NO;
            return 1;
        case JSON_OBJECT:
            //..
            return COMPARE_YEAH;
        default:
            return COMPARE_YEAH;
    }
}


myjson_type 
get_myjson_type(const myjson_val* v){
    assert(v != NULL);

    return v->type;
}


double 
get_myjson_number(const myjson_val* v){
    assert(v != NULL && v->type == JSON_NUMBER);

    return v->u.n;
}


void 
set_myjson_number(myjson_val* v, double d){
    myjson_free(v);
    v->u.n = d;
    v->type = JSON_NUMBER;
}

void 
myjson_free(myjson_val* v){
    size_t i;
    assert(v != NULL);
    switch (v->type){
        case JSON_STRING:
            free(v->u.s.s);
            break;
        case JSON_ARRAY:
        {
            for(i = 0; i < v->u.a.size; ++i)
                myjson_free(&v->u.a.e[i]);
            free(v->u.a.e);
            break;
        }
        case JSON_OBJECT:
        {
            for (int i = 0; i < v->u.o.size; ++i){
                free(v->u.o.m[i].k);
                myjson_free(&v->u.o.m[i].v);
            }
            free(v->u.o.m);
            break;
        }
        default: break;
    }
    v->type = JSON_NULL;
}

myjson_err 
get_myjson_boolean(const myjson_val* v){
    assert(v != NULL && (v->type == JSON_TRUE || v->type == JSON_FALSE));

    return v->type == JSON_TRUE;
}

void 
set_myjson_boolean(myjson_val* v, int b){
    myjson_free(v);
    v->type = (b ? JSON_TRUE : JSON_FALSE);
}

const char* 
get_myjson_string(const myjson_val* v){
    assert(v != NULL && v->type == JSON_STRING);
    return v->u.s.s;
}

size_t 
get_myjson_string_length(const myjson_val* v){
    assert(v != NULL && v->type == JSON_STRING);
    return v->u.s.len;
}

void 
set_myjson_string(myjson_val* v, const char* s , size_t len){
    assert(v != NULL && (s != NULL || len == 0));
    myjson_free(v);
    v->u.s.s = (char*)malloc(len + 1);
    memcpy(v->u.s.s, s, len);
    v->u.s.s[len] = '\0';
    v->u.s.len = len;
    v->type = JSON_STRING;
}


size_t 
myjson_get_array_size(const myjson_val* v) {
    assert(v != NULL && v->type == JSON_ARRAY);
    return v->u.a.size;
}

myjson_val* 
myjson_get_array_element(const myjson_val* v, size_t index) {
    assert(v != NULL && v->type == JSON_ARRAY);
    assert(index < v->u.a.size);
    return &v->u.a.e[index];
}

size_t 
myjson_get_object_size(const myjson_val* v) {
    assert(v != NULL && v->type == JSON_OBJECT);
    return v->u.o.size;
}

const char* 
myjson_get_object_key(const myjson_val* v, size_t index) {
    assert(v != NULL && v->type == JSON_OBJECT);
    assert(index < v->u.o.size);
    return v->u.o.m[index].k;
}

size_t 
myjson_get_object_key_length(const myjson_val* v, size_t index) {
    assert(v != NULL && v->type == JSON_OBJECT);
    assert(index < v->u.o.size);
    return v->u.o.m[index].klen;
}

myjson_val* 
myjson_get_object_value(const myjson_val* v, size_t index) {
    assert(v != NULL && v->type == JSON_OBJECT);
    assert(index < v->u.o.size);
    return &v->u.o.m[index].v;
}

void 
myjson_set_array(myjson_val* v, size_t cap){
    assert(v != NULL);
    myjson_free(v);
    v->type = JSON_ARRAY;
    v->u.a.size = 0;
    v->u.a.capacity = cap;
    v->u.a.e = cap > 0 ? (myjson_val*)malloc(cap * sizeof(myjson_val)) : NULL;
}


size_t 
myjson_get_array_capacity(const myjson_val* v){
    assert(v != NULL && v->type == JSON_ARRAY);
    return v->u.a.capacity;
}


void 
myjson_reserve_array(myjson_val* v, size_t cap){
    assert(v != NULL && v->type == JSON_ARRAY);
    if(v->u.a.capacity < cap){
        v->u.a.capacity = cap;
        v->u.a.e = (myjson_val*)realloc(v->u.a.e, cap * sizeof(myjson_val));
    }
}


void 
myjson_shrink_array(myjson_val* v){
    assert(v != NULL && v->type == JSON_ARRAY);
    if(v->u.a.capacity > v->u.a.size){
        v->u.a.capacity = v->u.a.size;
        v->u.a.e = (myjson_val*)realloc(v->u.a.e, v->u.a.capacity * sizeof(myjson_val));
    }
}

void 
myjson_clear_array(myjson_val* v){
    assert(v != NULL && v->type == JSON_ARRAY);
    //..
}


myjson_val* 
myjson_pushback_array_element(myjson_val* v){
    assert(v != NULL && v->type == JSON_ARRAY);
    if (v->u.a.size == v->u.a.capacity)
        myjson_reserve_array(v, v->u.a.capacity == 0 ? 1 : v->u.a.capacity * 2);
    MYJSON_INIT(&v->u.a.e[v->u.a.size]);
    return &v->u.a.e[v->u.a.size++];
}


void 
myjson_popback_array_element(myjson_val* v){
    assert(v != NULL && v->type == JSON_ARRAY && v->u.a.size > 0);
    myjson_free(&v->u.a.e[--v->u.a.size]);
}


myjson_val* 
myjson_insert_array_element(myjson_val* v, size_t index){
    assert(v != NULL && v->type == JSON_ARRAY && v->u.a.size > 0 && index < v->u.a.capacity);
    //..
    return v->u.a.e;
}

void 
myjson_erase_array_element(myjson_val* v, size_t index, size_t count){
    //..
}

 
void 
myjson_set_object(myjson_val* v, size_t capacity){
    assert(v != NULL);
    myjson_free(v);
    v->type = JSON_OBJECT;
    v->u.o.size = 0;
    v->u.o.capacity = capacity;
    v->u.o.m = capacity > 0 ? (myjson_member*)malloc(capacity * sizeof(myjson_member)) : NULL;
}


size_t 
myjson_get_object_capacity(const myjson_val* v){
    assert(v != NULL && v->type == JSON_OBJECT);
    return v->u.o.capacity;
}


void 
myjson_reserve_object(myjson_val* v, size_t cap){
    //..
}


void 
myjson_shrink_object(myjson_val* v){
    //..
}

void 
myjson_clear_object(myjson_val* v){
    //..
}



size_t 
myjson_find_object_index(const myjson_val* v, const char* key, size_t klen){
    size_t i;
    assert(v != NULL && v->type == JSON_OBJECT && key != NULL);
    for (i = 0; i < v->u.o.size; i++)
        if (v->u.o.m[i].klen == klen && memcmp(v->u.o.m[i].k, key, klen) == 0)
            return i;
    return JSON_KEY_NOT_EXIST;
}


myjson_val* 
myjson_find_object_value(myjson_val* v , const char* key, size_t klen){
    size_t index = myjson_find_object_index(v, key, klen);
    return index != JSON_KEY_NOT_EXIST ? &v->u.o.m[index].v : NULL;
}


myjson_val* 
myjson_set_object_value(myjson_val* v, const char* key, size_t klen){
    assert(v != NULL && v->type == JSON_OBJECT && key != NULL);
    //..
    return &v->u.o.m->v;
}

void 
myjson_remove_object_value(myjson_val* v, size_t index){
    assert(v != NULL && v->type == JSON_OBJECT && index < v->u.o.size);
    //..
}