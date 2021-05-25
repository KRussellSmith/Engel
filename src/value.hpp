#ifndef value_header
#define value_header

#include <stddef.h>
#include <stdint.h>
#include "chunk.hpp"

typedef enum
{
	OBJ_STRING,
	OBJ_MAP,
	OBJ_FUNCTION,
	OBJ_UPVALUE,
	OBJ_CLOSURE,
	OBJ_NATIVE,
} ObjType;

typedef struct Obj
{
	ObjType     type;
	struct Obj* next;
	bool        marked;
} Obj;

typedef enum
{
	VALUE_INT,
	VALUE_REAL,
	VALUE_BOOL,
	VALUE_NULL,
	VALUE_OBJ,
	VALUE_UNDEFINED,
} ValueType;

typedef struct Value
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

typedef struct
{
	Obj      header;
	uint64_t len;
	char*    chars;
	uint32_t hash;
} ObjString;

typedef struct
{
	ObjString* key;
	Value      value;
} MapEntry;

typedef struct
{
	Obj header;
	uint64_t len;
	uint64_t cap;
	MapEntry* entries;
} Map;

typedef struct
{
	Obj header;
	int arity;
	uint64_t num_upvalues;
	Chunk chunk;
	ObjString* name;
} Function;

typedef struct Upvalue
{
	Obj header;
	Value* loc;
	Value  closed;
	struct Upvalue* next;
} Upvalue;

typedef struct
{
	Obj header;
	Function* func;
	Upvalue** upvalues;
	uint64_t num_upvalues;
} Closure;

typedef Value (*NativeFunc)(struct VM* vm, int num_args, Value* args);

typedef struct
{
	Obj header;
	NativeFunc func;
} Native;

#define OBJ_TYPE(val) (AS_OBJ(val)->type)

#define IS_STRING(val) isobjtype(val, OBJ_STRING)

#define IS_MAP(val) isobjtype(val, OBJ_MAP)

#define IS_FUNCTION(val) isobjtype(val, OBJ_FUNCTION)
#define IS_NATIVE(val) isobjtype(val, OBJ_NATIVE)
#define IS_CLOSURE(val) isobjtype(val, OBJ_CLOSURE)

#define AS_STRING(val) ((ObjString*)AS_OBJ(val))

#define AS_MAP(val) ((Map*)AS_OBJ(val))

#define AS_FUNC(val) ((Function*)AS_OBJ(val))
#define AS_NATIVE(val) (((Native*)AS_OBJ(val))->func)
#define AS_CLOSURE(val) ((Closure*)AS_OBJ(val))

#define AS_CSTRING(val) (((ObjString*)AS_OBJ(val))->chars)


#define INT_VAL(x)  ((Value) {VALUE_INT,  {.integer=x}})
#define REAL_VAL(x) ((Value) {VALUE_REAL, {.real=x}})
#define BOOL_VAL(x) ((Value) {VALUE_BOOL, {.boolean=x}})
#define OBJ_VAL(x)  ((Value) {VALUE_OBJ,  {.obj=(Obj*)x}})
#define NULL_VAL    ((Value) {VALUE_NULL, {.integer=0}})

#define AS_INT(x)  ((x).as.integer)
#define AS_REAL(x) ((x).as.real)
#define AS_BOOL(x) ((x).as.boolean)
#define AS_OBJ(x)  ((x).as.obj)

#define IS_INT(x)  ((x).type == VALUE_INT)
#define IS_REAL(x) ((x).type == VALUE_REAL)
#define IS_BOOL(x) ((x).type == VALUE_BOOL)
#define IS_OBJ(x)  ((x).type == VALUE_OBJ)
#define IS_NULL(x) ((x).type == VALUE_NULL)

void print_val(Value val);
void print_obj(Obj*  obj);

static inline bool isobjtype(Value val, ObjType type)
{
	return IS_OBJ(val) && AS_OBJ(val)->type == type;
}

Function* new_function(struct VM* vm);
Upvalue*  new_upvalue(struct VM* vm, Value* slot);
Closure*  new_closure(struct VM* vm, Function* func);
Native*   new_native(struct VM* vm, NativeFunc func);
ObjString* copy_string(struct VM* vm, const char* chars, int len);
ObjString* take_string(struct VM* vm, char* chars, int len);

Map* new_map(struct VM* vm);
void init_map(Map* map);
bool put_map(Map* map, ObjString* key, Value value);
bool get_map(Map* map, ObjString* key, Value* value);
bool rm_map(Map* map, ObjString* key);
void copy_map(Map* from, Map* to);
ObjString* map_find_str(Map* map, const char* chars, int len, uint32_t hash);
void free_map(Map* map);
#endif
