// **********************************************************************
//
// Copyright (c) 2002
// MutableRealms, Inc.
// Huntsville, AL, USA
//
// All Rights Reserved
//
// **********************************************************************

#ifndef ICE_THREAD_POOL_F_H
#define ICE_THREAD_POOL_F_H

#include <Ice/Handle.h>

namespace __Ice
{

class ThreadPoolI;
void incRef(ThreadPoolI*);
void decRef(ThreadPoolI*);
typedef __Ice::Handle<ThreadPoolI> ThreadPool;

}

#endif
