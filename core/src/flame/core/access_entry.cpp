#include "access_entry.h"
#include <php/Zend/zend_API.h>

namespace flame::core {

access_entry access_entry::public_    {ZEND_ACC_PUBLIC   };
access_entry access_entry::private_   {ZEND_ACC_PRIVATE  };
access_entry access_entry::protected_ {ZEND_ACC_PROTECTED};

} // flame::core
