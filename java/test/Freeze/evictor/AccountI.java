// **********************************************************************
//
// Copyright (c) 2003-2007 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

public class AccountI extends Test.Account
{
    public int
    getBalance(Ice.Current current)
    {
        return balance;
    }

    public void
    deposit(int amount, Ice.Current current) throws Test.InsufficientFundsException
    {
        //
        // No need to synchronize since everything occurs within its own transaction
        //
        int newBalance = balance + amount;
        if(newBalance < 0)
        {
            throw new Test.InsufficientFundsException();
        }
        balance = newBalance;
    }

    public void
    transfer(int amount, Test.AccountPrx toAccount, Ice.Current current) throws Test.InsufficientFundsException
    {
        test(_evictor.getCurrentTransaction() != null);

        toAccount.deposit(amount); // collocated call
        deposit(-amount, current); // direct call
    }

    public void
    transfer2_async(Test.AMD_Account_transfer2 cb, int amount, Test.AccountPrx toAccount, Ice.Current current)
    {
        //
        // Here the dispatch thread does everything
        //
        test(_evictor.getCurrentTransaction() != null);

        try
        {
            toAccount.deposit(amount); // collocated call
            deposit(-amount, current); // direct call
        }
        catch(Test.InsufficientFundsException ex)
        {
            cb.ice_exception(ex);
        }

        cb.ice_response();
    }

    public void
    transfer3_async(final Test.AMD_Account_transfer3 cb, int amount, Test.AccountPrx toAccount, Ice.Current current)
    {
        //
        // Here the dispatch thread does the actual work, but a separate thread sends the response
        //

        class ResponseThread extends Thread
        {
            ResponseThread(Test.AMD_Account_transfer3 cb)
            {
                _cb = cb;
            }

            synchronized void response()
            {
                _response = true;
                notify();
            }

            synchronized void exception(Ice.UserException e)
            {
                _exception = e;
                notify();
            }


            public synchronized void run()
            {
                if(_response == false && _exception == null)
                {
                    try
                    {
                        wait(1000);
                    }
                    catch(InterruptedException e)
                    {
                    }
                }
                try
                {
                    test(_evictor.getCurrentTransaction() == null);
                }
                catch(Freeze.EvictorDeactivatedException ex)
                {
                    //
                    // Clearly nobody is waiting for a response!
                    //
                    return;
                }

                if(_response)
                {
                    cb.ice_response();
                }
                else if(_exception != null)
                {
                    cb.ice_exception(_exception);
                }
                else
                {
                    //
                    // We don't wait forever!
                    //
                    cb.ice_exception(new Ice.TimeoutException());
                }
            }

            private Test.AMD_Account_transfer3 _cb;
            private boolean _response = false;
            private Ice.UserException _exception;
        };

        ResponseThread thread = new ResponseThread(cb);
        thread.setDaemon(true);
        thread.start();

        test(_evictor.getCurrentTransaction() != null);

        try
        {
            toAccount.deposit(amount); // collocated call
            deposit(-amount, current); // direct call
        }
        catch(Ice.UserException e)
        {
            //
            // Need to rollback here -- "rollback on user exception" does not work
            // when the dispatch commits before it gets any response!
            //
            _evictor.getCurrentTransaction().rollback();

            thread.exception(e);
        }

        thread.response();
    }

    public AccountI(int initialBalance, Freeze.TransactionalEvictor evictor)
    {
        super(initialBalance);
        _evictor = evictor;
    }

    public AccountI()
    {
    }

    public void init(Freeze.TransactionalEvictor evictor)
    {
        assert _evictor == null;
        _evictor = evictor;
    }

    private Freeze.TransactionalEvictor _evictor;

    private static void
    test(boolean b)
    {
        if(!b)
        {
            throw new RuntimeException();
        }
    }
}
