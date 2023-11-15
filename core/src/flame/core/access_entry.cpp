#include "access_entry.h"
#include <php/Zend/zend_API.h>

namespace flame::core {

access_entry::modifier public_    {ZEND_ACC_PUBLIC   };
access_entry::modifier private_   {ZEND_ACC_PRIVATE  };
access_entry::modifier protected_ {ZEND_ACC_PROTECTED};
access_entry::modifier static_    {ZEND_ACC_STATIC   };
access_entry::modifier final_     {ZEND_ACC_FINAL    };
access_entry::modifier abstract_  {ZEND_ACC_ABSTRACT };

} // flame::core
