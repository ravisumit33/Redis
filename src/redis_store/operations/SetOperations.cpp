#include "redis_store/RedisStore.hpp"
#include "redis_store/values/SetValue.hpp"
#include <string>

std::size_t RedisStore::addMemberToSet(const std::string &key, double score,
                                       const std::string &member) {
  auto store = writeStore();
  auto itr = store->find(key);

  bool inserted = true;
  if (itr == store->end()) {
    auto val = SetValue();
    val.addMember(score, member);
    store->insert_or_assign(key, val);
  } else {
    auto *set = std::get_if<SetValue>(&itr->second);
    if (set == nullptr) {
      throw std::runtime_error("Key doesn't correspond to SET value type");
    }
    inserted = set->addMember(score, member);
  }
  notifyBlockingClients(key);
  return static_cast<std::size_t>(inserted);
}

unsigned int RedisStore::getSetMemberRank(const std::string &key,
                                          const std::string &member) const {
  auto store = readStore();
  const auto &itr = store->at(key);
  const auto *set = std::get_if<SetValue>(&itr);
  if (set == nullptr) {
    throw std::runtime_error("Key doesn't correspond to SET value type");
  }
  return set->getRank(member);
}

std::size_t RedisStore::removeMemberFromSet(const std::string &key,
                                            const std::string &member) {
  auto store = writeStore();
  auto &itr = store->at(key);
  auto *set = std::get_if<SetValue>(&itr);
  if (set == nullptr) {
    throw std::runtime_error("Key doesn't correspond to SET value type");
  }
  return set->erase(member);
}
