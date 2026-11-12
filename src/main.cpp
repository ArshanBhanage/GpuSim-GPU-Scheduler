#include "scheduler.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
int main(int argc, char **argv) {
  try {
    if (argc != 5)
      throw std::invalid_argument("usage: gpusim jobs.csv GPU_COUNT MEMORY_GIB priority|backfill");
    std::string policy = argv[4];
    if (policy != "priority" && policy != "backfill")
      throw std::invalid_argument("unknown policy");
    std::ifstream f(argv[1]);
    if (!f)
      throw std::invalid_argument("cannot open workload");
    std::string line;
    std::getline(f, line);
    if (line != "id,arrival,duration,priority,gpus,share,memory")
      throw std::invalid_argument("invalid CSV header");
    std::vector<gpusim::Job> jobs;
    while (std::getline(f, line)) {
      if (line.empty())
        continue;
      std::replace(line.begin(), line.end(), ',', ' ');
      std::istringstream s(line);
      gpusim::Job j;
      std::string extra;
      if (!(s >> j.id >> j.arrival >> j.duration >> j.priority >> j.gpus >> j.share >> j.memory) ||
          (s >> extra))
        throw std::invalid_argument("invalid CSV row");
      jobs.push_back(j);
    }
    auto runs = gpusim::simulate(jobs, std::stoi(argv[2]), std::stod(argv[3]),
                                 policy == "priority" ? gpusim::Policy::Priority
                                                      : gpusim::Policy::Backfill);
    std::cout << "id,start,finish,wait,devices\n";
    for (auto &r : runs) {
      std::cout << r.job.id << ',' << r.start << ',' << r.finish << ',' << r.start - r.job.arrival
                << ',';
      for (size_t i = 0; i < r.devices.size(); ++i)
        std::cout << (i ? ";" : "") << r.devices[i];
      std::cout << '\n';
    }
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
