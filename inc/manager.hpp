#ifndef MANAGER
#define MANAGER

#include <semaphore>
#include <tuple>
#include <unordered_map>

namespace localSocket {
template <typename Index, typename Resource>
class TempResource;

template <typename Index, typename Resource>
class Manager
{
public:
    using Member = std::tuple<int, bool, Resource>;

    Manager() : semaphore(1) {}

    void add(Index index, Resource&& resource) {
        semaphore.acquire();
        roster.emplace(index, Member{0, false, std::move(resource)});
        semaphore.release();
    }

    void remove(Index index) {
        semaphore.acquire();
        if (roster.find(index) == roster.end()) {
            semaphore.release();
            return;
        }
        auto&& [cnt, isExpired, resource] = roster.at(index);
        if (cnt <= 0) {
            roster.erase(index);
        }
        else {
            isExpired = true;
        }
        semaphore.release();
    }

    TempResource<Index, Resource> lend(Index index) {
        Resource* resource = takeOut(index);
        return {index, resource, this};
    }

private:
    std::unordered_map<Index, Member> roster;
    std::binary_semaphore semaphore;

    friend TempResource<Index, Resource>;

    Resource* takeOut(Index index) {
        semaphore.acquire();
        if (roster.find(index) == roster.end()) {
            semaphore.release();
            return nullptr;
        }
        auto&& [cnt, isExpired, resource] = roster.at(index);
        if (isExpired == true) {
            semaphore.release();
            return nullptr;
        }
        cnt++;
        Resource* ans = &resource;
        semaphore.release();
        return ans;
    }

    void takeBack(Index index) {
        semaphore.acquire();
        if (roster.find(index) == roster.end()) {
            semaphore.release();
            return;
        }
        auto&& [cnt, isExpired, resource] = roster.at(index);
        if (isExpired == true) {
            roster.erase(index);
        }
        else {
            cnt--;
        }
        semaphore.release();
    }
};

template <typename Index, typename Resource>
class TempResource
{
public:
    TempResource(Index index, Resource* resource, Manager<Index, Resource>* manager)
        : index(index), resource(resource), manager(manager) {}
    ~TempResource() {
        if (isExist())
            manager->takeBack(index);
    };

    bool isExist() { return resource != nullptr; }
    Resource& operator*() { return *resource; }
    Resource* operator->() { return resource; }

private:
    Index index;
    Resource* resource;
    Manager<Index, Resource>* manager;
};

}  // namespace server

#endif  // !MANAGER
