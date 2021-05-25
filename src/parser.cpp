#include "lexer.hpp"
#include "node.hpp"
#include "parser.hpp"
#include "langs.hpp"
using namespace Node;
Parser::Parser(Lexer* lexer, VM* vm, Lang lang)
{
	this->lexer = lexer;
	this->vm    = vm;
	this->lang  = lang;
	this->advance();
}

bool Parser::fin()
{
	return this->sniff(TOKEN_FIN);
}
void Parser::advance()
{
	this->prev = this->curr;
	this->curr = this->lexer->scan(vm);
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
bool Parser::sniff(const int types[])
{
	int i = 0;
	for (;;)
	{
		auto type = types[i];
		if (type == -1)
		{
			return false;
		}
		if (this->curr.type == type)
		{
			return true;
		}
		++i;
	}
	return false;
}
bool Parser::taste(const int types[])
{
	if (this->sniff(types))
	{
		this->advance();
		return true;
	}
	return false;
}
void Parser::error(const char* msg[])
{
	puts(msg[this->lang]);
}
void Parser::eat(TokenType type, const char* error[])
{
	if (!this->taste(type))
	{
		this->advance();
		this->error(error);
	}
}
void Parser::skip_breaks() // When seconds can't wait…
{
	while (this->taste(TOKEN_ENDL));
}

Base* Parser::func_body()
{
	this->skip_breaks();
	//auto start = this->curr;
	if (this->sniff(TOKEN_LBRACE))
	{
		/*auto result = new Block();
		for (;;)
		{
			if (this->sniff(TOKEN_FIN))
			{
				puts("Unexpected EOF.");
				break;
			}
			if (this->taste(TOKEN_RBRACE))
			{
				break;
			}
			auto stmt = this->dec();
			if (stmt != NULL)
			{
				result->list.push_back(stmt);
			}
		}*/
		return this->block();
	}
	else
	{
		return new Return(this->expr());
	}
	return NULL; // Unreachable, but strangely required to shut up my linter.
}
Base* Parser::finish_call(Base* node)
{
	if (this->taste(TOKEN_LPAREN))
	{
		auto result = new FuncCall(node);
		if (!this->taste(TOKEN_RPAREN))
		{
			//puts("started call");
			do
			{
				this->skip_breaks();
				result->args.push_back(this->expr());
				this->skip_breaks();
			} while (this->taste(TOKEN_COMMA));
			this->eat(TOKEN_RPAREN, (const char*[]) {
				[EN] = "Expected `)`",
				[ES] = "Se esperó `)`",
			});
			//puts("finished call");
		}
		return result;
	}
	return node;
}
Base* Parser::call_to(Base* node)
{
	auto result = node;
	for (;;)
	{
		if (this->sniff(TOKEN_LPAREN))
		{
			result = this->finish_call(result);
		}
		else
		{
			break;
		}
	}
	return result;
}

Base* Parser::factor()
{
	Base* result = NULL;
	if (
			this->taste(TOKEN_INT) ||
			this->taste(TOKEN_STRING))
	{
		result = new Const(&this->prev);
	}
	else if (this->taste(TOKEN_INTERP))
	{
		auto str_interp = new StringInterp;
		do
		{
			auto start = this->prev;
			auto value = this->expr();
			auto interp = new Interpolation(start, value);
			str_interp->list.push_back(interp);
		} while (this->taste(TOKEN_INTERP));
		this->eat(TOKEN_STRING, (const char*[]) {
			[EN] = "Unclosed string",
			[ES] = "Cadena sin callar",
		});
		str_interp->cap = this->prev;
		result = str_interp;
	}
	else if (this->taste(TOKEN_ID))
	{
		result = new Get(this->prev);
	}
	else if (this->taste(TOKEN_TRUE) || this->taste(TOKEN_FALSE))
	{
		this->prev.value = BOOL_VAL(this->prev.type == TOKEN_TRUE);
		return new Const(&this->prev);
	}
	else if (this->taste(TOKEN_MATCH))
	{
		this->skip_breaks();
		auto match = new Match;
		match->comp = this->expr();
		this->skip_breaks();
		this->eat(TOKEN_LBRACE, (const char*[]) {
			[EN] = "Expected `{`",
			[ES] = "Se esperó `{`",
		});
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
			match->cases.push_back(curr);
			do
			{
				this->skip_breaks();
				curr->checks.push_back(this->expr());
				this->skip_breaks();
			} while (this->taste(TOKEN_COMMA));
			this->eat(TOKEN_FAT_ARROW, (const char*[]) {
				[EN] = "Expected `=>`",
				[ES] = "Se esperó `=>`",
			});
			curr->then = this->expr();
		}
		this->skip_breaks();
		this->eat(TOKEN_ELSE, (const char*[]) {
			[EN] = "Expected else clause",
			[ES] = "Se esperó cláusula de `sino`",
		});
		this->skip_breaks();
		match->other = this->expr();
		result = match;
	}
	else if (this->taste(TOKEN_SUB))
	{
		result = new Unary(this->prev.type, this->factor());
	}
	else if (this->taste(TOKEN_LPAREN))
	{
		Func* func = NULL;
		Base* expr = NULL;
		this->skip_breaks();
		// Empty parenthenses = function:
		if (this->sniff(TOKEN_RPAREN))
		{
			func = new Func();
		}
		else
		{
			expr = this->expr();
			this->skip_breaks();
			// (a, ...) = function:
			if (this->taste(TOKEN_COMMA))
			{
				func = new Func();
				func->args.push_back(expr);
				do
				{
					this->skip_breaks();
					func->args.push_back(this->expr());
					this->skip_breaks();
				} while (this->taste(TOKEN_COMMA));
			}
		}
		this->eat(TOKEN_RPAREN, (const char*[]) {
			[EN] = "Expected `)`",
			[ES] = "Se esperó `)`",
		});
		this->skip_breaks();
		if (func != NULL)
		{
			this->eat(TOKEN_ARROW, (const char*[]) {
				[EN] = "Expected `->`",
				[ES] = "Se esperó `->`",
			});
			this->skip_breaks();
			func->body = this->func_body();
			result = func;
		}
		// (...) -> = function:
		else if (this->taste(TOKEN_ARROW))
		{
			func = new Func();
			func->args.push_back(expr);
			this->skip_breaks();
			func->body = this->func_body();
			result = func;
		}
		else
		{
			result = new Group(expr);
		}
	}
	return this->call_to(result);
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
		this->taste(TOKEN_DIV) ||
		this->taste(TOKEN_PER))
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
Base* Parser::band()
{
	Base* result = this->add();
	while (
		this->taste(TOKEN_BAND))
	{
		result = new Binary(this->prev.type, result, this->add());
	}
	return result;
}
Base* Parser::shift()
{
	Base* result = this->band();
	while (
		this->taste(TOKEN_LSHIFT) ||
		this->taste(TOKEN_RSHIFT))
	{
		result = new Binary(this->prev.type, result, this->band());
	}
	return result;
}
Base* Parser::xor_()
{
	Base* result = this->shift();
	while (
		this->taste(TOKEN_TIL))
	{
		result = new Binary(this->prev.type, result, this->shift());
	}
	return result;
}
Base* Parser::bor()
{
	Base* result = this->xor_();
	while (
		this->taste(TOKEN_BOR))
	{
		result = new Binary(this->prev.type, result, this->xor_());
	}
	return result;
}
Base* Parser::comp()
{
	auto expr = this->bor();
	int compare[] = {
		TOKEN_LT,
		TOKEN_GT,
		TOKEN_LE,
		TOKEN_GE,
		TOKEN_EQUIV,
		TOKEN_BANG_EQ,
		-1
	};
	if (this->taste(compare))
	{
		auto result = new Comparisons(expr);
		do
		{
			auto type = this->prev.type;
			this->skip_breaks();
			auto next = this->bor();
			auto comp = new Comparison(type, next);
			result->list.push_back(comp);
		} while (this->taste(compare));
		return result;
	}
	return expr;
}
Base* Parser::and_()
{
	auto result = this->comp();
	while (this->taste(TOKEN_AMP_AMP))
	{
		result = new Cond(this->prev.type, result, this->comp());
	}
	return result;
}
Base* Parser::or_()
{
	auto result = this->and_();
	while (this->taste(TOKEN_PIP_PIP))
	{
		result = new Cond(this->prev.type, result, this->and_());
	}
	return result;
}
Base* Parser::ifelse()
{
	auto result = this->or_();
	if (this->taste(TOKEN_IF))
	{
		auto cond = this->ifelse();
		this->eat(TOKEN_ELSE, (const char*[]) {
			[EN] = "Expected else clause.",
			[ES] = "Se esperó cláusula de `sino`",
		});
		auto other = this->ifelse();
		result = new If(cond, result, other);
	}
	return result;
}
Base* Parser::ass()
{
	auto result = this->ifelse();
	int ops[] = {
		TOKEN_SET,
		TOKEN_ADD_SET,
		TOKEN_SUB_SET,
		TOKEN_MUL_SET,
		TOKEN_DIV_SET,
		TOKEN_EXP_SET,
		TOKEN_LSHIFT_SET,
		TOKEN_RSHIFT_SET,
		TOKEN_BAND_SET,
		TOKEN_BOR_SET,
		TOKEN_TIL_SET,
		-1,
	};
	if (this->taste(ops))
	{
		auto op = this->prev.type;
		this->skip_breaks();
		auto value = this->ass();
		switch (result->type)
		{
			case NODE_GET: break;
			default:
			{
				this->error((const char*[]) {
					[EN] = "Invalid target for assignment.",
					[ES] = "Objetivo invalido para asignación.",
				});
				break;
			}
		}
		return new Set(op, result, value);
	}
	return result;
}
Base* Parser::expr()
{
	return this->ass();
}
Base* Parser::block()
{
	this->eat(TOKEN_LBRACE, (const char*[]) {
		[EN] = "Expected `{`",
		[ES] = "Se esperó `{`",
	});
	auto result = new Block;
	while (!(this->sniff(TOKEN_RBRACE) || this->sniff(TOKEN_FIN)))
	{
		this->skip_breaks();
		auto stmt = this->dec();
		if (stmt == NULL) continue;
		result->list.push_back(stmt);
	}
	this->eat(TOKEN_RBRACE, (const char*[]) {
		[EN] = "Expected `}`",
		[ES] = "Se esperó `}`",
	});
	return result;
}
Base* Parser::stmt()
{
	if (this->taste(TOKEN_ENDL))
	{
		return NULL;
	}
	else if (this->sniff(TOKEN_LBRACE))
	{
		return this->block();
	}
	else if (this->taste(TOKEN_IF))
	{
		auto cond = this->expr();
		this->skip_breaks();
		auto then = this->block();
		this->skip_breaks();
		Base* other = NULL;
		if (this->taste(TOKEN_ELSE))
		{
			this->skip_breaks();
			other = this->stmt();
		}
		return new If(cond, then, other);
	}
	else if (this->taste(TOKEN_WHILE))
	{
		auto cond = this->expr();
		this->skip_breaks();
		auto then = this->block();
		this->skip_breaks();
		Base* other = NULL;
		if (this->taste(TOKEN_ELSE))
		{
			this->skip_breaks();
			other = this->stmt();
		}
		return new While(cond, then, other);
	}
	auto expr = this->expr();
	if (expr == NULL)
	{
		return NULL;
	}
	return new Expr(expr);
}
Base* Parser::dec()
{
	if (this->taste(TOKEN_LET))
	{
		auto result = new Declarations;
		do
		{
			this->skip_breaks();
			this->eat(TOKEN_ID, (const char*[]) {
				[EN] = "Expected identifier",
				[ES] = "Se esperó un identificador",
			});
			auto name = this->prev;
			this->eat(TOKEN_SET, (const char*[]) {
				[EN] = "Expected `=`",
				[ES] = "Se esperó `=`",
			});
			auto value = this->expr();
			auto dec = new Declaration(name, value, true);
			result->list.push_back(dec);
			this->skip_breaks();
		} while (this->taste(TOKEN_COMMA));
		return result;
	}
	return this->stmt();
}
Base* Parser::parse()
{
	Base* result = NULL;
	for (;;)
	{
		if (this->curr.type == TOKEN_FIN)
		{
			return new Base(NODE_FIN);
		}
		result = this->dec();
		if (result != NULL)
		{
			return result;
		}
	}
}
