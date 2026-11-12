#pragma once
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
namespace gpusim {
struct Job {
  int id;
  double arrival, duration;
  int priority, gpus;
  double share, memory;
};
struct Device {
  double compute = 1, memory = 16;
};
struct Run {
  Job job;
  double start, finish;
  std::vector<int> devices;
};
enum class Policy { Priority, Backfill };
// share and memory are requested PER GPU; devices in a gang are distinct.
inline std::vector<int> place(const Job &j, const std::vector<Device> &free) {
  std::vector<int> ids;
  for (size_t i = 0; i < free.size() && ids.size() < static_cast<size_t>(j.gpus); ++i)
    if (free[i].compute + 1e-9 >= j.share && free[i].memory + 1e-9 >= j.memory)
      ids.push_back(static_cast<int>(i));
  if (ids.size() != static_cast<size_t>(j.gpus))
    ids.clear();
  return ids;
}
inline std::vector<Run> simulate(std::vector<Job> jobs, int count, double memory, Policy policy) {
  if (count <= 0 || !std::isfinite(memory) || memory <= 0)
    throw std::invalid_argument("invalid cluster");
  std::vector<Device> capacity(count, Device{1, memory});
  std::set<int> ids;
  for (auto j : jobs) {
    if (!ids.insert(j.id).second || !std::isfinite(j.arrival) || j.arrival < 0 ||
        !std::isfinite(j.duration) || j.duration <= 0 || !std::isfinite(j.share) || j.share <= 0 ||
        !std::isfinite(j.memory) || j.memory <= 0 || j.gpus <= 0 || place(j, capacity).empty())
      throw std::invalid_argument("invalid or unschedulable job");
  }
  std::stable_sort(jobs.begin(), jobs.end(), [](auto a, auto b) { return a.arrival < b.arrival; });
  std::vector<Job> waiting;
  std::vector<Run> active, done;
  size_t next = 0;
  double now = 0;
  auto available = [&](double t, const std::vector<Run> &running) {
    auto free = capacity;
    for (auto &r : running)
      if (r.finish > t)
        for (int d : r.devices) {
          free[d].compute -= r.job.share;
          free[d].memory -= r.job.memory;
        }
    return free;
  };
  auto reservation = [&](const Job &j, const std::vector<Run> &running) {
    if (!place(j, available(now, running)).empty())
      return now;
    std::vector<double> times;
    for (auto &r : running)
      times.push_back(r.finish);
    std::sort(times.begin(), times.end());
    for (double t : times)
      if (!place(j, available(t, running)).empty())
        return t;
    return std::numeric_limits<double>::infinity();
  };
  while (next < jobs.size() || !waiting.empty() || !active.empty()) {
    active.erase(
        std::remove_if(active.begin(), active.end(), [&](const Run &r) { return r.finish <= now; }),
        active.end());
    while (next < jobs.size() && jobs[next].arrival <= now)
      waiting.push_back(jobs[next++]);
    std::stable_sort(waiting.begin(), waiting.end(), [](auto a, auto b) {
      if (a.priority != b.priority)
        return a.priority > b.priority;
      if (a.arrival != b.arrival)
        return a.arrival < b.arrival;
      return a.id < b.id;
    });
    for (size_t i = 0; i < waiting.size();) {
      auto allocated = place(waiting[i], available(now, active));
      bool allowed = !allocated.empty();
      if (i > 0 && allowed) {
        auto trial = active;
        trial.push_back({waiting[i], now, now + waiting[i].duration, allocated});
        allowed = reservation(waiting.front(), trial) <= reservation(waiting.front(), active);
      }
      if (allowed) {
        Run r{waiting[i], now, now + waiting[i].duration, allocated};
        active.push_back(r);
        done.push_back(r);
        waiting.erase(waiting.begin() + i);
      } else {
        if (policy == Policy::Priority)
          break;
        ++i;
      }
    }
    double event =
        next < jobs.size() ? jobs[next].arrival : std::numeric_limits<double>::infinity();
    for (auto &r : active)
      event = std::min(event, r.finish);
    if (!std::isfinite(event)) {
      if (!waiting.empty())
        throw std::logic_error("scheduler stalled");
      break;
    }
    if (event <= now)
      throw std::logic_error("non-monotonic event");
    now = event;
  }
  return done;
}
} // namespace gpusim
