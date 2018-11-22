#include "../controller.h"
#include "../coroutine.h"
#include "_connection_base.h"
#include "_connection_lock.h"

namespace flame::mysql
{

    _connection_lock::_connection_lock(std::shared_ptr<MYSQL> c)
        : conn_(c)
    {
        
    }
    _connection_lock::~_connection_lock()
    {
    }
    std::shared_ptr<MYSQL> _connection_lock::acquire(coroutine_handler &ch)
    {
        return conn_;
    }

} // namespace flame
