#include <wchar.h>
#include <stdlib.h>
#include "lexer.hpp"
#include "memory.hpp"
#include "langs.hpp"

ProtoString::ProtoString()
{
	this->len   = 0;
	this->cap   = 0;
	this->chars = NULL;
}
ProtoString::~ProtoString()
{
	FREE_ARRAY(char, this->chars, this->cap);
}
void ProtoString::write(char ch)
{
	if (this->len >= this->cap)
	{
		int old = this->cap;
		this->cap = GROW(old);
		this->chars = GROW_ARRAY(char, this->chars, old, this->cap); 
	}
	this->chars[this->len] = ch;
	++this->len;
}

Lexer::Lexer(const char* str, Lang lang)
{
	this->start = str;
	this->curr  = str;
	this->line  = 1;
	this->col   = 0;
	this->lang  = lang;
}
bool Lexer::fin()
{
	return *this->curr == '\0';
}
wchar_t Lexer::advance()
{
	this->col++;
	wchar_t result;
	this->curr += mbrtowc(&result, this->curr, sizeof(wchar_t), NULL);
	return result;
}
wchar_t Lexer::peek()
{
	wchar_t result;
	mbrtowc(&result, this->curr, sizeof(wchar_t), NULL);
	return result;
}
bool Lexer::spy(wchar_t x)
{
	return this->peek() == x;
}
bool Lexer::match(wchar_t x)
{
	if (this->spy(x))
	{
		this->advance();
		return true;
	}
	return false;
}
bool Lexer::match(const char* x)
{
	int i = 0;
	for (;;)
	{
		if (x[i] == '\0')
		{
			break;
		}
		if (this->curr[i] == '\0' || this->curr[i] != x[i])
		{
			return false;
		}
		++i;
	}
	this->curr += i;
	return true;
}
bool Lexer::is_alpha(wchar_t x)
{
	return iswalpha(x) || x == '_';
}
bool Lexer::is_digit(wchar_t x)
{
	return x >= '0' && x <= '9';
}
void Lexer::skip_useless()
{
	for (;;)
	{
		if (this->fin())
		{
			break;
		}
		switch (this->peek())
		{
			case ' ':
			case '\t':
				this->advance();
				break;
			case ';':
				while (!this->fin() && !this->spy('\n'))
				{
					this->advance();
				}
				break;
			default:
				return;
		}
	}
}
void Lexer::new_line()
{
	this->line++;
	this->col = 0;
}
Token Lexer::new_token(TokenType type)
{
	return {
		.type   = type,
		.start  = this->start,
		.length = (int)(this->curr - this->start),
		.line   = this->line,
		.col    = this->col
	};
}
Token Lexer::new_token(TokenType type, Value value)
{
	return {
		.type   = type,
		.start  = this->start,
		.length = (int)(this->curr - this->start),
		.line   = this->line,
		.col    = this->col,
		.value  = value
	};
}
TokenType Lexer::resolve_keyword(
	int start, int len,
	const char* rest, TokenType type)
{
	if (
		this->curr - this->start == start + len &&
		memcmp(this->start + start, rest, len) == 0)
	{
		return type;
	}
	return TOKEN_ID;
}


