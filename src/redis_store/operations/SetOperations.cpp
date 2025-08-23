#include "redis_store/RedisStore.hpp"
#include "redis_store/values/SetValue.hpp"
#include <memory>
#include <string>

std::size_t RedisStore::addMemberToSet(const std::string &key, double score,
                                       const std::string &member) {
  auto store = writeStore();
  auto it = store->find(key);

  bool inserted = true;
  if (it == store->end()) {
    auto val = std::make_unique<SetValue>();
    val->addMember(score, member);
    store->insert_or_assign(key, std::move(val));
  } else {
    auto &set = static_cast<SetValue &>(*it->second);
    inserted = set.addMember(score, member);
  }
  notifyBlockingClients(key);
  return inserted;
}

unsigned int RedisStore::getSetMemberRank(const std::string &key,
                                          const std::string &member) {
  auto store = readStore();
  auto &it = store->at(key);
  if (it->getType() != RedisStoreValue::SET) {
    throw std::runtime_error("Key doesn't correspond to SET value type");
  }
  auto &set = static_cast<SetValue &>(*it);
  return set.getRank(member);
}

std::size_t RedisStore::removeMemberFromSet(const std::string &key,
                                            const std::string &member) {
  auto store = readStore();
  auto &it = store->at(key);
  if (it->getType() != RedisStoreValue::SET) {
    throw std::runtime_error("Key doesn't correspond to SET value type");
  }
  auto &set = static_cast<SetValue &>(*it);
  return set.erase(member);
}
