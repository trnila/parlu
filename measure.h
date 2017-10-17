#pragma once

#include <chrono>
struct profiler
{
	std::string name;
	std::chrono::high_resolution_clock::time_point p;
	profiler(std::string const &n) :
			name(n), p(std::chrono::high_resolution_clock::now()) { }
	~profiler()
	{
		using dura = std::chrono::duration<double>;
		auto d = std::chrono::high_resolution_clock::now() - p;
		std::cout << name << ": "
		          << std::chrono::duration_cast<dura>(d).count()
		          << std::endl;
	}
};

#define PROFILE_BLOCK(pbn) profiler _pfinstance(pbn)