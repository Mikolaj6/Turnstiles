#include "turnstile.h"
#include <assert.h>
#include <iostream>
#include <mutex>

Manager &manager() {
  static auto ans = new Manager();
  return *ans;
}

void Mutex::lock() {
  auto which_lock_unformated = (uintptr_t)this;
  auto which_lock = reinterpret_cast<size_t>(which_lock_unformated) %
                    manager().protection_size;

  manager().protection[which_lock]->lock();
  if (tur == nullptr) {
    tur = const_cast<Turnstile *>(manager().guard);
  } else {
    if (tur == manager().guard) {
      manager().shield.lock();
      tur = manager().get_turnstile();
      manager().shield.unlock();
    }
    tur->waiting++;

    manager().protection[which_lock]->unlock();
    std::unique_lock<std::mutex> lk(tur->mut);
    tur->cv.wait(lk, [=] { return tur->is_empty; });
    manager().protection[which_lock]->lock();
    tur->is_empty = false;
    tur->waiting--;
  }
  manager().protection[which_lock]->unlock();
}

void Mutex::unlock() {
  auto which_lock_unformated = (uintptr_t)this;
  auto which_lock = reinterpret_cast<size_t>(which_lock_unformated) %
                    manager().protection_size;

  manager().protection[which_lock]->lock();

  assert(tur != nullptr);
  if (tur == manager().guard) {
    tur = nullptr;
  } else {
    if (tur->waiting > 0) {
      std::unique_lock<std::mutex> lk(tur->mut);
      tur->is_empty = true;
      tur->cv.notify_one();
    } else {
      manager().shield.lock();
      manager().give_turnstile(tur);
      manager().shield.unlock();
      tur = nullptr;
    }
  }
  manager().protection[which_lock]->unlock();
}

Manager::Manager() {
  for (unsigned int i = 0; i < protection_size; i++)
    protection.emplace_back(new std::mutex());
  for (unsigned int i = 0; i < statring_size; i++)
    turnstiles.push(new Turnstile());
}

Manager::~Manager() {
  for (unsigned int i = 0; i < protection_size; i++) delete protection[i];
  while (!turnstiles.empty()) {
    auto to_ret = turnstiles.front();
    turnstiles.pop();
    delete to_ret;
  }
}

/**
 * Returns free turnstile
 *
 * @return
 */
Turnstile *Manager::get_turnstile() {
  if (turnstiles.empty()) {
    for (unsigned int i = 0; i < all_turnstiles; i++)
      turnstiles.push(new Turnstile());
    free_in_queue = all_turnstiles;
    all_turnstiles *= 2;
  }
  auto to_ret = turnstiles.front();
  turnstiles.pop();
  free_in_queue--;
  return to_ret;
}

/**
 * Puts turnstile back into set of unused
 *
 * @param tur turnstile to return
 */
void Manager::give_turnstile(Turnstile *tur) {
  turnstiles.push(tur);
  free_in_queue++;
  if (all_turnstiles > statring_size &&
      free_in_queue > (all_turnstiles * 3) / 4) {
    all_turnstiles /= 2;
    free_in_queue -= all_turnstiles;
    for (unsigned int i = 0; i < all_turnstiles; i++) {
      auto to_ret = turnstiles.front();
      turnstiles.pop();
      delete to_ret;
    }
  }
}