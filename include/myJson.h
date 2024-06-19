/******************************
*  Github:yichenyiyan  QAQ    *
*  	version 1				  *
*  							  *
*  							  *
*******************************/



#ifndef MY_JSON_H_
#define MY_JSON_H_


#include <stddef.h> /* size_t */

/* json六种数据类型
* null: 表示为 null
* boolean: 表示为 true 或 false
* number: 一般的浮点数表示方式，在下一单元详细说明
* string: 表示为 "..."
* array: 表示为 [ ... ]
* object: 表示为 { ... }
*/

enum _json_types {
    JSON_NULL,
    JSON_FALSE,
    JSON_TRUE,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
};

#define JSON_KEY_NOT_EXIST ((size_t)-1)

typedef enum _json_types myjson_type;

struct _myjson_member;

typedef struct _myjson_member myjson_member;

struct _json_value;

typedef struct _json_value myjson_val;

//JSON值
struct _json_value {
    union {
        struct { myjson_member* m; size_t size, capacity; }o;  //object  members, member count , capacity
        struct { myjson_val* e; size_t size, capacity; }a;     //array  elements, element count, capacity
        struct { char* s; size_t len; }s;            //string  val  length
        double n;                                    //number
    } u;
    
    myjson_type type;
};


struct _myjson_member {
    char* k;        //member key string
    size_t klen;    //key string length 
    myjson_val v;   //member value
};

//错误码
enum _json_err {
    JSON_PARSE_OK = 0,            //OK   
    JSON_PARSE_EXPECT_VALUE,      //JSON 只含有空白
    JSON_PARSE_INVALID_VALUE,     //一个值之后，在空白之后还有其他字符
    JSON_PARSE_ROOT_NOT_SINGULAR,  //值不是那三种字面值
    JSON_PARSE_NUMBER_TOO_BIG,     //数字过大
    JSON_PARSE_MISS_QUOTATION_MARK,  //双引号
    JSON_PARSE_INVALID_STRING_ESCAPE,  //无效(非法)字符串
    JSON_PARSE_INVALID_STRING_CHAR,    //无效的字符串字符
    JSON_PARSE_INVALID_UNICODE_HEX,   //无效的unicode十六进制数字
    JSON_PARSE_INVALID_UNICODE_SURROGATE,  //无效的代理对
    JSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, //缺少逗号或方括号
    JSON_PARSE_MISS_KEY,   //缺少键
    JSON_PARSE_MISS_COLON, //缺少冒号
    JSON_PARSE_MISS_COMMA_OR_CURLY_BRACKET,  //缺少逗号或大括号
};


typedef enum _json_err myjson_err;


typedef enum {
    COMPARE_YEAH = 0,
    COMPARE_NO
}myjson_compare_err;

#define MYJSON_INIT(v)  do { (v)->type = JSON_NULL; } while(0)

//解析JSON
myjson_err myjson_parse(myjson_val*, const char*);

//生成json
char* myjson_stringify(const myjson_val*, size_t*);


void myjson_copy(myjson_val*, const myjson_val*);
void myjson_move(myjson_val*, myjson_val*);
void myjson_swap(myjson_val*, myjson_val*);
myjson_compare_err myjson_is_equal(const myjson_val*, const myjson_val*);

//获取json数据类型
myjson_type get_myjson_type(const myjson_val*);


//获取数字
double get_myjson_number(const myjson_val*);
//设置数字
void set_myjson_number(myjson_val*, double);
//释放空间
void myjson_free(myjson_val*);

#define MYJSON_SET_NULL(v)  myjson_free(v)

myjson_err get_myjson_boolean(const myjson_val*);

void set_myjson_boolean(myjson_val*, int);

const char* get_myjson_string(const myjson_val*);
size_t get_myjson_string_length(const myjson_val*);
void set_myjson_string(myjson_val*, const char*, size_t);

size_t myjson_get_array_size(const myjson_val*);
myjson_val* myjson_get_array_element(const myjson_val*, size_t);

size_t myjson_get_object_size(const myjson_val*);
const char* myjson_get_object_key(const myjson_val*, size_t);
size_t myjson_get_object_key_length(const myjson_val*, size_t);
myjson_val* myjson_get_object_value(const myjson_val*, size_t);

void myjson_set_array(myjson_val*, size_t);
size_t myjson_get_array_capacity(const myjson_val*);
void myjson_reserve_array(myjson_val*, size_t);
void myjson_shrink_array(myjson_val*);
void myjson_clear_array(myjson_val*);
myjson_val* myjson_pushback_array_element(myjson_val*);
void myjson_popback_array_element(myjson_val*);
myjson_val* myjson_insert_array_element(myjson_val*, size_t);
void myjson_erase_array_element(myjson_val*, size_t, size_t);

void myjson_set_object(myjson_val*, size_t);
size_t myjson_get_object_capacity(const myjson_val*);
void myjson_reserve_object(myjson_val*, size_t);
void myjson_shrink_object(myjson_val*);
void myjson_clear_object(myjson_val*);
size_t myjson_find_object_index(const myjson_val*, const char*, size_t);
myjson_val* myjson_find_object_value(myjson_val*, const char*, size_t);
myjson_val* myjson_set_object_value(myjson_val*, const char*, size_t);
void myjson_remove_object_value(myjson_val*, size_t);


#endif