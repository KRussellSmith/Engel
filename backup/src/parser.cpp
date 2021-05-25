#include "lexer.hpp"
#include "node.hpp"
#include "parser.hpp"
using namespace Node;
Parser::Parser(Lexer* lexer)
{
	this->lexer = lexer;
	this->advance();
}

bool Parser::fin()
{
	return this->sniff(TOKEN_FIN);
}
void Parser::advance()
{
	this->prev = this->curr;
	this->curr = this->lexer->scan();
	if (this->curr.type == TOKEN_ERROR)
	{
		// Error here
	}
}
bool Parser::sniff(TokenType type)
{
	return this->curr.type == type;
}
bool Parser::taste(TokenType type)
{
	if (this->sniff(type))
	{
		this->advance();
		return true;
	}
	return false;
}
void Parser::eat(TokenType type, const char* error)
{
	if (!this->taste(type))
	{
		// Error here
	}
}
void Parser::skip_breaks()
{
	while (this->taste(TOKEN_ENDL));
}
Base* Parser::factor()
{
	if (this->taste(TOKEN_INT))
	{
		return new Const(NODE_INT, &this->prev);
	}
	else if (this->taste(TOKEN_MATCH))
	{
		this->skip_breaks();
		Match* result = new Match();
		result->comp = this->expr();
		this->skip_breaks();
		this->eat(TOKEN_LBRACE, "Expected opening brace");
		for (;;)
		{
			this->skip_breaks();
			if (this->fin())
			{
				break;
			}
			if (this->taste(TOKEN_RBRACE))
			{
				break;
			}
			Case* curr = new Case();
			result->cases.push_back(curr);
			do
			{
				this->skip_breaks();
				curr->checks.push_back(this->expr());
				this->skip_breaks();
			} while (this->taste(TOKEN_COMMA));
			this->eat(TOKEN_FAT_ARROW, "Expected fat arrow (=>)");
			curr->then = this->expr();
		}
		this->skip_breaks();
		this->eat(TOKEN_ELSE, "expected else");
		this->skip_breaks();
		result->other = this->expr();
		return result;
	}
	return NULL;
}
Base* Parser::exp()
{
	Base* result = this->factor();
	if (this->taste(TOKEN_EXP))
	{
		result = new Binary(this->prev.type, result, this->exp());
	}
	return result;
}
Base* Parser::mul()
{
	Base* result = this->exp();
	while (
		this->taste(TOKEN_MUL) ||
		this->taste(TOKEN_MOD) ||
		this->taste(TOKEN_DIV))
	{
		result = new Binary(this->prev.type, result, this->exp());
	}
	return result;
}
Base* Parser::add()
{
	Base* result = this->mul();
	while (
		this->taste(TOKEN_ADD) ||
		this->taste(TOKEN_SUB))
	{
		result = new Binary(this->prev.type, result, this->mul());
	}
	return result;
}
Base* Parser::expr()
{
	return this->add();
}
Base* Parser::stmt()
{
	return this->expr();
}
Base* Parser::dec()
{
	return this->stmt();
}
Base* Parser::parse()
{
	return this->dec();
}
