#include "compress.h"
extern "C" {
#include <librdkafka/snappy.h>
}

namespace flame::compress {
    
    static php::value snappy_compress(php::parameters& params) {
        php::string data = params[0].to_string();
        struct iovec iov_in = {
            .iov_base = data.data(),
            .iov_len = data.size(),
        };
        std::size_t len = rd_kafka_snappy_max_compressed_length(data.size());
        php::string out(len);
        struct iovec iov_out = {
            .iov_base = out.data(),
            .iov_len = 0xffffffff,
        };
        struct snappy_env env;
        rd_kafka_snappy_init_env(&env);
        rd_kafka_snappy_compress_iov(&env, &iov_in, 1, data.size(), &iov_out);
        rd_kafka_snappy_free_env(&env);
        out.shrink(iov_out.iov_len);
        return std::move(out);
    }

    static php::value snappy_uncompress(php::parameters& params) {
        php::string data = params[0].to_string();
        std::size_t len;
        rd_kafka_snappy_uncompressed_length(data.c_str(), data.size(), &len);
        php::string out(len);
        rd_kafka_snappy_uncompress(data.c_str(), data.size(), out.data());
        return std::move(out);
    }

    void declare(php::extension_entry &ext) {
        ext
            .function<snappy_compress>("flame\\compress\\snappy_compress", {
                {"data", php::TYPE::STRING},
            })
            .function<snappy_uncompress>("flame\\compress\\snappy_uncompress", {
                {"data", php::TYPE::STRING},
            });
    }
}
