#include "access_entry.h"
#include <php/Zend/zend_API.h>

namespace flame::core {

access_entry::modifier access_entry::public_    {ZEND_ACC_PUBLIC   };
access_entry::modifier access_entry::private_   {ZEND_ACC_PRIVATE  };
access_entry::modifier access_entry::protected_ {ZEND_ACC_PROTECTED};
access_entry::modifier access_entry::static_    {ZEND_ACC_STATIC   };
access_entry::modifier access_entry::final_     {ZEND_ACC_FINAL    };
access_entry::modifier access_entry::abstract_  {ZEND_ACC_ABSTRACT };

} // flame::core
