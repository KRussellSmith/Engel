#ifndef value_header
#define value_header
#include <stddef.h>
#include <stdint.h>
#include "obj.hpp"

typedef enum
{
	VALUE_INT,
	VALUE_REAL,
	VALUE_BOOL,
	VALUE_NULL,
	VALUE_OBJ,
} ValueType;

typedef struct
{
	ValueType type;
	union
	{
		double  real;
		int64_t integer;
		bool    boolean;
		Obj*    obj;
	} as;
} Value;
typedef struct Obj Obj;
typedef struct ObjString ObjString;

#define INT_VAL(x)  ((Value) {VALUE_INT,  {.integer=x}})
#define REAL_VAL(x) ((Value) {VALUE_REAL, {.real=x}})
#define BOOL_VAL(x) ((Value) {VALUE_BOOL, {.boolean=x}})
#define OBJ_VAL(x)  ((Value) {VALUE_OBJ,  {.obj=x}})

#define AS_INT(x)  ((x).as.integer)
#define AS_REAL(x) ((x).as.real)
#define AS_BOOL(x) ((x).as.boolean)
#define AS_OBJ(x)  ((x).as.obj)

#define IS_INT(x)  ((x).type == VALUE_INT)
#define IS_REAL(x) ((x).type == VALUE_REAL)
#define IS_BOOL(x) ((x).type == VALUE_BOOL)
#define IS_OBJ(x)  ((x).type == VALUE_OBJ)

typedef struct
{
	int len;
	int cap;
	Value* values;
} ValueArray;

void init_ValueArray(ValueArray* array);
void push_ValueArray(ValueArray* array, Value value);
void free_ValueArray(ValueArray* array);
#endif
