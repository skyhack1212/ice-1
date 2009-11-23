// **********************************************************************
//
// Copyright (c) 2003-2009 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

package test.Ice.operations;

import test.Ice.operations.Test.Callback_MyClass_opVoid;
import test.Ice.operations.Test.Callback_MyClass_opBool;
import test.Ice.operations.Test.Callback_MyClass_opBoolS;
import test.Ice.operations.Test.Callback_MyClass_opBoolSS;
import test.Ice.operations.Test.Callback_MyClass_opByte;
import test.Ice.operations.Test.Callback_MyClass_opByteBoolD;
import test.Ice.operations.Test.Callback_MyClass_opByteS;
import test.Ice.operations.Test.Callback_MyClass_opByteSS;
import test.Ice.operations.Test.Callback_MyClass_opContext;
import test.Ice.operations.Test.Callback_MyClass_opFloatDouble;
import test.Ice.operations.Test.Callback_MyClass_opFloatDoubleS;
import test.Ice.operations.Test.Callback_MyClass_opFloatDoubleSS;
import test.Ice.operations.Test.Callback_MyClass_opIntS;
import test.Ice.operations.Test.Callback_MyClass_opLongFloatD;
import test.Ice.operations.Test.Callback_MyClass_opMyClass;
import test.Ice.operations.Test.Callback_MyClass_opMyEnum;
import test.Ice.operations.Test.Callback_MyClass_opShortIntD;
import test.Ice.operations.Test.Callback_MyClass_opShortIntLong;
import test.Ice.operations.Test.Callback_MyClass_opShortIntLongS;
import test.Ice.operations.Test.Callback_MyClass_opShortIntLongSS;
import test.Ice.operations.Test.Callback_MyClass_opString;
import test.Ice.operations.Test.Callback_MyClass_opStringMyEnumD;
import test.Ice.operations.Test.Callback_MyClass_opMyEnumStringD;
import test.Ice.operations.Test.Callback_MyClass_opStringS;
import test.Ice.operations.Test.Callback_MyClass_opStringSS;
import test.Ice.operations.Test.Callback_MyClass_opStringSSS;
import test.Ice.operations.Test.Callback_MyClass_opStringStringD;
import test.Ice.operations.Test.Callback_MyClass_opStruct;
import test.Ice.operations.Test.Callback_MyClass_opMyStructMyEnumD;
import test.Ice.operations.Test.Callback_MyClass_opDoubleMarshaling;
import test.Ice.operations.Test.Callback_MyDerivedClass_opDerived;
import test.Ice.operations.Test.AnotherStruct;
import test.Ice.operations.Test.MyClass;
import test.Ice.operations.Test.MyClassPrx;
import test.Ice.operations.Test.MyClassPrxHelper;
import test.Ice.operations.Test.MyDerivedClassPrx;
import test.Ice.operations.Test.MyDerivedClassPrxHelper;
import test.Ice.operations.Test.MyEnum;
import test.Ice.operations.Test.Structure;
import test.Ice.operations.Test.MyStruct;
import test.Ice.operations.Test.StateChangerPrx;
import test.Ice.operations.Test.StateChangerPrxHelper;

class TwowaysNewAMI
{
    private static void
    test(boolean b)
    {
        if(!b)
        {
            throw new RuntimeException();
        }
    }

    private static class Callback
    {
        Callback()
        {
            _called = false;
        }

        public synchronized void check()
        {
            while(!_called)
            {
                try
                {
                    wait();
                }
                catch(InterruptedException ex)
                {
                }
            }

            _called = false;
        }

        public synchronized void called()
        {
            assert(!_called);
            _called = true;
            notify();
        }

        private boolean _called;
    }

    private static class NoEndpointCallbackUnsafe extends Ice.AsyncCallback
    {
        @Override
        public void completed(Ice.AsyncResult r)
        {
            try
            {
                MyClassPrx p = MyClassPrxHelper.uncheckedCast(r.getProxy());
                p.end_opVoid(r);
                callback.called();
            }
            catch(Ice.NoEndpointException ex)
            {
                test(false);
            }
        }

