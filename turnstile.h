#ifndef SRC_TURNSTILE_H_
#define SRC_TURNSTILE_H_

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <vector>

class Turnstile {
 public:
  std::mutex mut;
  std::condition_variable cv;
  size_t waiting = 0;
  bool is_empty = false;

  Turnstile() = default;

  Turnstile(const Turnstile &) = delete;
};

class Manager {
  const int bbb = 1;
  const size_t statring_size = 16;
  size_t all_turnstiles = statring_size;
  size_t free_in_queue = statring_size;
  std::queue<Turnstile *> turnstiles;

 public:
  const size_t protection_size = 257;
  const Turnstile *guard = reinterpret_cast<Turnstile *>(bbb);
  std::vector<std::mutex *> protection;
  std::mutex shield;

  Turnstile *get_turnstile();

  void give_turnstile(Turnstile *tur);

  Manager();

  ~Manager();
};

class Mutex {
  Turnstile *tur = nullptr;

 public:
  Mutex() = default;

  Mutex(const Mutex &) = delete;

  void lock();  // NOLINT

  void unlock();  // NOLINT
};

#endif  // SRC_TURNSTILE_H_