TokenType Lexer::resolve_ident()
{
	Keyword keywords[2][16]
	{
		[EN] = {
			{ "if",     2, TOKEN_IF     },
			{ "let",    3, TOKEN_LET    },
			{ "dec",    3, TOKEN_DEC    },
			{ "else",   4, TOKEN_ELSE   },
			{ "this",   4, TOKEN_THIS   },
			{ "true",   4, TOKEN_TRUE   },
			{ "false",  5, TOKEN_FALSE  },
			{ "null",   4, TOKEN_NULL   },
			{ "for",    3, TOKEN_FOR    },
			{ "call",   4, TOKEN_CALL   },
			{ "jump",   4, TOKEN_JUMP   },
			{ "break",  5, TOKEN_BREAK  },
			{ "while",  5, TOKEN_WHILE  },
			{ "match",  5, TOKEN_MATCH  },
			{ "return", 6, TOKEN_RETURN },

			{ NULL,     0, TOKEN_FIN    },
		},
		[ES] = {
			{ "si",       2, TOKEN_IF     },
			{ "sino",     4, TOKEN_ELSE   },
			{ "por",      3, TOKEN_FOR    },
			{ "mientras", 8, TOKEN_WHILE  },
			{ "salta",    5, TOKEN_JUMP   },
			{ "sale",     4, TOKEN_BREAK  },
			{ "coincide", 8, TOKEN_MATCH  },
			{ "deja",     4, TOKEN_LET    },
			{ "dec",      3, TOKEN_DEC    },
			{ "esto",     4, TOKEN_THIS   },
			{ "llama",    5, TOKEN_CALL   },
			{ "verdad",   6, TOKEN_TRUE   },
			{ "falso",    5, TOKEN_FALSE  },
			{ "nulo",     4, TOKEN_NULL   },
			{ "regresa",  7, TOKEN_RETURN },

			{ NULL,       0, TOKEN_FIN    },
		},
	};
	auto i = 0;
	auto remainder = strlen(this->start);
	for (;;)
	{
		auto keyword = &keywords[EN][i];
		if (keyword->word == NULL)
		{
			break;
		}
		if (
			remainder >= keyword->length &&
			memcmp(this->start, keyword->word, keyword->length) == 0)
		{
			return keyword->type;
		}
		++i;
	}
	return TOKEN_ID;
}
Token Lexer::double_str(VM* vm)
{
	ProtoString string;
	bool done = false;
	TokenType type = TOKEN_STRING;
	do
	{
		if (this->fin())
		{
			//finish_ProtoString(&string);
			//lex_error(lexer, "Unclosed string literal.");
			//return new_token(token_fin, lexer);
		}

		const char curr = *(this->curr++);
		switch (curr)
		{
			// The terminator:
			case '"':
				done = true;
				break;
			// Escape sequences:
			case '\\':
			{
				switch (this->peek())
				{
					case 'n':
						string.write('\n');
						break;
					case 'r':
						string.write('\r');
						break;
					case 'f':
						string.write('\f');
						break;
					case 't':
						string.write('\t');
						break;
					case 'v':
						string.write('\v');
						break;
					case 'a':
						string.write('\a');
						break;
					case 'b':
						string.write('\b');
						break;
					case '"':
						string.write('"');
						break;\
					case '\\':
						string.write('\\');
						break;
					case '\n':
						this->new_line();
						break;
					default:
						string.write('\0');
						//return lex_error(lexer, "Unrecognized escape: \\%c.", look(lexer));
				}
				++this->curr;
				break;
			}
			default:
				if (curr == '\n')
				{
					this->new_line();
				}
				string.write(curr);
				break;
		}
	} while (!done);
	string.write('\0');
	auto final_string = copy_string(vm, string.chars, string.len - 1);
	printf("String: %s\n", final_string->chars);
	return this->new_token(type, OBJ_VAL((Obj*)final_string));
}
Token Lexer::single_str(VM* vm)
{
	ProtoString string;
	bool done = false;
	TokenType type = TOKEN_STRING;
	do
	{
		if (this->fin())
		{
			//finish_ProtoString(&string);
			//lex_error(lexer, "Unclosed string literal.");
			//return new_token(token_fin, lexer);
		}

		const char curr = *(this->curr++);
		switch (curr)
		{
			// The terminator:
			case '\'':
				done = true;
				break;
			// Escape sequences:
			case '\\':
			{
				switch (this->peek())
				{
					case 'n':
						string.write('\n');
						break;
					case 'r':
						string.write('\r');
						break;
					case 'f':
						string.write('\f');
						break;
					case 't':
						string.write('\t');
						break;
					case 'v':
						string.write('\v');
						break;
					case 'a':
						string.write('\a');
						break;
					case 'b':
						string.write('\b');
						break;
					case '\'':
						string.write('\'');
						break;\
					case '\\':
						string.write('\\');
						break;
					case '\n':
						this->new_line();
						break;
					case '#':
						string.write('#');
						break;
					default:
						string.write('\0');
						//return lex_error(lexer, "Unrecognized escape: \\%c.", look(lexer));
				}
				++this->curr;
				break;
			}
			case '#':
				if (this->match('{'))
				{
					type = TOKEN_INTERP;
					++this->interp_count;
					this->interps[this->interp_count - 1] = 1;
					done = true;
					break;
				}
				else
				{
				}
			default:
				if (curr == '\n')
				{
					this->new_line();
				}
				string.write(curr);
				break;
		}
	} while (!done);
	string.write('\0');
	auto final_string = copy_string(vm, string.chars, string.len - 1);
	printf("String: %s\n", final_string->chars);
	return this->new_token(type, OBJ_VAL((Obj*)final_string));
}
Token Lexer::scan(VM* vm)
{
	this->skip_useless();
	this->start = this->curr;
	if (this->fin())
	{
		return this->new_token(TOKEN_FIN);
	}
	wchar_t c = this->advance();
	switch (c)
	{
		case '+':
			if (this->match('='))
			{
				return this->new_token(TOKEN_ADD_SET);
			}
			if (this->match('+'))
			{
				return this->new_token(TOKEN_PLUS_PLUS);
			}
			return this->new_token(TOKEN_ADD);
		case '-':
			if (this->match('='))
			{
				return this->new_token(TOKEN_SUB_SET);
			}
			if (this->match('-'))
			{
				return this->new_token(TOKEN_SUB_SUB);
			}
			if (this->match('>'))
			{
				return new_token(TOKEN_ARROW);
			}
			return this->new_token(TOKEN_SUB);
		case '*':
			if (this->match('='))
			{
				return this->new_token(TOKEN_MUL_SET);
			}
			return this->new_token(TOKEN_MUL);
		case '/':
			if (this->match('='))
			{
				return this->new_token(TOKEN_DIV_SET);
			}
			return this->new_token(TOKEN_DIV);
		case '^':
			if (this->match('='))
			{
				return this->new_token(TOKEN_EXP_SET);
			}
			return this->new_token(TOKEN_EXP);
		case '%':
			if (this->match('='))
			{
				return this->new_token(TOKEN_MOD_SET);
			}
			return this->new_token(TOKEN_MOD);
		case '=':
			if (this->match('='))
			{
				return this->new_token(TOKEN_EQUIV);
			}
			if (this->match('>'))
			{
				return this->new_token(TOKEN_FAT_ARROW);
			}
			return this->new_token(TOKEN_SET);
		case '<':
			if (this->match('='))
			{
				return this->new_token(TOKEN_LE);
			}
			if (this->match('<'))
			{
				if (this->match('='))
				{
					return new_token(TOKEN_LSHIFT_SET);
				}
				return this->new_token(TOKEN_LSHIFT);
			}
			return this->new_token(TOKEN_LT);
		case '>':
			if (this->match('='))
			{
				return this->new_token(TOKEN_GE);
			}
			if (this->match('>'))
			{
				if (this->match('='))
				{
					return this->new_token(TOKEN_RSHIFT_SET);
				}
				return this->new_token(TOKEN_RSHIFT);
			}
			return this->new_token(TOKEN_GT);
		case '&':
			if (this->match('&'))
			{
				return this->new_token(TOKEN_AMP_AMP);
			}
			if (this->match('='))
			{
				return this->new_token(TOKEN_BAND_SET);
			}
			return this->new_token(TOKEN_BAND);
		case '|':
			if (this->match('|'))
			{
				return this->new_token(TOKEN_PIP_PIP);
			}
			if (this->match('='))
			{
				return this->new_token(TOKEN_BOR_SET);
			}
			return this->new_token(TOKEN_BOR);
		case '?':
			if (this->match('!'))
			{
				return this->new_token(TOKEN_COAL);
			}
			return this->new_token(TOKEN_QUESTION);
		case '~':
			if (this->match('='))
			{
				return this->new_token(TOKEN_TIL_SET);
			}
			return this->new_token(TOKEN_TIL);
		case '\n':
			this->new_line();
			return this->new_token(TOKEN_ENDL);
		case '(':
			return this->new_token(TOKEN_LPAREN);
		case ')':
			return this->new_token(TOKEN_RPAREN);
		case '[':
			return this->new_token(TOKEN_LBRACE);
		case ']':
			return this->new_token(TOKEN_RBRACK);
		case '{':
			if (this->interp_count > 0)
			{
				++this->interps[this->interp_count - 1];
			}
			return this->new_token(TOKEN_LBRACE);
		case '}':
			if (this->interp_count > 0)
			{
				--this->interps[this->interp_count - 1];
				if (this->interps[this->interp_count - 1] == 0)
				{
					--this->interp_count;
					return this->single_str(vm);
				}
			}
			return this->new_token(TOKEN_RBRACE);
		case ':':
			return this->new_token(TOKEN_COLON);
		case ',':
			return this->new_token(TOKEN_COMMA);
		case '"':
			return this->double_str(vm);
		case '\'':
			return this->single_str(vm);
		default:
		{
			if (this->is_alpha(c))
			{
				while (
					!this->fin() &&
					(this->is_alpha(this->peek()) || this->is_digit(this->peek())))
				{
					this->advance();
				}
				return this->new_token(this->resolve_ident());
			}
			else if (this->is_digit(c))
			{
				TokenType type = TOKEN_INT;
				while (!this->fin() && this->is_digit(this->peek()))
				{
					this->advance();
				}
				if (this->match('.'))
				{
					type = TOKEN_REAL;
					while (this->is_digit(this->peek()))
					{
						this->advance();
					}
				}
				if (type == TOKEN_INT)
				{
					return this->new_token(
						type,
						INT_VAL(strtol(this->start, NULL, 10)));
				}
				else
				{
					return this->new_token(
						type,
						REAL_VAL(strtod(this->start, NULL)));
				}
			}
		}
	}
	return this->new_token(TOKEN_ERROR);
}
