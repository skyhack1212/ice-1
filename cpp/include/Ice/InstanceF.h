// **********************************************************************
//
// Copyright (c) 2002
// MutableRealms, Inc.
// Huntsville, AL, USA
//
// All Rights Reserved
//
// **********************************************************************

#ifndef ICE_INSTANCE_F_H
#define ICE_INSTANCE_F_H

#include <Ice/Handle.h>

namespace __Ice
{

class InstanceI;
void incRef(InstanceI*);
void decRef(InstanceI*);
typedef __Ice::Handle<InstanceI> Instance;

}

#endif
