/* Copyright (C) 2012 Ian MacLarty */
#ifndef LTCOMMON_H
#define LTCOMMON_H

#include <stdlib.h>
#include <stdio.h>

#include "ltutil.h"

#define ASSERT_CONCAT_(a, b) a##b
#define ASSERT_CONCAT(a, b) ASSERT_CONCAT_(a, b)
#define ct_assert(e) enum { ASSERT_CONCAT(assert_line_, __LINE__) = 1/(!!(e)) }

#define LT_PI 3.14159265358979323846f
#define LT_RADIANS_PER_DEGREE (LT_PI / 180.0f)
#define LT_DEGREES_PER_RADIAN (180.0f / LT_PI)

#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)

typedef float           LTfloat;
typedef double          LTdouble;
typedef unsigned int    LTuint;
typedef unsigned short  LTushort;
typedef unsigned char   LTubyte;
typedef unsigned int    LTuint32;

typedef float           LTsecs;
typedef float           LTdegrees;

typedef LTuint32        LTpixel;

#include "ltobject.h"

#endif
