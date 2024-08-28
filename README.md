## 框架介绍
BB-reactor仅关注reactor模式的逻辑，未实现socket读写功能以及TCP连接相关的acceptor，构建具体的server需要特化connection和acceptor的接口类模板。

mainReactor负责处理TCP连接以及断开连接，subReactor负责监听socket的IO事件，***有且仅有一个subReactor***，IO任务分发给线程池，由另外的线程处理。

## 组件介绍
<strike>threadPool和epoll应该不用过多介绍</strike>
### Reactor
`Reactor`基于epoll实现，取“反应”、“呈报事件”的词义，是一个具体的类***而非代指reactor模式***。server可以在`Reactor`注册或注销其“感兴趣”的IO事件，在Linux下，此类的IO事件可解释为内核层缓冲区可读或可写，当此类事件“可行”时，`Reactor`将调用***server设置的回调函数***处理同批次的IO事件。
### Valve
`Valve`本义是“阀门”，取“控制流动”的词义，即控制用户层缓冲区和内核层缓冲区之间的数据流，建立连接的socket将分配两个`Valve`，分别管理读缓冲区和写缓冲区。`Valve`本质是一个状态机，包括`open, shut, ready, busy, idle`五个状态。

- `open` : `Valve`使能
- `shut` : `Valve`失能
- `ready` : `Valve`开始数据传输
- `busy` : `Valve`执行数据传输
- `idle` : `Valve`完成数据传输

包括`tryTurnOn,tryStartUp,tryEnable(predicate),tryDisable(predicate)`四个方法。

- `tryTurnOn` : 尝试分发IO任务
- `tryStartUp` : 尝试开始数据传输
- `tryEnable` : 尝试开启分发IO任务，当内核写缓冲区写满执行该函数
- `tryDisable` : 尝试关闭分发IO任务，当用户写缓冲区读空执行该函数

### Keeper
`Keeper<Key,Value>`相当于`std::unordered_map<Key,std::shared_ptr<Value>>`和`std::weak_ptr<Value>`的结合体 <strike>使用方法比weak_ptr优雅</strike> 。考虑多线程场景，资源可能在使用前已被销毁或使用过程中被销毁，执行任务的线程需要尝试暂时拥有该资源。`Keeper`作为管理资源的中介，向线程“出租”资源，并需要线程“归还”资源，当资源已被“归还”才能释放销毁。特别的，`Keeper`本身也是资源，因此仍需考虑其自身与线程的生命周期问题。

### Channel
封装多线程的Socket读写操作，可能比较像Java的SocketChannel <strike>不太懂Java</strike>。

## 小测一下
socket库使用[sockpp](https://github.com/fpagliughi/sockpp.git)。
测试客户端发送1MB信息，等待服务端回传。

乒乓测试暴露一些问题

- linux的epoll边缘触发似乎会丢失读取事件
- 未考虑cpu处理任务的吞吐量，线程池的存放任务的队列没做添加限制，存在bug
