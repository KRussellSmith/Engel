#include <wchar.h>
#include <stdlib.h>
#include "lexer.hpp"
Lexer::Lexer(const char* str)
{
	this->start = str;
	this->curr  = str;
	this->line  = 1;
	this->col   = 0;
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
	switch (this->start[0])
	{
		case 'i':
			return this->resolve_keyword(
				1, 1,
				"f", TOKEN_IF);
		case 'l':
			return this->resolve_keyword(
				1, 2,
				"et", TOKEN_LET);
		case 'd':
			return this->resolve_keyword(
				1, 2,
				"ec", TOKEN_DEC);
		case 'e':
			return this->resolve_keyword(
				1, 3,
				"lse", TOKEN_ELSE);
		case 't':
		{
			if (this->curr - this->start > 1)
			{
				switch (this->start[1])
				{
					case 'h':
						return this->resolve_keyword(
							2, 2,
							"is", TOKEN_THIS);
					case 'r':
						return this->resolve_keyword(
							2, 2,
							"ue", TOKEN_TRUE);
				}
			}
			break;
		}
		case 'f':
		{
			if (this->curr - this->start > 1)
			{
				switch (this->start[1])
				{
					case 'a':
						return this->resolve_keyword(
							2, 3,
							"lse", TOKEN_FALSE);
					case 'o':
						return this->resolve_keyword(
							2, 1,
							"r", TOKEN_FOR);
				}
			}
			break;
		}
		case 'c':
			return this->resolve_keyword(
				1, 3,
				"all", TOKEN_CALL);
		case 'j':
			return this->resolve_keyword(
				1, 3,
				"ump", TOKEN_JUMP);
		case 'w':
			return this->resolve_keyword(
				1, 4,
				"hile", TOKEN_WHILE);
		case 'b':
			return this->resolve_keyword(
				1, 4,
				"reak", TOKEN_BREAK);
			break;
		case 'm':
			return this->resolve_keyword(
				1, 4,
				"atch", TOKEN_MATCH);
		case 'r':
			return this->resolve_keyword(
				1, 5,
				"eturn", TOKEN_RETURN);
	}
	return TOKEN_ID;
}
Token Lexer::scan()
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
			/*if (lexer->interp_count > 0)
			{
				++lexer->interps[lexer->interp_count];
			}*/
			return this->new_token(TOKEN_LBRACE);
		case '}':
			/*if (lexer->interp_count > 0)
			{
				--lexer->interps[lexer->interp_count - 1];
				if (lexer->interps[lexer->interp_count - 1] == 0)
				{
					//--lexer->interp_count;
					//return single_string(lexer);
				}
			}*/
			return this->new_token(TOKEN_RBRACE);
		case ':':
			return this->new_token(TOKEN_COLON);
		case ',':
			return this->new_token(TOKEN_COMMA);
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
				return this->new_token(type);
			}
		}
	}
	return this->new_token(TOKEN_ERROR);
}
