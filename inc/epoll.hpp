#include <sys/epoll.h>

#include <memory>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace frame {
class Epoll
{
public:
    using EpollEevents = std::vector<std::tuple<int, uint32_t>>;

    Epoll();
    ~Epoll();

    bool exist(int fd);
    uint32_t peerEvent(int fd);
    void add(int fd, uint32_t event);
    void mod(int fd, uint32_t event);
    void del(int fd);
    void setEdgeTrigger(int fd);
    void setLevelTrigger(int fd);
    EpollEevents wait();

private:
    int self;
    std::unordered_map<int, uint32_t> roster;
    std::unique_ptr<std::vector<epoll_event>> epollEvents;
};

}  // namespace frame
