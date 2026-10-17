#pragma once

// A check that counts failures without stopping, for the tests' plain main().

#include <cstdio>

namespace mdseqTest
{
	inline int g_failures = 0;

	inline void check(const bool _ok, const char* _what, const int _line)
	{
		if(_ok)
			return;
		std::printf("FAIL line %d: %s\n", _line, _what);
		++g_failures;
	}

	inline int finish(const char* _name)
	{
		if(g_failures)
		{
			std::printf("%s: %d failure(s)\n", _name, g_failures);
			return 1;
		}
		std::printf("%s: OK\n", _name);
		return 0;
	}
}

#define CHECK(_cond) mdseqTest::check((_cond), #_cond, __LINE__)
