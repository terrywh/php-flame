#ifndef CORE_EXTENSION_TIME_H
#define CORE_EXTENSION_TIME_H

#include <phpext.h>
namespace core { namespace extension {
    //
    class time {
    public:
        static void declare(php::module_entry& entry);
    private:
        static php::value now(php::parameters& params); // milliseconds
        static php::value iso(php::parameters& params); // yyyy-mm-dd hh:ii:ss
        static php::value utc(php::parameters& params); // yyyy-mm-ddThh:ii:ssZ
        static php::value sleep(php::parameters& params);
    };
}}

#endif // CORE_EXTENSION_TIME_H
