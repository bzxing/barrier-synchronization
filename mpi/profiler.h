#ifndef INC_PROFILER_H
#define INC_PROFILER_H

#include <iostream>
#include <sstream>
#include <chrono>
#include <string>
#include <iomanip>

namespace ProfilerDetails
{
	inline auto now()
	{
		auto val = (std::chrono::steady_clock::now().time_since_epoch());
		return val;
	}
};

class Profiler
{

public:

	Profiler(std::string name) :
		m_start(ProfilerDetails::now()),
		m_name(std::move(name))
	{
		std::cout << "Profiler: \"" + m_name + "\" started!\n";
	}

	~Profiler()
	{
		long double val = std::chrono::duration_cast<std::chrono::nanoseconds>(ProfilerDetails::now() - m_start).count();

		enum UNIT : unsigned                  { NS,   US,   MS,   S };
		const char * unit_names[] =           {"ns", "us", "ms", "s"};

		UNIT current_unit = NS;
		while (val >= 1000.0 && current_unit != S)
		{
			val /= 1000.0;
			current_unit = (UNIT)((unsigned)current_unit + 1);
		}

		std::ostringstream oss;
		oss << std::setprecision(3) << val;
		std::cout << "Profiler: \"" + m_name + "\" finished in " + oss.str() + unit_names[(unsigned)current_unit] + "\n";
	}

private:
	decltype(ProfilerDetails::now()) m_start;
	std::string m_name;
};

#endif
