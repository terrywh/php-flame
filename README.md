### 依赖库

#### PHP
``` Bash
./configure  --prefix=/data/vendor/php-7.2.11 --with-config-file-path=/data/vendor/php-7.0.32/etc --disable-fpm --disable-phar --disable-dom --disable-libxml --disable-simplexml --disable-xml --disable-xmlreader --disable-xmlwriter --with-openssl --with-readline --enable-mbstring --without-pear
```
#### Boost

``` Bash
./bootstrap.sh --prefix=/data/vendor/boost-1.68.0
./b2 -j4 --prefix=/data/vendor/boost-1.68.0 cxxflags="-fPIC" variant=release link=static threading=multi install
```

#### libphpext
``` Bash
CXXFLAGS="-O2" make -j4
make install
```

#### cpp-parser
``` Bash
make install
```

#### mysql-connector-c v6.1.11
``` Bash
mv mysql-connector-c-6.1.11-src/ /data/vendor/mysqlc-6.1.11
rm /data/vendor/mysqlc-6.1.11/lib/*.so*
```

#### mongoc-driver
``` Bash
mkdir stage && cd stage
CC=gcc CXX=g++ cmake -DCMAKE_INSTALL_PREFIX=/data/vendor/mongoc-1.13.0 -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS=-fPIC -DENABLE_STATIC=ON -DENABLE_SHM_COUNTERS=OFF -DENABLE_TESTS=OFF -DENABLE_EXAMPLES=OFF -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF ../
make
make install
rm /data/vendor/mongoc-1.13.0/lib/*.so*
```

#### AMQP-CPP
``` Bash
mkdir stage && cd stage
CC=gcc CXX=g++ cmake3 -DCMAKE_INSTALL_PREFIX=/data/vendor/amqpcpp-4.0.0 -DCMAKE_CXX_FLAGS=-fPIC -DCMAKE_BUILD_TYPE=Release -DAMQP-CPP_LINUX_TCP=on ../
make
make install
rm /data/vendor/amqpcpp-4.0.0/lib/*.so*
```

#### Rdkafka
``` Bash
./configure --prefix=/data/vendor/rdkafka-0.11.6
make
make install
rm /data/vendor/rdkafka-0.11.6/lib/*.so*
```

#### HttpParser
``` Bash
mkdir -p /data/vendor/http-parser-2.8.1/lib
mkdir -p /data/vendor/http-parser-2.8.1/include
CFLAGS=-fPIC make libhttp_parser.o
cp libhttp_parser.o /data/vendor/http-parser-2.8.1/lib
cp http_parser.h /data/vendor/http-parser-2.8.1/include
```

#### HiRedis
``` Bash
make
PREFIX=/data/vendor/hiredis-0.14.0 make install
rm /data/vendor/hiredis-0.14.0/lib/*.so*
```


