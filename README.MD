# TinyCraft

## Implementation Details

### Rendering

#### Generate Surface Mesh

原始数据相当于一个超大体数据的一个数据块```chunk```（多了一个padding），需要据此生成其表面的```mesh```。
我们需要找到那些可见的面，然后生成相应的```quad```（即两个三角形）。为了简单起见，冗余存储每个三角形的顶点。
基本的思路就是遍历一次chunk的每一个体素，每个体素查询周围6个面是否被遮挡，任何一个面如果没被遮挡就生成一个```quad```
这样子算法的时间复杂度在```6*sizeof(chunk)```。
如果每个```chunk```的尺寸为```32x32x256(xzy)```，那么需要```1.5MB次```。
每个chunk需要存储一个体数据块，为了加速遍历，我们只存储非空的体素，但同时为了方便增加（删除）体素，需要将其存储到
一个链表结构中，否则线性内存的增加和删除元素代价太大（参照vector），可以只用一个unordered_map。
甚至可以在存储key-value时（key为体素在chunk内的xyz坐标，value代表体素的值），在value里存储额外信息，比如是否完全不可见。
每个Cube可以有多个纹理贴图去选择，但是如果把每个纹理都单独保存成一张图片或者说加载为一个纹理对象的话 会占用更多的资源 主要是在shader里选择纹理需要额外的信息去判断 而且选择纹理这一行为本身是有代价的 那么可以将所有Cube的纹理放到一个大的纹理图中 而在生成Cube的时候 根据其纹理贴图的类别 生成相应的不同的纹理坐标 那么在shader里进行纹理采样时就无需判断 直接使用纹理坐标得到正确的贴图

加载多个Chunk可以采用多线程异步方式 但是生成OpenGL Buffer只能在主线程中 一般来说 这一步骤是非常快的 不会造成画面卡顿 但是如果内存中保存的Chunk太多 每一个Chunk都会保留生成的OpenGL Buffer 最终会导致显存爆满 另外每一个Chunk生成的mesh在生成OpenGL Buffer即上传到显存后应该清楚 否则如果电脑内存不够用 导致内存页面被置换到缓存 会十分缓慢 
OpenGL在多个线程中 需要MakeCurrentContext甚至加锁 才可以正确生成Buffer 但是在我的测试中 可能会有一些少数生成失败的Buffer 可能是锁没有加好 也就是生成Buffer应该是串行的

在跳跃运动过程中 到达最高点的速度可能是0 这个时候可以附加一个加速度用来判断当前的状态是静止在地面还是到了跳起的最高点相对静止状态

OpenGL Context 是thread_local变量 更准确地说 global static? thread_local类型的变量 每一个线程都只能有一个OpenGL Context
一个程序可以创建多个OpenGL Context 但是一个Context同时只能被一个线程使用

#### Noise to generate initial world
一开始的世界使用噪声函数生成 噪声函数采用与 x z 二维坐标相关的 也就是说 每次只要传入的坐标一样 噪声值就一样 从而保证每次生成的世界都是一样的




