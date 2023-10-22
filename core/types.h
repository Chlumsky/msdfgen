
#pragma once

namespace msdfgen {

typedef unsigned char byte;
typedef unsigned unicode_t;

#ifdef MSDFGEN_REAL
typedef MSDFGEN_REAL real;
#else
typedef double real;
#endif

}
