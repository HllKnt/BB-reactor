#pragma once

#include <semaphore>
#include <tuple>
#include <unordered_map>

namespace frame {
template <typename Key, typename Value>
class Temp;

template <typename Key, typename Value>
class Keeper
{
public:
    Keeper();

    void obtain(Key key, Value&& value);
    Temp<Key, Value> lend(Key key);
    void discard(Key key);

private:
    friend Temp<Key, Value>;
    using Member = std::tuple<int, bool, Value>;

    std::binary_semaphore lock;
    std::unordered_map<Key, Member> repo;

    Value* takeOut(Key key);
    void takeBack(Key key);
};

template <typename Key, typename Value>
class Temp
{
public:
    Temp(Key key, Value* value, Keeper<Key, Value>* keeper);
    Temp(const Temp&) = delete;
    ~Temp();

    bool exist();
    Value& peerValue();

private:
    Key key;
    Value* value;
    Keeper<Key, Value>* keeper;
};

}  // namespace frame

namespace frame {
template <typename Key, typename Value>
Keeper<Key, Value>::Keeper() : lock{1} {}

template <typename Key, typename Value>
void Keeper<Key, Value>::obtain(Key key, Value&& value) {
    lock.acquire();
    repo.emplace(key, Member{0, false, std::move(value)});
    lock.release();
}

template <typename Key, typename Value>
Temp<Key, Value> Keeper<Key, Value>::lend(Key key) {
    Value* value = takeOut(key);
    return {key, value, this};
}

template <typename Key, typename Value>
void Keeper<Key, Value>::discard(Key key) {
    lock.acquire();
    if (not repo.contains(key)) {
        lock.release();
        return;
    }
    auto& [userCount, expired, value] = repo.at(key);
    if (userCount <= 0) {
        repo.erase(key);
    }
    else {
        expired = true;
    }
    lock.release();
}

template <typename Key, typename Value>
Value* Keeper<Key, Value>::takeOut(Key key) {
    lock.acquire();
    if (not repo.contains(key)) {
        lock.release();
        return nullptr;
    }
    auto& [userCount, expired, value] = repo.at(key);
    if (expired) {
        lock.release();
        return nullptr;
    }
    userCount++;
    Value* ans = &value;
    lock.release();
    return ans;
}

template <typename Key, typename Value>
void Keeper<Key, Value>::takeBack(Key key) {
    lock.acquire();
    if (not repo.contains(key)) {
        lock.release();
        return;
    }
    auto& [userCount, expired, value] = repo.at(key);
    if (expired) {
        repo.erase(key);
    }
    else {
        userCount--;
    }
    lock.release();
}

template <typename Key, typename Value>
Temp<Key, Value>::Temp(Key key, Value* value, Keeper<Key, Value>* keeper)
    : key{key}, value{value}, keeper{keeper} {}

template <typename Key, typename Value>
Temp<Key, Value>::~Temp() {
    if (not exist()) {
        return;
    }
    keeper->takeBack(key);
};

template <typename Key, typename Value>
bool Temp<Key, Value>::exist() {
    return value != nullptr;
}

template <typename Key, typename Value>
Value& Temp<Key, Value>::peerValue() {
    return *value;
}

}  // namespace frame
