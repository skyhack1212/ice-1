// **********************************************************************
//
// Copyright (c) 2003-2005 ZeroC, Inc. All rights reserved.
//
// This copy of Ice-E is licensed to you under the terms described in the
// ICEE_LICENSE file included in this distribution.
//
// **********************************************************************

#include <IceE/IceE.h>
#include <TestCommon.h>
#include <Test.h>

using namespace std;
using namespace Test;

class EmptyI : virtual public Empty
{
};

GPrx
allTests(const Ice::CommunicatorPtr& communicator)
{
    tprintf("testing facet registration exceptions...");
    Ice::ObjectAdapterPtr adapter = communicator->createObjectAdapter("FacetExceptionTestAdapter");
    Ice::ObjectPtr obj = new EmptyI;
    adapter->add(obj, Ice::stringToIdentity("d"));
    adapter->addFacet(obj, Ice::stringToIdentity("d"), "facetABCD");
    try
    {
	adapter->addFacet(obj, Ice::stringToIdentity("d"), "facetABCD");
	test(false);
    }
    catch(Ice::AlreadyRegisteredException&)
    {
    }
    adapter->removeFacet(Ice::stringToIdentity("d"), "facetABCD");
    try
    {
	adapter->removeFacet(Ice::stringToIdentity("d"), "facetABCD");
	test(false);
    }
    catch(Ice::NotRegisteredException&)
    {
    }
    tprintf("ok\n");

    tprintf("testing removeAllFacets...");
    Ice::ObjectPtr obj1 = new EmptyI;
    Ice::ObjectPtr obj2 = new EmptyI;
    adapter->addFacet(obj1, Ice::stringToIdentity("id1"), "f1");
    adapter->addFacet(obj2, Ice::stringToIdentity("id1"), "f2");
    Ice::ObjectPtr obj3 = new EmptyI;
    adapter->addFacet(obj1, Ice::stringToIdentity("id2"), "f1");
    adapter->addFacet(obj2, Ice::stringToIdentity("id2"), "f2");
    adapter->addFacet(obj3, Ice::stringToIdentity("id2"), "");
    Ice::FacetMap fm = adapter->removeAllFacets(Ice::stringToIdentity("id1"));
    test(fm.size() == 2);
    test(fm["f1"] == obj1);
    test(fm["f2"] == obj2);
    try
    {
	adapter->removeAllFacets(Ice::stringToIdentity("id1"));
	test(false);
    }
    catch(Ice::NotRegisteredException&)
    {
    }
    fm = adapter->removeAllFacets(Ice::stringToIdentity("id2"));
    test(fm.size() == 3);
    test(fm["f1"] == obj1);
    test(fm["f2"] == obj2);
    test(fm[""] == obj3);
    tprintf("ok\n");

    adapter->deactivate();

    tprintf("testing stringToProxy...");
    string ref = communicator->getProperties()->getPropertyWithDefault("Facets.Proxy", "d:default -p 12010 -t 10000");
    Ice::ObjectPrx db = communicator->stringToProxy(ref);
    test(db);
    tprintf("ok\n");

    tprintf("testing checked cast...");
    DPrx d = DPrx::checkedCast(db);
    test(d);
    test(d == db);
    tprintf("ok\n");

    tprintf("testing non-facets A, B, C, and D...");
    test(d->callA() == "A");
    test(d->callB() == "B");
    test(d->callC() == "C");
    test(d->callD() == "D");
    tprintf("ok\n");

    tprintf("testing facets A, B, C, and D...");
    DPrx df = DPrx::checkedCast(d, "facetABCD");
    test(df);
    test(df->callA() == "A");
    test(df->callB() == "B");
    test(df->callC() == "C");
    test(df->callD() == "D");
    tprintf("ok\n");

    tprintf("testing facets E and F...");
    FPrx ff = FPrx::checkedCast(d, "facetEF");
    test(ff);
    test(ff->callE() == "E");
    test(ff->callF() == "F");
    tprintf("ok\n");

    tprintf("testing facet G...");
    GPrx gf = GPrx::checkedCast(ff, "facetGH");
    test(gf);
    test(gf->callG() == "G");
    tprintf("ok\n");

    tprintf("testing whether casting preserves the facet...");
    HPrx hf = HPrx::checkedCast(gf);
    test(hf);
    test(hf->callG() == "G");
    test(hf->callH() == "H");
    tprintf("ok\n");

    return gf;
}
