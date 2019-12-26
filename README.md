
## kcp-hpp

A single header file version of kcp protocol.
- complete notes.
- RDC reliability data control support.
- loss rate and other data statistics.

define macro `KCP_EXTENTION` to open extention.

You can use custom namespace inclusion to make it easy to call all interface functions.

```cpp
namespace kcp{
#include<ikcp.hpp>
}
kcp::ikcp_create();
```

Used as a project submodule:
```bash
git submodule add https://github.com/Mainvooid/kcp-hpp.git /3rdparty
git submodule update --init --recursive
```

#### KCP Features

- Average RTT reduce 30% - 40% vs traditional ARQ like tcp.
- Maximum RTT reduce three times vs tcp.
- Lightweight, distributed as a single source file.
