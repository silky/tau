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
#include <cstdint>
#include <vector>
#include <iostream>
#include <set>
#include "bdd.h"

void tml_init();

template<> struct std::hash<std::pair<size_t, bools>> {
	size_t operator()(const std::pair<size_t, bools>& m) const; };

class lp { // [pfp] logic program
	friend struct rule;
	struct step {
		std::unordered_map<std::pair<size_t, bools>, size_t> pos, neg;
	} p;
	std::vector<struct rule*> rules;
public:
	const size_t bits, ar, dsz;
	size_t db = F; // db's bdd root
	lp(size_t maxbits,size_t ar,size_t dsz):bits(maxbits),ar(ar),dsz(dsz) {}
	void rule_add(const matrix& t, std::set<matrix>* proof = 0);
	void fwd(size_t &add, size_t &del, std::set<size_t>* pf);
	matrix getbdd(size_t t) const;
	matrix getbdd_one(size_t t) const;
	matrix getdb() const;
	matrix getbdd(size_t t, size_t b, size_t a) const;
	matrix getbdd_one(size_t t, size_t b, size_t a) const;
	size_t get_sym_bdd(size_t sym, size_t pos) const;
	size_t proof_arity() const;
	size_t get_varbdd() const;
	~lp();
};

std::wostream& out(std::wostream& os, const node& n); // print bdd in ?: syntax
std::wostream& out(std::wostream& os, size_t n);
extern int_t pad;
