#include "timer.hpp"

#include <iomanip>

// for timing routine
#include <omp.h>

#include <iostream>
#include <iomanip>
#include <math.h>

// static members of a class must be defined
// somewhere in an object file, otherwise you
// will get linker errors (undefined reference)
std::map<std::string, int> Timer::counts_;
std::map<std::string, double> Timer::times_;
std::map<std::string, double> Timer::squared_times_;
std::map<std::string, double> Timer::max_time_;
std::map<std::string, double> Timer::min_time_;

  Timer::Timer(std::string label)
  : label_(label)
  {
    t_start_ = omp_get_wtime();
  }


  Timer::~Timer()
  {
    double t_end = omp_get_wtime();
    double t_diff = t_end - t_start_;
    times_[label_] += t_diff;
    squared_times_[label_] += (t_diff)*(t_diff);
    counts_[label_]++;
    if(max_time_[label_]==0.0){ //If this is the first time calling the max_time_ map for this label:
      max_time_[label_] += t_diff;
      min_time_[label_] += t_diff;
    } else { //If the max_time_ map has already been called for this label:
      max_time_[label_] = (t_diff>max_time_[label_])?t_diff:max_time_[label_];
      min_time_[label_] = (t_diff<min_time_[label_])?t_diff:min_time_[label_];
    }
  }

void Timer::summarize(std::ostream& os)
{
	os << std::endl;
	std::cout << std::setw(25) << std::left << "label" << "\t,"
	<< std::setw(10) << "mean time (ms)" << "\t,"
	<< std::setw(5) << "calls"			 << "\t,"
	<< std::setw(10) << "total time (s)" << "\t,"
	<< std::setw(10) << "max time (ms)"  << "\t,"
	<< std::setw(10) << "min time (ms)"  << "\t,"
	<< std::setw(10) << "std time (ms)"  << std::endl;

	for (auto [label, time]: times_)
	{
		int count = counts_[label];
		double sigma = sqrt(squared_times_[label]/count-time*time/double(count*count));
		std::cout << std::fixed << std::setprecision(4) << std::setw(25) 
		<< std::left 	 << label.substr(0, 35) 		<< "\t,"
		<< std::setw(10) << time/double(count)*1000.0 	<< "\t,"
		<< std::setw(5) << count 						<< "\t,"
		<< std::setw(10) << time 						<< "\t,"
		<< std::setw(10) << max_time_[label]*1000.0 	<< "\t,"
		<< std::setw(10) << min_time_[label]*1000.0 	<< "\t,"
		<< std::setw(10) << sigma*1000.0 				<< std::endl;
	}
	os << std::endl;
}

