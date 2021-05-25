#include <string.h>
#include <jni.h>
#include <wchar.h>
#include <stdio.h>
#include "value.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "compiler.hpp"
#include "chunk.hpp"
#include "node.hpp"
#include "vm.hpp"
#include "langs.hpp"
/*Lexer lexer(
	"match 30 {"
	"	20, 10 => 55"
	"	56, 77 => 30"
	"} else 100");
*/
//Lexer lexer("20 + 34");
uint64_t readULEB(int* i, uint8_t* ops)
{
	uint64_t result = 0;
	uint8_t shift = 0;
	uint8_t val;
	for (;;)
	{
		val = ops[(*i)++];
		result |= (uint64_t)(val & 0x7F) << shift;
		if ((val & 0x80) == 0)
			break;
		
		shift += 7;
	}
	return result;
}
uint32_t readUint32(int* i, uint8_t* ops)
{
	return (
		(ops[(*i)++] << 24) |
		(ops[(*i)++] << 16) |
		(ops[(*i)++] << 8)  |
		(ops[(*i)++]));
}
void dis(Chunk* chunk, int indent);
void dis_obj(Value val, int indent)
{
	switch (AS_OBJ(val)->type)
	{
		case OBJ_STRING:
			printf("‘%s’", AS_CSTRING(val));
			break;
		case OBJ_FUNCTION:
			printf("function:\n");
			dis(&AS_FUNC(val)->chunk, indent + 1);
			break;
		// native functions are not a constant, as there is no way for them to be
		// user-defined; so no need for dissassembler support.
		default: break;
	}
}
void dis_val(Value val, int indent)
{
	switch (val.type)
	{
		case VALUE_INT:
			printf("%ld", AS_INT(val));
			break;
		case VALUE_REAL:
			printf("%g", AS_REAL(val));
			break;
		case VALUE_OBJ:
			dis_obj(val, indent);
			break;
		case VALUE_BOOL:
			printf("%s", AS_BOOL(val)? "true" : "false");
			break;
		case VALUE_NULL:
			printf("%s", "null");
			break;
	}
}
void dis(Chunk* chunk, int indent)
{
	int i = 0;
	int j;
	for (;;)
	{
		if (i >= chunk->len) break;
		uint8_t op = chunk->code[i++];
		for (j = 0; j < indent; ++j)
		{
			printf("  ");
		}
		printf("%-4X ", i - 1);
		switch (op)
		{
			case OP_POP:
				printf("POP\n");
				break;
			case OP_DUP:
				printf("DUP\n");
				break;
			case OP_ROT2:
				printf("ROT DOUBLE\n");
				break;
			case OP_ROT3:
				printf("ROT TRIPLE\n");
				break;
			case OP_ROT4:
				printf("ROT QUAD\n");
				break;
			case OP_TRUE:
				printf("TRUE\n");
				break;
			case OP_FALSE:
				printf("FALSE\n");
				break;
			case OP_NULL:
				printf("NULL\n");
				break;
			case OP_NEG:
				printf("NEG\n");
				break;
			case OP_BNOT:
				printf("BNOT\n");
				break;
			case OP_NOT:
				printf("NOT\n");
				break;
			case OP_TO_STR:
				printf("TO STRING\n");
				break;
			case OP_CONCAT:
				printf("CONCAT\n");
				break;
			case OP_ADD:
				printf("ADD\n");
				break;
			case OP_SUB:
				printf("SUB\n");
				break;
			case OP_DIV:
				printf("DIV\n");
				break;
			case OP_MUL:
				printf("MUL\n");
				break;
			case OP_MOD:
				printf("MOD\n");
				break;
			case OP_LSHIFT:
				printf("LSHIFT\n");
				break;
			case OP_RSHIFT:
				printf("RSHIFT\n");
				break;
			case OP_BOR:
				printf("BOR\n");
				break;
			case OP_BAND:
				printf("BAND\n");
				break;
			case OP_XOR:
				printf("XOR\n");
				break;
			case OP_LT:
				printf("LT\n");
				break;
			case OP_GT:
				printf("GT\n");
				break;
			case OP_LE:
				printf("LE\n");
				break;
			case OP_GE:
				printf("GE\n");
				break;
			case OP_EQUIV:
				printf("EQUIV\n");
				break;
			case OP_NOT_EQUIV:
				printf("NOT EQUIV\n");
				break;
			case OP_CONST:
			{
				int64_t index = readULEB(&i, chunk->code);
				auto val = chunk->consts.values[index];
				printf("CONST [%lX] := ", index);
				dis_val(val, indent);
				/*if (IS_FUNC(val))
				{
					printf(" <%02X> {\n", AS_FUNC(val)->arity);
					dis(&(AS_FUNC(val)->chunk), indent + 1);
					for (j = 0; j < indent; ++j)
					{
						printf("  ");
					}
					printf("}");
				}*/
				printf("\n");
				break;
			}
			case OP_DEF_VAR:
			{
				int64_t index = readULEB(&i, chunk->code);
				auto val = chunk->consts.values[index];
				printf("DEF VAR ");
				dis_val(val, indent);
				printf(" [%lx]\n", index);
				break;
			}
			case OP_GET_VAR:
			{
				int64_t index = readULEB(&i, chunk->code);
				auto val = chunk->consts.values[index];
				printf("GET VAR ");
				dis_val(val, indent);
				printf(" [%lx]\n", index);
				break;
			}
			case OP_GET_LOCAL:
			{
				printf("GET LOCAL [%lX]\n", readULEB(&i, chunk->code));
				break;
			}
			case OP_SET_LOCAL:
			{
				printf("SET LOCAL [%lX]\n", readULEB(&i, chunk->code));
				break;
			}
			case OP_GET_UPVAL:
			{
				printf("GET UPVALUE [%lX]\n", readULEB(&i, chunk->code));
				break;
			}
			case OP_SET_UPVAL:
			{
				printf("SET UPVALUE [%lX]\n", readULEB(&i, chunk->code));
				break;
			}
			case OP_CLOSE:
			{
				printf("CLOSE UPVALUE\n");
				break;
			}
			case OP_CLOSURE:
			{
				auto index = readULEB(&i, chunk->code);
				auto val = chunk->consts.values[index];
				printf("CLOSURE [%lX] := ", index);
				//print_val(val);

				printf(" <%02X> {\n", AS_FUNC(val)->arity);
				dis(&(AS_FUNC(val)->chunk), indent + 1);
				int j;
				for (j = 0; j < indent; ++j)
				{
					printf("  ");
				}
				printf("} [");
				for (j = 0; j < AS_FUNC(val)->num_upvalues; ++j)
				{
					printf("{ %s, %lu }", chunk->code[i++] ? "true" : "false", readULEB(&i, chunk->code));
					if (j < AS_FUNC(val)->num_upvalues - 1)
					{
						printf(", ");
					}
				}
				printf("]\n");
				break;
			}
			case OP_CALL:
			{
				printf("CALL [%lX]\n", readULEB(&i, chunk->code));
				break;
			}/*
			case OP_SUB_GET:
			{
				printf("SUBSCRIPT\n");
				break;
			}
			case OP_SUB_SET:
			{
				printf("SET SUBSCRIPT\n");
				break;
			}
			case OP_ARRAY:
			{
				printf("GEN ARRAY [%lX]\n", readULEB(&i, chunk->code));
				break;
			}*/
			case OP_JMP:
			{
				uint32_t start = i - 1;
				uint32_t end  = readUint32(&i, chunk->code);
				printf("JMP %X - %X\n", start, i + end);
				break;
			}
			/*case OP_CND:
				printf("CND %X - %X\n", i - 1, i - 1 + readUint32(&i, chunk->code));
				break;
			case OP_CND_NOT:
				printf("CND NOT %X - %X\n", i - 1, i - 1 + readUint32(&i, chunk->code));
				break;*/
			case OP_AND:
			{
				uint32_t start = i - 1;
				uint32_t end  = readUint32(&i, chunk->code);
				printf("AND %X - %X\n", start, i + end);
				break;
			}
			case OP_OR:
			{
				uint32_t start = i - 1;
				uint32_t end  = readUint32(&i, chunk->code);
				printf("OR %X - %X\n", start, i + end);
				break;
			}
			/*case OP_GOTO:
				printf("GOTO %X\n", readUint32(&i, chunk->code));
				break;*/
			case OP_RET:
				printf("RETURN\n");
				break;
			/*case OP_EMIT:
				printf("EMIT\n");
				break;*/
			default:
				printf("Unrecognized code #%X\n", op);
				break;
		}
	}
}
static auto open_file(const char* path)
{
	auto file = fopen(path, "rb");
	fseek(file, 0L, SEEK_END);
	auto size = ftell(file);
	rewind(file);
	auto buffer = (char*)malloc(size + 1);
	auto read = fread(buffer, sizeof(char), size, file);
	buffer[read] = '\0';
	fclose(file);
	return buffer;
}
int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		puts("argument needed.");
		exit(1);
	}
	auto program = open_file(argv[1]);
	VM vm;
	Lexer lexer(program, EN);
	Parser parser(&lexer, &vm, EN);
	Compiler compiler(&parser, &vm);
	auto func = compiler.compile();
	vm.push(OBJ_VAL(func));
	auto closure = new_closure(&vm, func);
	vm.pop();
	vm.push(OBJ_VAL(closure));
	vm.call_val(OBJ_VAL(closure), 0);
	dis(&func->chunk, 0);
	vm.run();
	free(program);
	return 0;
}	