        @Override
        public void sent(Ice.AsyncResult r)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class NoEndpointCallbackUnsafeEx extends Ice.AsyncCallback
    {
        @Override
        public void completed(Ice.AsyncResult r)
        {
            try
            {
                MyClassPrx p = MyClassPrxHelper.uncheckedCast(r.getProxy());
                p.end_opVoid(r);
                test(false);
            }
            catch(Ice.NoEndpointException ex)
            {
                callback.called();
            }
        }

        @Override
        public void sent(Ice.AsyncResult r)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class NoEndpointCallbackSafe extends Callback_MyClass_opVoid
    {
        @Override
        public void response()
        {
            test(false);
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(ex instanceof Ice.NoEndpointException);
            callback.called();
        }

        @Override
        public void sent()
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class opVoidI extends Callback_MyClass_opVoid
    {
        @Override
        public void response()
        {
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    /*
    private static class opVoidExI extends Ice.ResponseDelegate
    {
        public void
        ice_response()
        {
            test(false);
        }

        public void
        ice_exception(Ice.LocalException ex)
        {
            test(ex instanceof Ice.NoEndpointException);
            callback.called();
        }

        public void
        check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }
    */

    private static class opByteI extends Callback_MyClass_opByte
    {
        @Override
        public void response(byte r, byte b)
        {
            test(b == (byte)0xf0);
            test(r == (byte)0xff);
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    /*
    private static class opByteExI extends Callback_MyClass_opByte
    {
        public void
        ice_response(byte r, byte b)
        {
            test(false);
        }

        public void
        ice_exception(Ice.LocalException ex)
        {
            test(ex instanceof Ice.NoEndpointException);
            callback.called();
        }

        public void
        check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }
    */

    private static class opBoolI extends Callback_MyClass_opBool
    {
        @Override
        public void response(boolean r, boolean b)
        {
            test(b);
            test(!r);
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class opShortIntLongI extends Callback_MyClass_opShortIntLong
    {
        @Override
        public void response(long r, short s, int i, long l)
        {
            test(s == 10);
            test(i == 11);
            test(l == 12);
            test(r == 12);
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class opFloatDoubleI extends Callback_MyClass_opFloatDouble
    {
        @Override
        public void response(double r, float f, double d)
        {
            test(f == 3.14f);
            test(d == 1.1E10);
            test(r == 1.1E10);
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class opStringI extends Callback_MyClass_opString
    {
        @Override
        public void response(String r, String s)
        {
            test(s.equals("world hello"));
            test(r.equals("hello world"));
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class opMyEnumI extends Callback_MyClass_opMyEnum
    {
        @Override
        public void response(MyEnum r, MyEnum e)
        {
            test(e == MyEnum.enum2);
            test(r == MyEnum.enum3);
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class opMyClassI extends Callback_MyClass_opMyClass
    {
        opMyClassI(Ice.Communicator communicator)
        {
            _communicator = communicator;
        }

        @Override
        public void response(MyClassPrx r, MyClassPrx c1, MyClassPrx c2)
        {
            test(c1.ice_getIdentity().equals(_communicator.stringToIdentity("test")));
            test(c2.ice_getIdentity().equals(_communicator.stringToIdentity("noSuchIdentity")));
            test(r.ice_getIdentity().equals(_communicator.stringToIdentity("test")));
            // We can't do the callbacks below in connection serialization mode.
            if(_communicator.getProperties().getPropertyAsInt("Ice.ThreadPool.Client.Serialize") == 0)
            {
                r.opVoid();
                c1.opVoid();
                try
                {
                    c2.opVoid();
                    test(false);
                }
                catch(Ice.ObjectNotExistException ex)
                {
                }
            }
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
        private Ice.Communicator _communicator;
    }

    private static class opStructI extends Callback_MyClass_opStruct
    {
        opStructI(Ice.Communicator communicator)
        {
            _communicator = communicator;
        }

        @Override
        public void response(Structure rso, Structure so)
        {
            test(rso.p == null);
            test(rso.e == MyEnum.enum2);
            test(rso.s.s.equals("def"));
            test(so.e == MyEnum.enum3);
            test(so.s.s.equals("a new string"));
            // We can't do the callbacks below in connection serialization mode.
            if(_communicator.getProperties().getPropertyAsInt("Ice.ThreadPool.Client.Serialize") == 0)
            {
                so.p.opVoid();
            }
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
        private Ice.Communicator _communicator;
    }

    private static class opByteSI extends Callback_MyClass_opByteS
    {
        @Override
        public void response(byte[] rso, byte[] bso)
        {
            test(bso.length == 4);
            test(bso[0] == (byte)0x22);
            test(bso[1] == (byte)0x12);
            test(bso[2] == (byte)0x11);
            test(bso[3] == (byte)0x01);
            test(rso.length == 8);
            test(rso[0] == (byte)0x01);
            test(rso[1] == (byte)0x11);
            test(rso[2] == (byte)0x12);
            test(rso[3] == (byte)0x22);
            test(rso[4] == (byte)0xf1);
            test(rso[5] == (byte)0xf2);
            test(rso[6] == (byte)0xf3);
            test(rso[7] == (byte)0xf4);
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class opBoolSI extends Callback_MyClass_opBoolS
    {
        @Override
        public void response(boolean[] rso, boolean[] bso)
        {
            test(bso.length == 4);
            test(bso[0]);
            test(bso[1]);
            test(!bso[2]);
            test(!bso[3]);
            test(rso.length == 3);
            test(!rso[0]);
            test(rso[1]);
            test(rso[2]);
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class opShortIntLongSI extends Callback_MyClass_opShortIntLongS
    {
        @Override
        public void response(long[] rso, short[] sso, int[] iso, long[] lso)
        {
            test(sso.length == 3);
            test(sso[0] == 1);
            test(sso[1] == 2);
            test(sso[2] == 3);
            test(iso.length == 4);
            test(iso[0] == 8);
            test(iso[1] == 7);
            test(iso[2] == 6);
            test(iso[3] == 5);
            test(lso.length == 6);
            test(lso[0] == 10);
            test(lso[1] == 30);
            test(lso[2] == 20);
            test(lso[3] == 10);
            test(lso[4] == 30);
            test(lso[5] == 20);
            test(rso.length == 3);
            test(rso[0] == 10);
            test(rso[1] == 30);
            test(rso[2] == 20);
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class opFloatDoubleSI extends Callback_MyClass_opFloatDoubleS
    {
        @Override
        public void response(double[] rso, float[] fso, double[] dso)
        {
            test(fso.length == 2);
            test(fso[0] == 3.14f);
            test(fso[1] == 1.11f);
            test(dso.length == 3);
            test(dso[0] == 1.3E10);
            test(dso[1] == 1.2E10);
            test(dso[2] == 1.1E10);
            test(rso.length == 5);
            test(rso[0] == 1.1E10);
            test(rso[1] == 1.2E10);
            test(rso[2] == 1.3E10);
            test((float)rso[3] == 3.14f);
            test((float)rso[4] == 1.11f);
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class opStringSI extends Callback_MyClass_opStringS
    {
        @Override
        public void response(String[] rso, String[] sso)
        {
            test(sso.length == 4);
            test(sso[0].equals("abc"));
            test(sso[1].equals("de"));
            test(sso[2].equals("fghi"));
            test(sso[3].equals("xyz"));
            test(rso.length == 3);
            test(rso[0].equals("fghi"));
            test(rso[1].equals("de"));
            test(rso[2].equals("abc"));
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class opByteSSI extends Callback_MyClass_opByteSS
    {
        @Override
        public void response(byte[][] rso, byte[][] bso)
        {
            test(bso.length == 2);
            test(bso[0].length == 1);
            test(bso[0][0] == (byte)0xff);
            test(bso[1].length == 3);
            test(bso[1][0] == (byte)0x01);
            test(bso[1][1] == (byte)0x11);
            test(bso[1][2] == (byte)0x12);
            test(rso.length == 4);
            test(rso[0].length == 3);
            test(rso[0][0] == (byte)0x01);
            test(rso[0][1] == (byte)0x11);
            test(rso[0][2] == (byte)0x12);
            test(rso[1].length == 1);
            test(rso[1][0] == (byte)0xff);
            test(rso[2].length == 1);
            test(rso[2][0] == (byte)0x0e);
            test(rso[3].length == 2);
            test(rso[3][0] == (byte)0xf2);
            test(rso[3][1] == (byte)0xf1);
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class opBoolSSI extends Callback_MyClass_opBoolSS
    {
        @Override
        public void response(boolean[][] rso, boolean[][] bso)
        {
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class opShortIntLongSSI extends Callback_MyClass_opShortIntLongSS
    {
        @Override
        public void response(long[][] rso, short[][] sso, int[][] iso, long[][] lso)
        {
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class opFloatDoubleSSI extends Callback_MyClass_opFloatDoubleSS
    {
        @Override
        public void response(double[][] rso, float[][] fso, double[][] dso)
        {
            test(fso.length == 3);
            test(fso[0].length == 1);
            test(fso[0][0] == 3.14f);
            test(fso[1].length == 1);
            test(fso[1][0] == 1.11f);
            test(fso[2].length == 0);
            test(dso.length == 1);
            test(dso[0].length == 3);
            test(dso[0][0] == 1.1E10);
            test(dso[0][1] == 1.2E10);
            test(dso[0][2] == 1.3E10);
            test(rso.length == 2);
            test(rso[0].length == 3);
            test(rso[0][0] == 1.1E10);
            test(rso[0][1] == 1.2E10);
            test(rso[0][2] == 1.3E10);
            test(rso[1].length == 3);
            test(rso[1][0] == 1.1E10);
            test(rso[1][1] == 1.2E10);
            test(rso[1][2] == 1.3E10);
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class opStringSSI extends Callback_MyClass_opStringSS
    {
        @Override
        public void response(String[][] rso, String[][] sso)
        {
            test(sso.length == 5);
            test(sso[0].length == 1);
            test(sso[0][0].equals("abc"));
            test(sso[1].length == 2);
            test(sso[1][0].equals("de"));
            test(sso[1][1].equals("fghi"));
            test(sso[2].length == 0);
            test(sso[3].length == 0);
            test(sso[4].length == 1);
            test(sso[4][0].equals("xyz"));
            test(rso.length == 3);
            test(rso[0].length == 1);
            test(rso[0][0].equals("xyz"));
            test(rso[1].length == 0);
            test(rso[2].length == 0);
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class opStringSSSI extends Callback_MyClass_opStringSSS
    {
        @Override
        public void response(String[][][] rsso, String[][][] ssso)
        {
            test(ssso.length == 5);
            test(ssso[0].length == 2);
            test(ssso[0][0].length == 2);
            test(ssso[0][1].length == 1);
            test(ssso[1].length == 1);
            test(ssso[1][0].length == 1);
            test(ssso[2].length == 2);
            test(ssso[2][0].length == 2);
            test(ssso[2][1].length == 1);
            test(ssso[3].length == 1);
            test(ssso[3][0].length == 1);
            test(ssso[4].length == 0);
            test(ssso[0][0][0].equals("abc"));
            test(ssso[0][0][1].equals("de"));
            test(ssso[0][1][0].equals("xyz"));
            test(ssso[1][0][0].equals("hello"));
            test(ssso[2][0][0].equals(""));
            test(ssso[2][0][1].equals(""));
            test(ssso[2][1][0].equals("abcd"));
            test(ssso[3][0][0].equals(""));

            test(rsso.length == 3);
            test(rsso[0].length == 0);
            test(rsso[1].length == 1);
            test(rsso[1][0].length == 1);
            test(rsso[2].length == 2);
            test(rsso[2][0].length == 2);
            test(rsso[2][1].length == 1);
            test(rsso[1][0][0].equals(""));
            test(rsso[2][0][0].equals(""));
            test(rsso[2][0][1].equals(""));
            test(rsso[2][1][0].equals("abcd"));
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class opByteBoolDI extends Callback_MyClass_opByteBoolD
    {
        @Override
        public void response(java.util.Map<Byte, Boolean> ro, java.util.Map<Byte, Boolean> _do)
        {
            java.util.Map<Byte, Boolean> di1 = new java.util.HashMap<Byte, Boolean>();
            di1.put((byte)10, Boolean.TRUE);
            di1.put((byte)100, Boolean.FALSE);
            test(_do.equals(di1));
            test(ro.size() == 4);
            test(ro.get((byte)10).booleanValue() == true);
            test(ro.get((byte)11).booleanValue() == false);
            test(ro.get((byte)100).booleanValue() == false);
            test(ro.get((byte)101).booleanValue() == true);
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class opShortIntDI extends Callback_MyClass_opShortIntD
    {
        @Override
        public void response(java.util.Map<Short, Integer> ro, java.util.Map<Short, Integer> _do)
        {
            java.util.Map<Short, Integer> di1 = new java.util.HashMap<Short, Integer>();
            di1.put((short)110, -1);
            di1.put((short)1100, 123123);
            test(_do.equals(di1));
            test(ro.size() == 4);
            test(ro.get((short)110).intValue() == -1);
            test(ro.get((short)111).intValue() == -100);
            test(ro.get((short)1100).intValue() == 123123);
            test(ro.get((short)1101).intValue() == 0);
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class opLongFloatDI extends Callback_MyClass_opLongFloatD
    {
        @Override
        public void response(java.util.Map<Long, Float> ro, java.util.Map<Long, Float> _do)
        {
            java.util.Map<Long, Float> di1 = new java.util.HashMap<Long, Float>();
            di1.put(999999110L, new Float(-1.1f));
            di1.put(999999111L, new Float(123123.2f));
            test(_do.equals(di1));
            test(ro.size() == 4);
            test(ro.get(999999110L).floatValue() == -1.1f);
            test(ro.get(999999120L).floatValue() == -100.4f);
            test(ro.get(999999111L).floatValue() == 123123.2f);
            test(ro.get(999999130L).floatValue() == 0.5f);
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class opStringStringDI extends Callback_MyClass_opStringStringD
    {
        @Override
        public void response(java.util.Map<String, String> ro, java.util.Map<String, String> _do)
        {
            java.util.Map<String, String> di1 = new java.util.HashMap<String, String>();
            di1.put("foo", "abc -1.1");
            di1.put("bar", "abc 123123.2");
            test(_do.equals(di1));
            test(ro.size() == 4);
            test(ro.get("foo").equals("abc -1.1"));
            test(ro.get("FOO").equals("abc -100.4"));
            test(ro.get("bar").equals("abc 123123.2"));
            test(ro.get("BAR").equals("abc 0.5"));
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class opStringMyEnumDI extends Callback_MyClass_opStringMyEnumD
    {
        @Override
        public void response(java.util.Map<String, MyEnum> ro, java.util.Map<String, MyEnum> _do)
        {
            java.util.Map<String, MyEnum> di1 = new java.util.HashMap<String, MyEnum>();
            di1.put("abc", MyEnum.enum1);
            di1.put("", MyEnum.enum2);
            test(_do.equals(di1));
            test(ro.size() == 4);
            test(ro.get("abc") == MyEnum.enum1);
            test(ro.get("qwerty") == MyEnum.enum3);
            test(ro.get("") == MyEnum.enum2);
            test(ro.get("Hello!!") == MyEnum.enum2);
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class opMyEnumStringDI extends Callback_MyClass_opMyEnumStringD
    {
        @Override
        public void response(java.util.Map<MyEnum, String> ro, java.util.Map<MyEnum, String> _do)
        {
            java.util.Map<MyEnum, String> di1 = new java.util.HashMap<MyEnum, String>();
            di1.put(MyEnum.enum1, "abc");
            di1.put(MyEnum.enum2, "");
            test(_do.equals(di1));
            test(ro.size() == 3);
            test(ro.get(MyEnum.enum1).equals("abc"));
            test(ro.get(MyEnum.enum2).equals("Hello!!"));
            test(ro.get(MyEnum.enum3).equals("qwerty"));
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class opMyStructMyEnumDI extends Callback_MyClass_opMyStructMyEnumD
    {
        @Override
        public void response(java.util.Map<MyStruct, MyEnum> ro, java.util.Map<MyStruct, MyEnum> _do)
        {
            MyStruct s11 = new MyStruct(1, 1);
            MyStruct s12 = new MyStruct(1, 2);
            java.util.Map<MyStruct, MyEnum> di1 = new java.util.HashMap<MyStruct, MyEnum>();
            di1.put(s11, MyEnum.enum1);
            di1.put(s12, MyEnum.enum2);
            test(_do.equals(di1));
            MyStruct s22 = new MyStruct(2, 2);
            MyStruct s23 = new MyStruct(2, 3);
            test(ro.size() == 4);
            test(ro.get(s11) == MyEnum.enum1);
            test(ro.get(s12) == MyEnum.enum2);
            test(ro.get(s22) == MyEnum.enum3);
            test(ro.get(s23) == MyEnum.enum2);
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class opIntSI extends Callback_MyClass_opIntS
    {
        opIntSI(int l)
        {
            _l = l;
        }

        @Override
        public void response(int[] r)
        {
            test(r.length == _l);
            for(int j = 0; j < r.length; ++j)
            {
                test(r[j] == -j);
            }
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private int _l;
        private Callback callback = new Callback();
    }

    /*
    private static class opContextEqualI extends Callback_MyClass_opContext
    {
        Callback_MyClass_opContextEqualI(java.util.Map<String, String> d)
        {
            _d = d;
        }

        @Override
        public void response(java.util.Map r)
        {
            test(r.equals(_d));
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private java.util.Map<String, String> _d;
        private Callback callback = new Callback();
    }

    private static class opContextNotEqualI extends Callback_MyClass_opContext
    {
        Callback_MyClass_opContextNotEqualI(java.util.Map<String, String> d)
        {
            _d = d;
        }

        @Override
        public void response(java.util.Map r)
        {
            test(!r.equals(_d));
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private java.util.Map<String, String> _d;
        private Callback callback = new Callback();
    }
    */

    private static class opDerivedI extends Callback_MyDerivedClass_opDerived
    {
        @Override
        public void response()
        {
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class opDoubleMarshalingI extends Callback_MyClass_opDoubleMarshaling
    {
        @Override
        public void response()
        {
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class IsACallbackUnsafe extends Ice.AsyncCallback
    {
        @Override
        public void completed(Ice.AsyncResult r)
        {
            test(r.getProxy().end_ice_isA(r));
            callback.called();
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class IsACallbackSafe extends Ice.Callback_Object_ice_isA
    {
        @Override
        public void response(boolean ret)
        {
            test(ret);
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class PingCallbackUnsafe extends Ice.AsyncCallback
    {
        @Override
        public void completed(Ice.AsyncResult r)
        {
            r.getProxy().end_ice_ping(r);
            callback.called();
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class PingCallbackSafe extends Ice.Callback_Object_ice_ping
    {
        @Override
        public void response()
        {
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class IdsCallbackUnsafe extends Ice.AsyncCallback
    {
        @Override
        public void completed(Ice.AsyncResult r)
        {
            test(java.util.Arrays.equals(r.getProxy().end_ice_ids(r), r.getProxy().ice_ids()));
            callback.called();
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class IdsCallbackSafe extends Ice.Callback_Object_ice_ids
    {
        IdsCallbackSafe(String[] ids)
        {
            _ids = ids;
        }

        @Override
        public void response(String[] ids)
        {
            test(java.util.Arrays.equals(ids, _ids));
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private String[] _ids;
        private Callback callback = new Callback();
    }

    private static class IdCallbackUnsafe extends Ice.AsyncCallback
    {
        @Override
        public void completed(Ice.AsyncResult r)
        {
            test(r.getProxy().end_ice_id(r).equals(r.getProxy().ice_id()));
            callback.called();
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
    }

    private static class IdCallbackSafe extends Ice.Callback_Object_ice_id
    {
        IdCallbackSafe(String id)
        {
            _id = id;
        }

        @Override
        public void response(String id)
        {
            test(id.equals(_id));
            callback.called();
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private String _id;
        private Callback callback = new Callback();
    }

    enum ThrowType { LocalException, OtherException };

    private static void throwEx(ThrowType t)
    {
        switch(t)
        {
        case LocalException:
            throw new Ice.ObjectNotExistException();

        case OtherException:
            throw new NullPointerException();
        }
    }

    private static class ThrowerUnsafe extends Ice.AsyncCallback
    {
        ThrowerUnsafe(ThrowType t)
        {
            _t = t;
        }

        @Override
        public void completed(Ice.AsyncResult r)
        {
            MyClassPrx p = (MyClassPrx)r.getProxy();
            p.end_opVoid(r);
            callback.called();
            throwEx(_t);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
        private ThrowType _t;
    }

    private static class ThrowerSafe extends Callback_MyClass_opVoid
    {
        ThrowerSafe(ThrowType t)
        {
            _t = t;
        }

        @Override
        public void response()
        {
            callback.called();
            throwEx(_t);
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            test(false);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
        private ThrowType _t;
    }

    private static class ThrowerUnsafeEx extends Ice.AsyncCallback
    {
        ThrowerUnsafeEx(ThrowType t)
        {
            _t = t;
        }

        @Override
        public void completed(Ice.AsyncResult r)
        {
            MyClassPrx p = (MyClassPrx)r.getProxy();
            try
            {
                p.end_opVoid(r);
                test(false);
            }
            catch(Ice.LocalException ex)
            {
                callback.called();
                throwEx(_t);
            }
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
        private ThrowType _t;
    }

    private static class ThrowerSafeEx extends Callback_MyClass_opVoid
    {
        ThrowerSafeEx(ThrowType t)
        {
            _t = t;
        }

        @Override
        public void response()
        {
            test(false);
        }

        @Override
        public void exception(Ice.LocalException ex)
        {
            callback.called();
            throwEx(_t);
        }

        public void check()
        {
            callback.check();
        }

        private Callback callback = new Callback();
        private ThrowType _t;
    }

    private static class SentCounter extends Ice.AsyncCallback
    {
        SentCounter()
        {
            _queuedCount = 0;
            _completedCount = 0;
        }

        @Override
        public void completed(Ice.AsyncResult r)
        {
            test(r.isCompleted());
            synchronized(this)
            {
                ++_completedCount;
                notify();
            }
        }

        @Override
        public void sent(Ice.AsyncResult r)
        {
            synchronized(this)
            {
                ++_queuedCount;
            }
        }

        synchronized int queuedCount()
        {
            return _queuedCount;
        }

        synchronized void check(int size)
        {
            while(_completedCount != size)
            {
                try
                {
                    wait();
                }
                catch(InterruptedException ex)
                {
                }
            }
        }

        private int _queuedCount;
        private int _completedCount;
    }

    static void
    twowaysNewAMI(test.Util.Application app, MyClassPrx p)
    {
        Ice.Communicator communicator = app.communicator();

        {
            //
            // Check that a call to a void operation raises NoEndpointException
            // in the end_ method instead of at the point of call.
            //
            MyClassPrx indirect = MyClassPrxHelper.uncheckedCast(p.ice_adapterId("dummy"));
            Ice.AsyncResult r;

            r = indirect.begin_opVoid();
            try
            {
                indirect.end_opVoid(r);
                test(false);
            }
            catch(Ice.NoEndpointException ex)
            {
            }

            //
            // Check that a second call to the end_ method throws IllegalArgumentException.
            //
            try
            {
                indirect.end_opVoid(r);
                test(false);
            }
            catch(IllegalArgumentException ex)
            {
            }

            //
            // Use type-unsafe and type-safe variations of the callback.
            // Also test that the sent callback is not called in this case.
            //
            NoEndpointCallbackUnsafeEx cb1 = new NoEndpointCallbackUnsafeEx();
            indirect.begin_opVoid(cb1);
            cb1.check();

            NoEndpointCallbackSafe cb2 = new NoEndpointCallbackSafe();
            indirect.begin_opVoid(cb2);
            cb2.check();

            //
            // Check that calling the end_ method with a different proxy or for a different operation than the begin_
            // method throws IllegalArgumentException. If the test throws as expected, we never call the end_ method,
            // so this also tests that it is safe to throw the AsyncResult away without calling the end_ method.
            //
            MyClassPrx indirect1 = MyClassPrxHelper.uncheckedCast(p.ice_adapterId("dummy"));
            MyClassPrx indirect2 = MyClassPrxHelper.uncheckedCast(p.ice_adapterId("dummy2"));

            Ice.AsyncResult r1 = indirect1.begin_opVoid();
            Ice.AsyncResult r2 = indirect2.begin_opVoid();

            try
            {
                indirect1.end_opVoid(r2); // Wrong proxy
                test(false);
            }
            catch(IllegalArgumentException ex)
            {
            }

            try
            {
                indirect1.end_shutdown(r1); // Wrong operation
                test(false);
            }
            catch(IllegalArgumentException ex)
            {
            }
        }

        {
            //
            // Check that calling the end_ method with a null result throws IllegalArgumentException.
            //
            try
            {
                p.end_opVoid(null);
                test(false);
            }
            catch(IllegalArgumentException ex)
            {
            }
        }

        {
            //
            // Check that throwing an exception from the success callback doesn't cause problems.
            //
            {
                ThrowerSafe cb = new ThrowerSafe(ThrowType.LocalException);
                p.begin_opVoid(cb);
                cb.check();
            }

            {
                ThrowerSafe cb = new ThrowerSafe(ThrowType.OtherException);
                p.begin_opVoid(cb);
                cb.check();
            }

            {
                ThrowerUnsafe cb = new ThrowerUnsafe(ThrowType.LocalException);
                p.begin_opVoid(cb);
                cb.check();
            }

            {
                ThrowerUnsafe cb = new ThrowerUnsafe(ThrowType.OtherException);
                p.begin_opVoid(cb);
                cb.check();
            }
        }

        {
            //
            // Check that throwing an exception from the exception callback doesn't cause problems.
            //
            MyClassPrx indirect = MyClassPrxHelper.uncheckedCast(p.ice_adapterId("dummy"));

            {
                ThrowerSafeEx cb = new ThrowerSafeEx(ThrowType.LocalException);
                indirect.begin_opVoid(cb);
                cb.check();
            }

            {
                ThrowerSafeEx cb = new ThrowerSafeEx(ThrowType.OtherException);
                indirect.begin_opVoid(cb);
                cb.check();
            }

            {
                ThrowerUnsafeEx cb = new ThrowerUnsafeEx(ThrowType.LocalException);
                indirect.begin_opVoid(cb);
                cb.check();
            }

            {
                ThrowerUnsafeEx cb = new ThrowerUnsafeEx(ThrowType.OtherException);
                indirect.begin_opVoid(cb);
                cb.check();
            }
        }

        {
            //
            // Check that CommunicatorDestroyedException is raised directly.
            //
            Ice.InitializationData initData = new Ice.InitializationData();
            initData.properties = communicator.getProperties()._clone();
            Ice.Communicator ic = app.initialize(initData);
            Ice.ObjectPrx obj = ic.stringToProxy(p.toString());
            MyClassPrx p2 = MyClassPrxHelper.checkedCast(obj);

            ic.destroy();

            try
            {
                p2.begin_opVoid();
                test(false);
            }
            catch(Ice.CommunicatorDestroyedException ex)
            {
                // Expected.
            }
        }

        //
        // Test that marshaling works as expected, and that the delegates for each type of callback work.
        //

        {
            Ice.AsyncResult r = p.begin_opVoid();
            p.end_opVoid(r);
        }

        {
            opVoidI cb = new opVoidI();
            p.begin_opVoid(cb);
            cb.check();
        }

        {
            Ice.AsyncResult r = p.begin_opByte((byte)0xff, (byte)0x0f);
            Ice.ByteHolder p3 = new Ice.ByteHolder();
            byte ret = p.end_opByte(p3, r);
            test(p3.value == (byte)0xf0);
            test(ret == (byte)0xff);
        }

        {
            opByteI cb = new opByteI();
            p.begin_opByte((byte)0xff, (byte)0x0f, cb);
            cb.check();
        }

        {
            opBoolI cb = new opBoolI();
            p.begin_opBool(true, false, cb);
            cb.check();
        }

        {
            opShortIntLongI cb = new opShortIntLongI();
            p.begin_opShortIntLong((short)10, 11, 12L, cb);
            cb.check();
        }

        {
            opFloatDoubleI cb = new opFloatDoubleI();
            p.begin_opFloatDouble(3.14f, 1.1E10, cb);
            cb.check();
        }

        {
            opStringI cb = new opStringI();
            p.begin_opString("hello", "world", cb);
            cb.check();
        }

        {
            opMyEnumI cb = new opMyEnumI();
            p.begin_opMyEnum(MyEnum.enum2, cb);
            cb.check();
        }

        {
            opMyClassI cb = new opMyClassI(communicator);
            p.begin_opMyClass(p, cb);
            cb.check();
        }

        {
            Structure si1 = new Structure();
            si1.p = p;
            si1.e = MyEnum.enum3;
            si1.s = new AnotherStruct();
            si1.s.s = "abc";
            Structure si2 = new Structure();
            si2.p = null;
            si2.e = MyEnum.enum2;
            si2.s = new AnotherStruct();
            si2.s.s = "def";

            opStructI cb = new opStructI(communicator);
            p.begin_opStruct(si1, si2, cb);
            cb.check();
        }

        {
            final byte[] bsi1 =
                {
                    (byte)0x01,
                    (byte)0x11,
                    (byte)0x12,
                    (byte)0x22
                };
            final byte[] bsi2 =
                {
                    (byte)0xf1,
                    (byte)0xf2,
                    (byte)0xf3,
                    (byte)0xf4
                };

            opByteSI cb = new opByteSI();
            p.begin_opByteS(bsi1, bsi2, cb);
            cb.check();
        }

        {
            final boolean[] bsi1 = { true, true, false };
            final boolean[] bsi2 = { false };

            opBoolSI cb = new opBoolSI();
            p.begin_opBoolS(bsi1, bsi2, cb);
            cb.check();
        }

        {
            final short[] ssi = { 1, 2, 3 };
            final int[] isi = { 5, 6, 7, 8 };
            final long[] lsi = { 10, 30, 20 };

            opShortIntLongSI cb = new opShortIntLongSI();
            p.begin_opShortIntLongS(ssi, isi, lsi, cb);
            cb.check();
        }

        {
            final float[] fsi = { 3.14f, 1.11f };
            final double[] dsi = { 1.1E10, 1.2E10, 1.3E10 };

            opFloatDoubleSI cb = new opFloatDoubleSI();
            p.begin_opFloatDoubleS(fsi, dsi, cb);
            cb.check();
        }

        {
            final String[] ssi1 = { "abc", "de", "fghi" };
            final String[] ssi2 = { "xyz" };

            opStringSI cb = new opStringSI();
            p.begin_opStringS(ssi1, ssi2, cb);
            cb.check();
        }

        {
            final byte[][] bsi1 =
                {
                    { (byte)0x01, (byte)0x11, (byte)0x12 },
                    { (byte)0xff }
                };
            final byte[][] bsi2 =
                {
                    { (byte)0x0e },
                    { (byte)0xf2, (byte)0xf1 }
                };

            opByteSSI cb = new opByteSSI();
            p.begin_opByteSS(bsi1, bsi2, cb);
            cb.check();
        }

        {
            final float[][] fsi =
                {
                    { 3.14f },
                    { 1.11f },
                    { },
                };
            final double[][] dsi =
                {
                    { 1.1E10, 1.2E10, 1.3E10 }
                };

            opFloatDoubleSSI cb = new opFloatDoubleSSI();
            p.begin_opFloatDoubleSS(fsi, dsi, cb);
            cb.check();
        }

        {
            final String[][] ssi1 =
                {
                    { "abc" },
                    { "de", "fghi" }
                };
            final String[][] ssi2 =
                {
                    { },
                    { },
                    { "xyz" }
                };

            opStringSSI cb = new opStringSSI();
            p.begin_opStringSS(ssi1, ssi2, cb);
            cb.check();
        }

        {
            final String[][][] sssi1 =
            {
                {
                    {
                        "abc", "de"
                    },
                    {
                        "xyz"
                    }
                },
                {
                    {
                        "hello"
                    }
                }
            };

            final String[][][] sssi2 =
            {
                {
                    {
                        "", ""
                    },
                    {
                        "abcd"
                    }
                },
                {
                    {
                        ""
                    }
                },
                {
                }
            };

            opStringSSSI cb = new opStringSSSI();
            p.begin_opStringSSS(sssi1, sssi2, cb);
            cb.check();
        }

        {
            java.util.Map<Byte, Boolean> di1 = new java.util.HashMap<Byte, Boolean>();
            di1.put((byte)10, Boolean.TRUE);
            di1.put((byte)100, Boolean.FALSE);
            java.util.Map<Byte, Boolean> di2 = new java.util.HashMap<Byte, Boolean>();
            di2.put((byte)10, Boolean.TRUE);
            di2.put((byte)11, Boolean.FALSE);
            di2.put((byte)101, Boolean.TRUE);

            opByteBoolDI cb = new opByteBoolDI();
            p.begin_opByteBoolD(di1, di2, cb);
            cb.check();
        }

        {
            java.util.Map<Short, Integer> di1 = new java.util.HashMap<Short, Integer>();
            di1.put((short)110, -1);
            di1.put((short)1100, 123123);
            java.util.Map<Short, Integer> di2 = new java.util.HashMap<Short, Integer>();
            di2.put((short)110, -1);
            di2.put((short)111, -100);
            di2.put((short)1101, 0);

            opShortIntDI cb = new opShortIntDI();
            p.begin_opShortIntD(di1, di2, cb);
            cb.check();
        }

        {
            java.util.Map<Long, Float> di1 = new java.util.HashMap<Long, Float>();
            di1.put(999999110L, new Float(-1.1f));
            di1.put(999999111L, new Float(123123.2f));
            java.util.Map<Long, Float> di2 = new java.util.HashMap<Long, Float>();
            di2.put(999999110L, new Float(-1.1f));
            di2.put(999999120L, new Float(-100.4f));
            di2.put(999999130L, new Float(0.5f));

            opLongFloatDI cb = new opLongFloatDI();
            p.begin_opLongFloatD(di1, di2, cb);
            cb.check();
        }

        {
            java.util.Map<String, String> di1 = new java.util.HashMap<String, String>();
            di1.put("foo", "abc -1.1");
            di1.put("bar", "abc 123123.2");
            java.util.Map<String, String> di2 = new java.util.HashMap<String, String>();
            di2.put("foo", "abc -1.1");
            di2.put("FOO", "abc -100.4");
            di2.put("BAR", "abc 0.5");

            opStringStringDI cb = new opStringStringDI();
            p.begin_opStringStringD(di1, di2, cb);
            cb.check();
        }

        {
            java.util.Map<String, MyEnum> di1 = new java.util.HashMap<String, MyEnum>();
            di1.put("abc", MyEnum.enum1);
            di1.put("", MyEnum.enum2);
            java.util.Map<String, MyEnum> di2 = new java.util.HashMap<String, MyEnum>();
            di2.put("abc", MyEnum.enum1);
            di2.put("qwerty", MyEnum.enum3);
            di2.put("Hello!!", MyEnum.enum2);

            opStringMyEnumDI cb = new opStringMyEnumDI();
            p.begin_opStringMyEnumD(di1, di2, cb);
            cb.check();
        }

        {
            java.util.Map<MyEnum, String> di1 = new java.util.HashMap<MyEnum, String>();
            di1.put(MyEnum.enum1, "abc");
            di1.put(MyEnum.enum2, "");
            java.util.Map<MyEnum, String> di2 = new java.util.HashMap<MyEnum, String>();
            di2.put(MyEnum.enum1, "abc");
            di2.put(MyEnum.enum2, "Hello!!");
            di2.put(MyEnum.enum3, "qwerty");

            opMyEnumStringDI cb = new opMyEnumStringDI();
            p.begin_opMyEnumStringD(di1, di2, cb);
            cb.check();
        }

        {
            MyStruct s11 = new MyStruct(1, 1);
            MyStruct s12 = new MyStruct(1, 2);
            java.util.Map<MyStruct, MyEnum> di1 = new java.util.HashMap<MyStruct, MyEnum>();
            di1.put(s11, MyEnum.enum1);
            di1.put(s12, MyEnum.enum2);
            MyStruct s22 = new MyStruct(2, 2);
            MyStruct s23 = new MyStruct(2, 3);
            java.util.Map<MyStruct, MyEnum> di2 = new java.util.HashMap<MyStruct, MyEnum>();
            di2.put(s11, MyEnum.enum1);
            di2.put(s22, MyEnum.enum3);
            di2.put(s23, MyEnum.enum2);

            opMyStructMyEnumDI cb = new opMyStructMyEnumDI();
            p.begin_opMyStructMyEnumD(di1, di2, cb);
            cb.check();
        }

        {
            int[] lengths = { 0, 1, 2, 126, 127, 128, 129, 253, 254, 255, 256, 257, 1000 };

            for(int l : lengths)
            {
                int[] s = new int[l];
                for(int i = 0; i < s.length; ++i)
                {
                    s[i] = i;
                }
                opIntSI cb = new opIntSI(l);
                p.begin_opIntS(s, cb);
                cb.check();
            }
        }

        {
            java.util.Map<String, String> ctx = new java.util.HashMap<String, String>();
            ctx.put("one", "ONE");
            ctx.put("two", "TWO");
            ctx.put("three", "THREE");
            {
                test(p.ice_getContext().isEmpty());
                Ice.AsyncResult r = p.begin_opContext();
                java.util.Map<String, String> c = p.end_opContext(r);
                test(!c.equals(ctx));
            }
            {
                test(p.ice_getContext().isEmpty());
                Ice.AsyncResult r = p.begin_opContext(ctx);
                java.util.Map<String, String> c = p.end_opContext(r);
                test(c.equals(ctx));
            }
            MyClassPrx p2 = MyClassPrxHelper.checkedCast(p.ice_context(ctx));
            test(p2.ice_getContext().equals(ctx));
            {
                Ice.AsyncResult r = p2.begin_opContext();
                java.util.Map<String, String> c = p2.end_opContext(r);
                test(c.equals(ctx));
            }
            {
                Ice.AsyncResult r = p2.begin_opContext(ctx);
                java.util.Map<String, String> c = p2.end_opContext(r);
                test(c.equals(ctx));
            }
        }

        {
            //
            // Test implicit context propagation
            //

            String[] impls = {"Shared", "PerThread"};
            for(int i = 0; i < 2; i++)
            {
                Ice.InitializationData initData = new Ice.InitializationData();
                initData.properties = communicator.getProperties()._clone();
                initData.properties.setProperty("Ice.ImplicitContext", impls[i]);

                Ice.Communicator ic = app.initialize(initData);

                java.util.Map<String, String> ctx = new java.util.HashMap<String, String>();
                ctx.put("one", "ONE");
                ctx.put("two", "TWO");
                ctx.put("three", "THREE");

                MyClassPrx p3 = MyClassPrxHelper.uncheckedCast(ic.stringToProxy("test:default -p 12010"));

                ic.getImplicitContext().setContext(ctx);
                test(ic.getImplicitContext().getContext().equals(ctx));
                {
                    Ice.AsyncResult r = p3.begin_opContext();
                    java.util.Map<String, String> c = p3.end_opContext(r);
                    test(c.equals(ctx));
                }

                ic.getImplicitContext().put("zero", "ZERO");

                ctx = ic.getImplicitContext().getContext();
                {
                    Ice.AsyncResult r = p3.begin_opContext();
                    java.util.Map<String, String> c = p3.end_opContext(r);
                    test(c.equals(ctx));
                }

                java.util.Map<String, String> prxContext = new java.util.HashMap<String, String>();
                prxContext.put("one", "UN");
                prxContext.put("four", "QUATRE");

                java.util.Map<String, String> combined = new java.util.HashMap<String, String>(ctx);
                combined.putAll(prxContext);
                test(combined.get("one").equals("UN"));

                p3 = MyClassPrxHelper.uncheckedCast(p3.ice_context(prxContext));

                ic.getImplicitContext().setContext(null);
                {
                    Ice.AsyncResult r = p3.begin_opContext();
                    java.util.Map<String, String> c = p3.end_opContext(r);
                    test(c.equals(prxContext));
                }

                ic.getImplicitContext().setContext(ctx);
                {
                    Ice.AsyncResult r = p3.begin_opContext();
                    java.util.Map<String, String> c = p3.end_opContext(r);
                    test(c.equals(combined));
                }

                ic.destroy();
            }
        }

        {
            double d = 1278312346.0 / 13.0;
            double[] ds = new double[5];
            for(int i = 0; i < 5; i++)
            {
                ds[i] = d;
            }
            opDoubleMarshalingI cb = new opDoubleMarshalingI();
            p.begin_opDoubleMarshaling(d, ds, cb);
            cb.check();
        }

        {
            MyDerivedClassPrx derived = MyDerivedClassPrxHelper.checkedCast(p);
            test(derived != null);
            opDerivedI cb = new opDerivedI();
            derived.begin_opDerived(cb);
            cb.check();
        }

        {
            //
            // Check that we can call operations on Object asynchronously.
            //
            {
                Ice.AsyncResult r = p.begin_ice_isA(MyClass.ice_staticId());
                try
                {
                    test(p.end_ice_isA(r));
                }
                catch(RuntimeException ex)
                {
                    test(false);
                }
            }

            {
                IsACallbackUnsafe cb = new IsACallbackUnsafe();
                p.begin_ice_isA(MyClass.ice_staticId(), cb);
                cb.check();
            }

            {
                IsACallbackSafe cb = new IsACallbackSafe();
                p.begin_ice_isA(MyClass.ice_staticId(), cb);
                cb.check();
            }

            {
                Ice.AsyncResult r = p.begin_ice_ping();
                try
                {
                    p.end_ice_ping(r);
                }
                catch(RuntimeException ex)
                {
                    test(false);
                }
            }

            {
                PingCallbackUnsafe cb = new PingCallbackUnsafe();
                p.begin_ice_ping(cb);
                cb.check();
            }

            {
                PingCallbackSafe cb = new PingCallbackSafe();
                p.begin_ice_ping(cb);
                cb.check();
            }

            {
                Ice.AsyncResult r = p.begin_ice_ids();
                try
                {
                    test(java.util.Arrays.equals(p.end_ice_ids(r), p.ice_ids()));
                }
                catch(RuntimeException ex)
                {
                    test(false);
                }
            }

            {
                IdsCallbackUnsafe cb = new IdsCallbackUnsafe();
                p.begin_ice_ids(cb);
                cb.check();
            }

            {
                IdsCallbackSafe cb = new IdsCallbackSafe(p.ice_ids());
                p.begin_ice_ids(cb);
                cb.check();
            }

            {
                Ice.AsyncResult r = p.begin_ice_id();
                try
                {
                    test(p.end_ice_id(r).equals(p.ice_id()));
                }
                catch(RuntimeException ex)
                {
                    test(false);
                }
            }

            {
                IdCallbackUnsafe cb = new IdCallbackUnsafe();
                p.begin_ice_id(cb);
                cb.check();
            }

            {
                IdCallbackSafe cb = new IdCallbackSafe(p.ice_id());
                p.begin_ice_id(cb);
                cb.check();
            }
        }

        //
        // Test that queuing indication works.
        //
        {
            Ice.AsyncResult r = p.begin_delay(100);
            test(!r.isCompleted());
            p.end_delay(r);
            test(r.isCompleted());
        }

        //
        // Put the server's adapter into the holding state and pump out requests until one is queued.
        // Then activate the adapter again and pump out more until one isn't queued again.
        // Check that all the callbacks arrive after calling the end_ method for each request.
        // We fill a context with a few kB of data to make sure we don't queue up too many requests.
        //
        {
            final int contextSize = 10; // Kilobytes
            StringBuilder s = new StringBuilder();
            for(int i = 0; i < 1024; ++i)
            {
                s.append('a');
            }

            java.util.Map<String, String> ctx = new java.util.HashMap<String, String>();
            for(int i = 0; i < contextSize; ++i)
            {
                ctx.put("" + i, s.toString());
            }

            //
            // Keep all the AsyncResults we get from the begin_ calls, so we can call end_ for each of them.
            //
            java.util.ArrayList<Ice.AsyncResult> results = new java.util.ArrayList<Ice.AsyncResult>();

            int queuedCount = 0;

            SentCounter cb = new SentCounter();
            Ice.AsyncResult r;

            StateChangerPrx state =
                StateChangerPrxHelper.checkedCast(communicator.stringToProxy("hold:default -p 12011"));
            state.hold(3);

            do
            {
                r = p.begin_opVoid(ctx, cb);
                results.add(r);
                if(!r.sentSynchronously())
                {
                    ++queuedCount;
                }
            }
            while(r.sentSynchronously());

            int numRequests = results.size();
            test(numRequests > 1); // Something is wrong if we didn't get something out without queueing.

            //
            // Re-enable the adapter.
            //
            state.activate(3);

            //
            // Fire off a bunch more requests until we get one that wasn't queued.
            // We sleep in between calls to allow the queued requests to drain.
            //
            do
            {
                r = p.begin_opVoid(cb);
                results.add(r);
                if(!r.sentSynchronously())
                {
                    ++queuedCount;
                }
                try
                {
                    Thread.sleep(1);
                }
                catch(InterruptedException ex)
                {
                }
            }
            while(!r.sentSynchronously());
            test(results.size() > numRequests); // Something is wrong if we didn't queue additional requests.

            //
            // Now make all the outstanding calls to the end_ method.
            //
            for(Ice.AsyncResult ar : results)
            {
                p.end_opVoid(ar);
            }

            //
            // Check that all the callbacks have arrived and that we got a sent callback for each queued request.
            //
            cb.check(results.size());
            test(cb.queuedCount() == queuedCount);
        }
    }
}
