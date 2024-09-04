## 框架介绍
总的来讲和单reactor+线程池比较像，只是把连接交给单独的reactor处理，所有的io和业务交给线程池处理。
具体而言，所有的事件由两个reactor响应，分为mainReactor和subReactor，mainReactor响应连接事件，subReactor响应正常的io事件以及断开连接事件。io以及业务处理在线程池执行，如果线程池的任务队列已满，由subReactor所在的线程处理。

从网上教程来看，基本上主从reactor模式有多个subReactor。
每个subReactor绑定一个cpu核心，并在自身线程处理io以及业务，但如果某个业务非常耗时，该subReactor负责的其他业务将被阻塞，即使其他cpu核心处于空闲状态。
这些教程提到了许多调度算法，但归根结底是寻找空闲的cpu，不如交给操作系统去干这活，因此将所有的io事件和业务一起交给线程池处理。

代价分为两部分，一部分是：由于需要唤醒线程，会牺牲一点点性能。另一部分是：如果线程池的任务队列已满，将由subReactor的线程处理io以及业务，如果在此期间线程池的任务迅速完成，存在空闲的cpu，那么同批次的事件相当于白白被阻塞了。后者发生概率应该是比较低的，而且可以通过设置一个较大的任务队列尽量避免。

## 使用
参考测试案例

- 特化abstract.hpp的接口类，需要实现socket的recv和send功能，设置socket为非阻塞，查看socket的文件描述符和内核发送接受缓存区大小。
以及acceptor的accept功能，查看acceptor文件描述符。<strike>应该基本上是tcp socket，[sockpp](https://github.com/fpagliughi/sockpp.git)写的比较完善，就不造更多的轮子了 ^_^ </strike>
- 继承server，重载server的readInfo函数，在此处实现业务逻辑。

## 小测一下
测试环境为本地。
socket库使用[sockpp](https://github.com/fpagliughi/sockpp.git)。
服务器回传客户端内容，进行两个测试：

- 客户端发送1MB字节，等待服务器回传。
- 100个客户端同时进行1000次乒乓测试。

两个测试都能完成。后续添加Http协议，用其他的webServer基准测试框架测测性能。

## 组件介绍
### ThreadPool
一个具备必要功能的线程池，比较简单。拒绝策略是在当前线程执行拟交给线程池的任务。出于性能考虑，每次添加任务不release信号，而是一次性处理一批次。

### Epoll
添加查找功能，Linux的epoll貌似只有增删改的功能。

### Reactor
`Reactor`基于epoll实现，为了简易使用，对epoll的功能有所取舍。
可以设置`Reactor`的默认事件和触发模式，即该`Reactor`的所有文件描述符具有相同的事件和触发模式。
可以添加多个不同类别的`Reactor`处理更复杂的需求，同一个文件描述符可以被多个epoll接管。
事件分为四类：

- EPOLLIN: 内核接受缓冲区不空
- EPOLLOUT: 内核发送缓冲区不满
- EPOLLRDHUP: 对端调用shutdown(fd, SHUT_WR)发送FIN，使得本端关闭接受
- EPOLLHUP: 本端关闭发送

相应的，有四个回调函数处理，EPOLLIN和EPOLLOUT正常地io，EPOLLRDHUP可以优雅地关闭tcp socket，EPOLLHUP回收资源。还没实现优雅关闭socket。

### Channel
意图封装socket读写，实现每个socket最多同时占用两个线程执行读写。有两个方法：`tryReadyRecv`和`tryReadySend`，如果执行返回false则说明已有一个线程将接管或正在执行该socket的读写。
对于socket的写入提供两个方法：`trySend`和`trySendRest`，如果某次`trySend`的数据过多，由于内核缓冲区有限，将返回false，并且就剩余数据存放至另一个缓冲区中。subReactor响应可写入事件，调用`trySendRest`，如果已经将剩余数据全部写入内核缓冲区，则返回true。
应当注意，不能无限制地调用`trySend`。如果某次发送的数据过大，仍应分包发送。可以根据`trySend`和`trySendRest`的返回值维护某次发送的阶段状态(已完成、等待完成)，并且触发相应的事件，大致实现用户层的异步发送。

### Valve
`Valve`本义是“阀门”，取“控制流动”的词义，即控制用户层缓冲区和内核层缓冲区之间的数据流。`Channel`有两个`Valve`，分别管理`Channel`的读缓冲区和写缓冲区。`Valve`本质是一个状态机，包括`open, shut, ready, busy, idle`五个状态。

- `open` : `Valve`使能
- `shut` : `Valve`失能
- `ready` : `Valve`开始数据传输
- `busy` : `Valve`执行数据传输
- `idle` : `Valve`完成数据传输

包括`tryTurnOnWithAgain,tryStartUpWithAgain,tryEnable(predicate),tryDisable(predicate)`四个方法。

- `tryTurnOnWithAgain` : 尝试分发IO任务
- `tryStartUpWithAgain` : 尝试开始数据传输
- `tryEnable` : 尝试开启分发IO任务，当内核写缓冲区写满执行该函数
- `tryDisable` : 尝试关闭分发IO任务，当用户写缓冲区读空执行该函数

函数名应该极具迷惑性，后续再改名，命名是件难事。但总的来讲，`Valve`完全是为`Channel`服务的。
Linux的epoll并不完美，响应EPOLLIN会顺带响应EPOLLOUT，即使处于EPOLLET。
为了过滤预期之外的EPOLLOUT事件，配合使用`tryTurnOnWithAgain`和`tryStartUpWithAgain`，可以实现处理EPOLLOUT最多同时占用一个线程。
也适用EPOLLIN，EPOLLIN简单的多。
配合使用`tryEnable`和`tryDisable`的目的是实现完整地发送某次数据，如果数据过大，将存放至额外的缓冲区，再经EPOLLOUT事件发送。

### Keeper
`Keeper<Key,Value>`相当于`std::unordered_map<Key,std::shared_ptr<Value>>`和`std::weak_ptr<Value>`的结合体 <strike>使用方法比weak_ptr优雅</strike> 。考虑多线程场景，资源可能在使用前已被销毁或使用过程中被销毁，执行任务的线程需要尝试暂时拥有该资源的所有权。`Keeper`作为管理资源的中介，向线程“出租”资源，并需要线程“归还”资源，当资源已被“归还”才能释放销毁。特别的，`Keeper`本身也是资源，因此仍需考虑其自身与线程的生命周期问题。
