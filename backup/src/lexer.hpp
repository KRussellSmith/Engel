#ifndef lexer_header
#define lexer_header
#include <wchar.h>
typedef enum
{
	TOKEN_LPAREN,     // (
	TOKEN_RPAREN,     // )
	TOKEN_LBRACK,     // [
	TOKEN_RBRACK,     // ]
	TOKEN_LBRACE,     // {
	TOKEN_RBRACE,     // }
	TOKEN_ENDL,       // ;
	TOKEN_COLON,      // :
	TOKEN_DOT,        // .
	TOKEN_DOTDOT,     // ..
	TOKEN_DOTDOTDOT,  // ...
	TOKEN_COMMA,      // ,
	TOKEN_MUL,        // *
	TOKEN_MUL_SET,    // *=
	TOKEN_DIV,        // /
	TOKEN_DIV_SET,    // /=
	TOKEN_MOD,        // %
	TOKEN_MOD_SET,    // %=
	TOKEN_ADD,        // +
	TOKEN_SUB,        // -
	TOKEN_ADD_SET,    // +=
	TOKEN_SUB_SET,    // -=
	TOKEN_LSHIFT,     // <<
	TOKEN_LSHIFT_SET, // <<=
	TOKEN_RSHIFT,     // >>
	TOKEN_RSHIFT_SET, // >>=
	TOKEN_BOR,        // |
	TOKEN_BOR_SET,    // |=
	TOKEN_PIP_PIP,    // ||
	TOKEN_EXP,        // ^
	TOKEN_EXP_SET,    // ^=
	TOKEN_BAND,       // &
	TOKEN_BAND_SET,   // &=
	TOKEN_AMP_AMP,    // &&
	TOKEN_BANG,       // !
	TOKEN_TIL,        // ~
	TOKEN_TIL_SET,    // ~=
	TOKEN_QUESTION,   // ?
	TOKEN_SET,        // =
	TOKEN_LT,         // <
	TOKEN_GT,         // >
	TOKEN_LE,         // <=
	TOKEN_GE,         // >=
	TOKEN_EQUIV,      // ==
	TOKEN_BANG_EQ,    // !=
	TOKEN_PLUS_PLUS,  // ++
	TOKEN_SUB_SUB,    // --
	TOKEN_BREAK,      // break
	TOKEN_ELSE,       // else
	TOKEN_FALSE,      // false
	TOKEN_FOR,        // for
	TOKEN_IF,         // if
	TOKEN_IMPORT,     // import
	TOKEN_IN,         // in
	TOKEN_NULL,       // null
	TOKEN_RETURN,     // return
	TOKEN_THIS,       // this
	TOKEN_TRUE,       // true
	TOKEN_LET,        // let
	TOKEN_DEC,        // dec
	TOKEN_WHILE,      // while
	TOKEN_MATCH,      // match
	TOKEN_JUMP,       // jump
	TOKEN_CALL,       // call
	TOKEN_ARROW,      // ->
	TOKEN_FAT_ARROW,  // =>
	
	TOKEN_ID,         // « identifier »
	TOKEN_INT,        // « integer »
	TOKEN_REAL,       // « real »
	TOKEN_STRING,     // « string »
	TOKEN_INTERP,     // « interpolation »
	TOKEN_ERROR,
	TOKEN_FIN
} TokenType;

typedef struct
{
	TokenType   type;
	const char* start;
	int         length;
	int         line;
	int         col;
} Token;
class Lexer
{
private:
	const char* start;
	const char* curr;
	int   line;
	int   col;
	
	TokenType resolve_keyword(
		int start, int len,
		const char* rest, TokenType type);
	TokenType resolve_ident();
	bool      fin();
	wchar_t   advance();
	wchar_t   peek();
	bool      spy(wchar_t x);
	bool      match(wchar_t x);
	bool      match(const char* x);
	bool      is_alpha(wchar_t x);
	bool      is_digit(wchar_t x);
	void      skip_useless();
	void      new_line();
	Token     new_token(TokenType type);
public:
	Lexer(const char* str);
	Token scan();
};
#endif
