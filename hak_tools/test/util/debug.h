//
// Created by hdevarajan on 8/24/17.
//

#ifndef MPIONESIDED_DEBUG_H
#define MPIONESIDED_DEBUG_H

#include <iostream>

#define DBGVAR(os, var) \
  (os) << "DBG: " << __FILE__ << "(" << __LINE__ << ") "\
       << #var << " = [" << (var) << "]" << std::endl
#define DBGVAR2(os, var1, var2) \
  (os) << "DBG: " << __FILE__ << "(" << __LINE__ << ") "\
       << #var1 << " = [" << (var1) << "]"\
       << #var2 << " = [" << (var2) << "]"  << std::endl
#define DBGVAR3(os, var1, var2, var3) \
  (os) << "DBG: " << __FILE__ << "(" << __LINE__ << ") "\
       << #var1 << " = [" << (var1) << "]"\
       << #var2 << " = [" << (var2) << "]"\
       << #var3 << " = [" << (var3) << "]"  << std::endl
#define DBGMSG(os, msg) \
  (os) << "DBG: " << __FILE__ << "(" << __LINE__ << ") " \
       << msg << std::endl
#endif //MPIONESIDED_DEBUG_H
