#ifndef obj_header
#define obj_header
#include "value.hpp"

typedef enum
{
	OBJ_STRING,
} ObjType;

struct Obj
{
	ObjType type;
};
struct ObjString
{
	Obj      header;
	uint64_t len;
	char*    chars;
};

#endif
