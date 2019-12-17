#include "hash.h"
#include <boost/crc.hpp>
extern "C" {
    #include <librdkafka/rdmurmur2.h>
    #include <librdkafka/xxhash.h>
}

namespace flame::hash {
    
    template <typename INTEGER_TYPE>
    static php::string dec2hex(INTEGER_TYPE dec) {
        php::string hex(sizeof(INTEGER_TYPE) * 2);
        sprintf(hex.data(), "%0*lx", sizeof(INTEGER_TYPE) * 2, dec);
        return std::move(hex);
    }

    static php::value murmur2(php::parameters& params) {
        php::string data = params[0].to_string();
        std::uint32_t raw = rd_murmur2(data.c_str(), data.size());
        if (params.size() > 1 && params[1].to_boolean()) return raw;
        return dec2hex<std::uint32_t>(raw);
    }

    static php::value xxh64(php::parameters& params) {
        php::string data = params[0].to_string();
        unsigned long long seed = 0;
        if (params.size() > 1) {
            seed = params[1].to_integer();
        }
        std::uint64_t raw = XXH64(data.c_str(), data.size(), seed);
        if (params.size() > 2 && params[2].to_boolean()) return raw;
        return dec2hex<std::uint64_t>(raw);
    }

    static php::value crc64(php::parameters& params) {
        php::string data = params[0].to_string();
        boost::crc_optimal<64, 0x42F0E1EBA9EA3693, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, true, true>  crc64;
        crc64.process_bytes(data.c_str(), data.size());
        std::uint64_t raw = crc64.checksum();
        if (params.size() > 1 && params[1].to_boolean()) return raw;
        return dec2hex<std::uint64_t>(raw);
    }

    void declare(php::extension_entry &ext) {
        ext
            .function<murmur2>("flame\\hash\\murmur2", {
                {"data", php::TYPE::STRING},
            })
            .function<xxh64>("flame\\hash\\xxh64", {
                {"data", php::TYPE::STRING},
                {"seed", php::TYPE::INTEGER, false, true},
            })
            .function<crc64>("flame\\hash\\crc64", {
                {"data", php::TYPE::STRING},
            });
    }
}
