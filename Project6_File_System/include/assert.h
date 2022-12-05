#ifndef ASSERT_H
#define ASSERT_H

#include <printk.h>

static inline void _panic(const char* file_name,int lineno, const char* func_name)
{
    printk("Assertion failed at %s in %s:%d\n\r",
           func_name,file_name,lineno);
    for(;;);
}

#define assert(cond)                                 \
    {                                                \
        if (!(cond)) {                               \
            _panic(__FILE__, __LINE__,__FUNCTION__); \
        }                                            \
    }

#endif /* ASSERT_H */
