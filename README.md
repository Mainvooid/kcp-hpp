
## kcp-hpp
.hpp版kcp
- 完整注释
- RDC可靠性数据控制支持
- 丢包率等数据统计

`KCP_EXTENTION`宏开启扩展.


include时可以使用自定义命名空间包含，这样可以方便调用所有接口函数。
```cpp
namespace kcp{
#include<ikcp.hpp>
}
kcp::ikcp_create();
```