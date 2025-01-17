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
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <cstring>
#include <iostream>
#include <random>
#include <sstream>
#include <climits>
#include <stdexcept>
#include <cassert>
#include "rule.h"
#include "term.h"
#ifdef DEBUG
#include "driver.h"
#else
#include "lp.h"
#endif
using namespace std;

void tml_init() { bdd::init(); }
DBG(wostream& printbdd(wostream& os, size_t t);)

void lp::add_fact(spbdd f, const prefix& p) {
	spbdd t = db[p];
	db[p] = t ? t||f : f;
}

bool lp::add_not_fact(spbdd f, const prefix& p) {
	spbdd x = db[p];
	return (db[p] = (x ? x : T) % f), (!x || x == F || db[p] != F);
}

bool lp::add_fact(const term& x) {
//	if (x.neg) return add_not_fact(fact(x, bits, dsz), x.rel, x.arity);
	return add_fact(fact(x, rng), x.pref()), true;
}

bool lp::add_facts(const matrix& x) {
	for (auto y : x) if (!add_fact(y)) return false; // FIXME
	return true;
}

spbdd prefix_zeros(spbdd x, size_t v, size_t k) {
	if (v) return bdd::add(k-v+1, F, prefix_zeros(x, v-1, k));
	if (x->leaf()) return x;
	return bdd::add(x->v()+k, prefix_zeros(x->h(),0,k),
			prefix_zeros(x->l(),0,k));
}

spbdd prefix_zeros(spbdd x, size_t k) { return prefix_zeros(x, k, k); }

db_t rebit(size_t pbits, size_t bits, db_t db) {
	if (pbits == bits) return db;
	assert(pbits < bits);
	for (auto x : db)
		x.second = prefix_zeros(x.second, (bits-pbits)*x.first.len());
	return db;
}

void bdd_or(diff_t& x, const term& t, range& rng) {
	auto it = x.find(t.pref());
	if (it == x.end()) it = x.emplace(t.pref(), F).first;
	it->second = it->second || fact(t, rng);
}

lp::lp(prog_data pd, range rng, lp *prev) : pd(pd), rng(rng) {
	if (prev) {
		db = rebit(prev->rng.bits, rng.bits, move(prev->db));
		delete prev;
	}
	//wcout<<r<<endl;
	for (const auto& m : pd.r) {
		for (const term& t : m.first)
			if (db.find(t.pref()) == db.end())
				db[t.pref()] = F;
		for (const term& t : m.second)
			if (db.find(t.pref()) == db.end())
				db[t.pref()] = F;
	}
	for (const auto& m : pd.r)
 		if (m.second.empty()) {
			if (!add_facts(m.first)) {}
				// FIXME
//				(wcout << L"contradictory fact: "<<m[0]<<endl),
//			exit(0);
		} else {
			vector<db_t::iterator> dbs;
			for (size_t n = 0; n < m.second.size(); ++n)
				dbs.push_back(db.find({m.second[n].rel(),
					m.second[n].arity()}));
			rules.emplace_back(
				new rule(m.first, m.second, dbs, rng));
		}
//	DBG(printdb(wcout<<L"pos:"<<endl, this);)
//	DBG(printndb(wcout<<L"neg:"<<endl, this)<<endl;)
	for (auto x : pd.strtrees)
		strtrees[x.first][x.second.pref()] = fact(x.second, rng);
	if (!pd.bwd) for (const term& t : pd.goals) bdd_or(gbdd, t, rng);
	for (const term& t : pd.tgoals)
		trees.emplace(diff_t::key_type{t.rel(),t.arity()}, fact(t,rng));
}

