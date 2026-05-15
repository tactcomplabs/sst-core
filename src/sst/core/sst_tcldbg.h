//
// Copyright (C) 2017-2026 Tactical Computing Laboratories, LLC
// All Rights Reserved
// contact@tactcomplabs.com
// See LICENSE in the top level directory for licensing details
//

#ifndef _SST_TCLDBG_H
#define _SST_TCLDBG_H

#include <iostream>
#include <unistd.h>

namespace sst_tcldbg {

#pragma GCC push_options
#pragma GCC optimize ("O0")
static inline void spin(const char* id = "") {
  std::cout << id << " spinning PID " << getpid() << std::endl;
  unsigned long spinner = 1;
  while( spinner > 0 ) {
    spinner++;
    usleep(100000);
    if( spinner % 10 == 0 )  // breakpoint here
      std::cout << "." << std::flush;
  }
  std::cout << std::endl;
}
#pragma GCC pop_options

static inline void spinner( const char* id, bool cond = true ) {
  if( !std::getenv( id ) )
    return;
  if( !cond )
    return;
  spin(id);
}


}  //namespace sst_tcldbg

#endif  //_SST_TCLDBG_H
