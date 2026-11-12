#include "scheduler.hpp"
#include <iostream>
using namespace gpusim;
void check(bool x) {
  if (!x)
    throw std::runtime_error("test failed");
}
double start(const std::vector<Run> &r, int id) {
  for (auto x : r)
    if (x.job.id == id)
      return x.start;
  throw std::runtime_error("missing job");
}
int main() {
  std::vector<Job> j = {{1, 0, 10, 10, 1, 1, 8}, {2, 1, 5, 9, 2, 1, 8}, {3, 1, 2, 1, 1, 1, 8}};
  auto p = simulate(j, 2, 16, Policy::Priority), b = simulate(j, 2, 16, Policy::Backfill);
  check(start(p, 3) == 15);
  check(start(b, 3) == 1);
  check(start(b, 2) == 10);
  j[2].duration = 20;
  b = simulate(j, 2, 16, Policy::Backfill);
  check(start(b, 3) == 15);
  auto shared = simulate({{1, 0, 5, 0, 1, .5, 8}, {2, 0, 5, 0, 1, .5, 8}, {3, 0, 1, 0, 1, .5, 8}},
                         1, 16, Policy::Backfill);
  check(start(shared, 2) == 0);
  check(start(shared, 3) == 5);
  auto mem = simulate({{1, 0, 5, 0, 1, .2, 12}, {2, 0, 1, 0, 1, .2, 12}}, 1, 16, Policy::Backfill);
  check(start(mem, 2) == 5);
  check(simulate({}, 2, 16, Policy::Backfill).empty());
  bool rejected = false;
  try {
    simulate({{1, 0, 1, 0, 3, 1, 8}}, 2, 16, Policy::Priority);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  check(rejected);
  std::cout << "priority, reservation protection, sharing, memory, empty and rejection passed\n";
}
