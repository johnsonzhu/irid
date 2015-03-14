一.Irid简介

Irid(Iridescent)是在CentOS5.3下用纯C写的一个缓存应用程序（类似memcached、redis），不依赖额外第三方库。

她有以下特点:

1)epoll + 多线程支持高并发（使用长连接性能更佳）.

2)纯内存操作，速度快.

3)支持memcache通讯协议（add get set delete replace append prepend increment decrement），客户端无需装额外扩展

4)程序内部使用LRU算法实现热点缓存功能（当容量达到上限的时候，优先将少人访问的缓存数据剔除,在内存一定的前提下，提高命中率）

5)支持动态修改配置,修改segment的容量（需执行reload指令）

5)扩展支持存储结构化数据，例如可以存储php的array（serialize）,可以单独更新某个array的某个键值

> 例子：有个数组如下

> key=user\_12345678

> value=array(

> "userId"=>12345678,

> "username"=>"用户",

> "mood"=>"天气真好啊!",

> "friendId"=>array(1,2,3,4,5,6,7,8,9),

> "other"=>array(

> "login\_num"=>10,

> )
> )

> 情景1:根据用户id获取用户名称
> > $username = $mc->get("user\_12345678@username");


> 情景2：为数组添加多一个key last\_login\_time
> > $mc->set("user\_12345678@other.last\_login\_time",time());


> 情景3：修改用户的mood
> > $mc->append("user\_12345678@mood",",好想去玩啊！！！")


> 情景4：增加登录次数
> > $mc->increment("user\_12345678@other.login\_num");


> 更多调用例子参考testcase下的php文件

6)增强了部分功能:(原生memcache客户段不支持，需要自己实现)


  1. 支持模糊搜索key（list指令）

> 2.支持对数组push、pop、shift、unshift操作

> 3.支持统计数组元素个数（count指令）

> 4.支持分页获取数据（page指令）

7)提高cli接口，方便管理,cli帮助如下：

> ./Irid Usage:

> start server  ./Irid -c irid.conf 	//启动服务器

> stop server   ./Irid -s stop			//关闭服务器


> ./Irid CLI API:

> ./Irid add host:port key value

> ./Irid append host:port key value

> ./Irid count host:port key			//统计数组中有多少个元素

> ./Irid decrement host:port key num

> ./Irid delete host:port key

> ./Irid flush\_all host:port			//清除所有数据

> ./Irid get host:port key1 {key2}

> ./Irid increment host:port key num

> ./Irid list host:port {-fm str} {-mm str} {-em str} //模糊查询key,支持3种匹配模式（前缀匹配、中缀匹配、后缀匹配）

> ./Irid page host:port key offset limit 	//分页显示数据

> ./Irid prepend host:port key value

> ./Irid pop host:port key

> ./Irid push host:port key value

> ./Irid reload host:port filename		//重新加载配置

> ./Irid replace host:port key value

> ./Irid set host:port key value

> ./Irid shift host:port key

> ./Irid stats host:port				//查看当前服务器的运行的状态

> ./Irid unshift host:port key value

> ./Irid version host:port

当前v0.9版全部代码大概5500行，95%代码都是本人原创编写的（除了暴雪的hash\_code和自己改写过的系统rbtree外）,
程序经过内存工具valgrind和自己写的内存小工具测试过，是没有内存泄露的.

程序经过应用层单元测试(php调用)，暂时还没有发现很大的问题.
PS:没有bug的就不叫程序，希望大家帮忙找找bug,毕竟这个程序写的很匆忙,加上本人水平很菜,希望在这里抛砖引玉，以后多多交流.

二.应用场景:

1)取代memcached,当缓存系统使用.

2)做热点缓存服务器（基于LRU算法），容量大小可配置,无需再为缓存的实效时间头疼.

3)对缓存中的大数组，可以单独对部分数据进行增删改查

4)对缓存中的列表数据（如好友id列表），可以分页获取（原生memcache客户端不支持）

三.实现要点：
1)使用epoll+多线程实现一个高性能

高并发的Server主线程负责接收网络新连接，并将recv回来的数据写到一个链表里面去（recv\_mlist）,而其他子线程就从链表（recv\_mlist）获取数据，并进行业务操作，最后将结果send回给客户端,支持长连接（长连接的性能比短连接性能高很多）,当前版本不支持跨平台

2)实现LRU(Least Recently Used)算法

程序中结合业务的特性，使用N个双向链表+哈希表（开放地址法）来实现的。
哈希表用于快速定位key在那个bucket,而链表用于实现对权重（weight）排序，多个链表是用于维护业务的合理性,每一条数据都有一个权重（weight）,当操作一次（get set replace increment decrement append prepend）权重就会加1,程序会自动修正哈希桶的状态，避免哈希桶在经过多次删除数据后，查找性能下降的问题.

3)支持结构化数据存储

可以对数据的某个键做操作，支持以下操作：

1.为数组加多一个key

2. unset数组的一个key

3. set/replace数组的一个key

4. append/prepend数组的一个key（该key的值要求是一个字符串）

5. increment/decrement数组的一个key(该key的值要求是一个数字)

6. 可以对数组进行pop/push/shift/unshift操作

7. 可以对数组进行count操作

8. 可以对数组进行分页获取操作

四.已知缺陷

1.尚不支持memcached的cas操作(原子操作).

2.不支持一致性哈希，这是由于我们将@path放到key，影响的一致性哈希的结果.

3.暂不支持数据压缩传送

4.不支持UDP协议

5.在ubuntu下对quit指令处理有误


五.下个版本期望改进点

1.实现内存复用机制,优化内存分配（slab）,减少内存占用率

2.提供一个自己的client,支持所有指令,支持一致性哈希

3.支持数据压缩存储

4.支持json格式

5.数据持久化