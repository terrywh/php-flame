#### 依赖
##### c++ 11
* -std=c+11
##### boost
* system
* asio
##### mongodb c driver
```
CFLAGS=-fPIC ./configure --enable-sasl=yes --enable-ssl=openssl --disable-shared --enable-static --disable-automatic-init-and-cleanup --prefix=/data/vendor/mongo
make
sudo make install
```
