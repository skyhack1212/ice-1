// **********************************************************************
//
// Copyright (c) 2003-2009 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#include <Ice/Ice.h>
#include <TestCommon.h>
#include <Test.h>

using namespace std;
using namespace Test;

class CallbackBase : public IceUtil::Monitor<IceUtil::Mutex>
{
public:

    CallbackBase() :
        _called(false)
    {
    }

    virtual ~CallbackBase()
    {
    }

    void check()
    {
        IceUtil::Monitor<IceUtil::Mutex>::Lock sync(*this);
        while(!_called)
        {
            wait();
        }
        _called = false;
    }

protected:

    void called()
    {
        IceUtil::Monitor<IceUtil::Mutex>::Lock sync(*this);
        assert(!_called);
        _called = true;
        notify();
    }

private:

    bool _called;
};

class AMIRegular : public Test::AMI_Retry_op, public CallbackBase
{
public:

    virtual void ice_response()
    {
        called();
    }

    virtual void ice_exception(const ::Ice::Exception&)
    {
        test(false);
    }
};

typedef IceUtil::Handle<AMIRegular> AMIRegularPtr;

class AMIException : public Test::AMI_Retry_op, public CallbackBase
{
public:

    virtual void ice_response()
    {
        test(false);
    }

    virtual void ice_exception(const ::Ice::Exception& ex)
    {
        test(dynamic_cast<const Ice::ConnectionLostException*>(&ex));
        called();
    }
};

typedef IceUtil::Handle<AMIException> AMIExceptionPtr;

RetryPrx
allTests(const Ice::CommunicatorPtr& communicator)
{
    cout << "testing stringToProxy... " << flush;
    string ref = "retry:default -p 12010";
    Ice::ObjectPrx base1 = communicator->stringToProxy(ref);
    test(base1);
    Ice::ObjectPrx base2 = communicator->stringToProxy(ref);
    test(base2);
    cout << "ok" << endl;

    cout << "testing checked cast... " << flush;
    RetryPrx retry1 = RetryPrx::checkedCast(base1);
    test(retry1);
    test(retry1 == base1);
    RetryPrx retry2 = RetryPrx::checkedCast(base2);
    test(retry2);
    test(retry2 == base2);
    cout << "ok" << endl;

    cout << "calling regular operation with first proxy... " << flush;
    retry1->op(false);
    cout << "ok" << endl;

    cout << "calling operation to kill connection with second proxy... " << flush;
    try
    {
        retry2->op(true);
        test(false);
    }
    catch(Ice::ConnectionLostException)
    {
        cout << "ok" << endl;
    }

    cout << "calling regular operation with first proxy again... " << flush;
    retry1->op(false);
    cout << "ok" << endl;

    AMIRegularPtr cb1 = new AMIRegular;
    AMIExceptionPtr cb2 = new AMIException;

    cout << "calling regular AMI operation with first proxy... " << flush;
    retry1->op_async(cb1, false);
    cb1->check();
    cout << "ok" << endl;

    cout << "calling AMI operation to kill connection with second proxy... " << flush;
    retry2->op_async(cb2, true);
    cb2->check();
    cout << "ok" << endl;

    cout << "calling regular AMI operation with first proxy again... " << flush;
    retry1->op_async(cb1, false);
    cb1->check();
    cout << "ok" << endl;

    cout << "calling regular AMI operation with new AMI mapping with first proxy... " << flush;
    Ice::AsyncResultPtr r = retry1->begin_op(false);
    retry1->end_op(r);
    cout << "ok" << endl;

    cout << "calling AMI operation with new AMI mapping to kill connection with second proxy... " << flush;
    r = retry2->begin_op(true);
    try
    {
	retry2->end_op(r);
    }
    catch(const Ice::ConnectionLostException&)
    {
    }
    cout << "ok" << endl;

    cout << "calling regular AMI operation with first proxy again... " << flush;
    r = retry1->begin_op(false);
    retry1->end_op(r);
    cout << "ok" << endl;

    return retry1;
}
