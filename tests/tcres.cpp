#include <iostream>
#include <cstdlib>
using namespace std;
int main(int argc, char** argv) {
	size_t n = atol(argv[1]);
	for (size_t k = 1; k <= n; ++k)
		for (size_t i = 1; i <= n; ++i)
			cout << "e " << k << ' ' << i << ' ' << endl;
	return 0;
}