void lp::fwd(diff_t &add, diff_t &del) {
	//DBG(printdb(wcout, this));
	map<prefix, vector<spbdd>> a, d;
	for (rule* r : rules) {
		const bdds x = r->fwd();
		if (!x.empty())
			for (size_t n = 0; n != x.size(); ++n)
				(r->neg[n]? d : a)[r->hpref[n]].push_back(x[n]);
	}
	diff_t::iterator it;
	for (auto x : a) {
		DBG(for(auto y:x.second)assert_nvars(y,rng.bits*x.first.len());)
		it = add.find(x.first);
		if (it == add.end()) add[x.first] = bdd_or_many(x.second);
		else	x.second.push_back(it->second),
			it->second = bdd_or_many(x.second);
	}
	for (auto x : d) {
		DBG(for(auto y:x.second)assert_nvars(y,rng.bits*x.first.len());)
		it = del.find(x.first);
		if (it == del.end()) del[x.first] = bdd_or_many(x.second);
		else	x.second.push_back(it->second),
			it->second = bdd_or_many(x.second);
	}
	if (onmemo(0)>1e+17)
		(wcerr<<onmemo(0)), memos_clear(), bdd_and_eq::memo_clear(),
		range::memo_clear(), (wcerr << " gc " << onmemo(0) << endl),
		onmemo(-onmemo(0));
	//DBG(printdiff(wcout<<"add:"<<endl,add,rng.bits););
	//DBG(printdiff(wcout<<"del:"<<endl,del,rng.bits););
	//DBG(printdb(wcout<<"after step: "<<endl, this)<<endl;)
}

struct diffcmp {
	bool operator()(const diff_t& x, const diff_t& y) const {
		if (x.size() != y.size()) return x.size() < y.size();
		auto xt = x.begin(), yt = y.begin();
		while (xt != x.end())
			if (xt->first != yt->first) return xt->first<yt->first;
			else if (xt->second == yt->second) ++xt, ++yt;
			else return xt->second < yt->second;
		return false;
	}
};

diff_t copy(const db_t& x) {
	return x;
	diff_t r;
	for (auto y : x) r[y.first] = y.second;
	return r;
}

bool bdd_and_not(const diff_t& x, const diff_t& y, diff_t& r) {
	for (auto t : x)
		if (has(y, t.first) && t.second && F == (r[t.first] 
				= t.second % y.at(t.first)))
			return false;
	return true;
}

db_t& bdd_and_not(db_t& x, const diff_t& y) {
	for (auto t : x)
		if (has(y, t.first))
			t.second = t.second % y.at(t.first);
	return x;
}

void bdd_or(db_t& x, const diff_t& y) {
	for (auto t : x)
		if (has(y, t.first))
			t.second = t.second || y.at(t.first);
}

/*bool operator==(const db_t& x, const diff_t& y) {
	return x == y;
	if (x.size() != y.size()) return false;
	auto xt = x.begin();
	auto yt = y.begin();
	while (xt != x.end())
		if (xt->first==yt->first && xt->second==yt->second) ++xt, ++yt;
		else return false;
	return true;
}*/

bool lp::pfp() {
	diff_t d, add, del, t;
	set<size_t> pf;
	size_t step = 0;
//	wcout << V.size() << endl;
//	DBG(printdb(wcout<<"before prog: "<<endl, this)<<endl;)
	for (set<diff_t, diffcmp> s;;) {
		//DBG()
		wcout << "step: " << step++ << endl;
//		d = copy(db),
		s.emplace(d = copy(db)),
		fwd(add, del);
		if (!bdd_and_not(add, del, t))
			return false; // detect contradiction
		for (auto x : add) add_fact(x.second, x.first);
		for (auto x : del) add_not_fact(x.second, x.first);
		if (db == d) break;
		if (has(s, copy(db))) return false;
	}
//	DBG(drv->printdiff(wcout<<"trees:"<<endl, trees, rng.bits);)
	get_trees();
//	DBG(static int nprog = 0;)
//	DBG(printdb(wcout<<"after prog: "<<nprog++<<endl, this)<<endl;)
	auto it = db.end();
	for (auto x : gbdd)
		if ((it = db.find(x.first)) != db.end()) {
//			delete it->second;
			db.erase(it);
		}
	return true;
}

lp::~lp() { for (rule* r : rules) delete r; }

onexit _onexit;

onexit::~onexit() {
//	memos_clear(), bdd_and_eq::memo_clear(), range::memo_clear(),
	bdd::onexit = true;
}

#ifdef DEBUG
void assert_nvars(spbdd x, size_t vars) {
	size_t nv = bdd_nvars(x);
	assert(nv <= vars);
}
#endif
