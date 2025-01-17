// LICENSE
// This software is free for use and redistribution while including this
// license notice, unless:
// 1. is used for commercial or non-personal purposes, or
// 2. used for a product which includes or associated with a blockchain or other
// decentralized database technology, or
// 3. used for a product which includes or associated with the issuance or use
// of cryptographic or electronic currencies/coins/tokens.
// On all of the mentioned cases, an explicit and written permission is required
// from the Author (Ohad Asor).
// Contact ohad@idni.org for requesting a permission. This license may be
// modified over time by the Author.
#include "defs.h"
#include <vector>
#include <array>
#include <iostream>
#include <sys/stat.h>

bool operator==(const lexeme& x, const lexeme& y);

struct elem {
	enum etype { SYM, NUM, CHR, VAR, OPENP, CLOSEP, ALT, STR } type;
	int_t num = 0;
	lexeme e;
	wchar_t ch;
	elem() {}
	elem(int_t num) : type(NUM), num(num) {}
	elem(wchar_t ch) : type(CHR), ch(ch) {}
	elem(etype type, lexeme e) : type(type), e(e) {
		DBG(assert(type!=NUM&&type!=CHR&&(type!=SYM||(e[0]&&e[1])));)
	}
	bool parse(const lexemes& l, size_t& pos);
	bool operator<(const elem& t) const {
		if (type != t.type) return type < t.type;
		if (type == NUM) return num < t.num;
		if (type == CHR) return ch < t.ch;
		if (e[1]-e[0] != t.e[1]-t.e[0]) return e[1]-e[0]<t.e[1]-t.e[0];
		return wcsncmp(e[0], t.e[0], e[1]-e[0]) < 0;
	}
	bool operator==(const elem& t) const {
		if (type != t.type) return false;
		if (type == NUM) return num == t.num;
		if (type == CHR) return ch == t.ch;
		return e == t.e;
	}
};

struct raw_term {
	bool neg = false;
	std::vector<elem> e;
	ints arity;
	bool parse(const lexemes& l, size_t& pos);
	void calc_arity();
	void clear() { e.clear(), arity.clear(); }
};

struct directive {
	elem rel;
	lexeme arg;
	raw_term t;
	int_t n;
	enum etype { STR, FNAME, CMDLINE, STDIN, STDOUT, TREE, TRACE, BWD }type;
	bool parse(const lexemes& l, size_t& pos);
};

struct production {
//	bool start = false;
//	raw_term t;
	std::vector<elem> p;
	bool parse(const lexemes& l, size_t& pos);
	bool operator<(const production& t) const { return p < t.p; }
};

class raw_rule {
	std::vector<raw_term> h, b;
public:
	raw_term& head(size_t n) { return h[n]; }
	raw_term& body(size_t n) { return b[n]; }
	const raw_term& head(size_t n) const { return h[n]; }
	const raw_term& body(size_t n) const { return b[n]; }
	const std::vector<raw_term>& heads() const { return h; }
	const std::vector<raw_term>& bodies() const { return b; }
	void add_head(const raw_term& t) { h.push_back(t); }
	void add_body(const raw_term& t) { b.push_back(t); }
	size_t nheads() const { return h.size(); }
	size_t nbodies() const { return b.size(); }

	enum etype { NONE, GOAL, TREE };
	etype type = NONE;
	bool parse(const lexemes& l, size_t& pos);
	void clear() { h.clear(), b.clear(), type = NONE; }
	raw_rule(){}
	raw_rule(const raw_term& t) : h({t}) {}
	raw_rule(const raw_term& h, const raw_term& b) : h({h}), b({b}) {}
	static raw_rule getdel(const raw_term& t) {
		raw_rule r(t, t);
		return r.head(0).neg = true, r;
	}
};

struct raw_prog {
	std::vector<directive> d;
	std::vector<production> g;
	std::vector<raw_rule> r;
//	int_t delrel = -1;
	bool parse(const lexemes& l, size_t& pos);
};

struct raw_progs {
	std::vector<raw_prog> p;
	raw_progs(FILE*);
	raw_progs(const std::wstring& s);
};

void parse_error(std::wstring e, lexeme l);
void parse_error(std::wstring e, cws s);
void parse_error(std::wstring e, std::wstring s);
void parse_error(std::wstring e, cws s, size_t len);
std::wostream& operator<<(std::wostream& os, const directive& d);
std::wostream& operator<<(std::wostream& os, const elem& e);
std::wostream& operator<<(std::wostream& os, const raw_term& t);
std::wostream& operator<<(std::wostream& os, const raw_rule& r);
std::wostream& operator<<(std::wostream& os, const raw_prog& p);
std::wostream& operator<<(std::wostream& os, const lexeme& l);
std::wostream& operator<<(std::wostream& os, const production& p);
lexeme lex(pcws s);
lexemes prog_lex(cws s);
std::wstring file_read(std::wstring fname);
std::wstring file_read_text(FILE *f);
std::wstring file_read_text(std::wstring fname);
off_t fsize(cws s, size_t len);
bool operator==(const lexeme& l, cws s);
bool operator<(const raw_term& x, const raw_term& y);
bool operator<(const raw_rule& x, const raw_rule& y);
void parser_test();
#define lexeme2str(l) wstring((l)[0], (l)[1]-(l)[0])
