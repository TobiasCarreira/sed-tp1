#ifndef __UTIL_H
#define __UTIL_H

#include "atomic.h"     // class Atomic
#include "message.h"       // class InternalMessage
#include "parsimu.h"      // class Simulator
#include "strutil.h"       // str2Value( ... )
#include <vector>
using namespace std;

#define CYCLE_TIME VTime(1)

extern vector<Real> PGIs;
extern vector<vector<Real> > productsAffinity;

#endif //__UTIL_H
