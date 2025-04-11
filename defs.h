/*! \file
    \brief DEfinitions for marty_utf
 */


#pragma once

// #ifndef MARTY_UTF_MARTY_DECIMAL_H
// #define MARTY_UTF_MARTY_DECIMAL_H

#ifndef MARTY_UTF_ASSERT_FAIL

    //#if defined(UMBA_ASSERT_FAIL)

        #define MARTY_UTF_ASSERT_FAIL(msg)     throw std::runtime_error(msg)

    //#else

    //    #define MARTY_UTF_ASSERT_FAIL(msg)    UMBA_ASSERT_FAIL(msg)

    //#endif

#endif

//----------------------------------------------------------------------------
#ifndef MARTY_ARG_USED

    //! Подавление варнинга о неиспользованном аргументе
    #define MARTY_ARG_USED(x)                   (void)(x)

#endif
