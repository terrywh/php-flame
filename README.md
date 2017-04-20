#### 依赖

##### libmill

为了方便使用，使用 libmill 静态库：
``` bash
cd vendor/libmill
./autogen.sh
./configure --disable-shared CFLAGS=-fPIC
make
```

##### http-parser

为了方便使用，使用 http-parser 静态库：
``` bash
cd vendor/http-parser
make package CFLAGS=-fPIC
```