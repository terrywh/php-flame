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

    void _connection_lock::begin_tx(coroutine_handler &ch)
    {
        int err = 0;
        boost::asio::post(gcontroller->context_y, [this, &ch, &err]() {
            err = mysql_real_query(conn_.get(), "START TRANSACTION", 17);
            ch.resume();
        });
        ch.suspend();
        if(err != 0) {
            err = mysql_errno(conn_.get());
            throw php::exception(zend_ce_error,
                                 (boost::format("failed to begin transaction: (%1%) %2%") % err % mysql_error(conn_.get())).str(),
                                 err);
        }
        
    }

    void _connection_lock::commit(coroutine_handler &ch)
    {
        int err = 0;
        boost::asio::post(gcontroller->context_y, [this, &ch, &err]() {
            err = mysql_real_query(conn_.get(), "COMMIT", 6);
            ch.resume();
        });
        ch.suspend();
        if (err != 0)
        {
            err = mysql_errno(conn_.get());
            throw php::exception(zend_ce_exception,
                                 (boost::format("failed to commit MySQL tx: (%1%) %2%") % err % mysql_error(conn_.get())).str(),
                                 err);
        }
    }
    void _connection_lock::rollback(coroutine_handler &ch) 
    {
        int err = 0;
        boost::asio::post(gcontroller->context_y, [this, &ch, &err]() {
            err = mysql_real_query(conn_.get(), "ROLLBACK", 8);
            ch.resume();
        });
        ch.suspend();
        if (err != 0)
        {
            err = mysql_errno(conn_.get());
            throw php::exception(zend_ce_error,
                                 (boost::format("failed to rollback MySQL tx: (%1%) %2%") % err % mysql_error(conn_.get())).str(),
                                 err);
        }
    }

} // namespace flame::mysql
