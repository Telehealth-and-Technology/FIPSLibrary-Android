/*
 * This file is auto-generated.  DO NOT MODIFY.
 * Original file: /Users/scoleman/dev/fips/fcads/dev/android-database-sqlcipher/src/net/sqlcipher/IContentObserver.aidl
 */
package net.sqlcipher;
/**
 * @hide
 */
public interface IContentObserver extends android.os.IInterface
{
/** Local-side IPC implementation stub class. */
public static abstract class Stub extends android.os.Binder implements net.sqlcipher.IContentObserver
{
private static final java.lang.String DESCRIPTOR = "net.sqlcipher.IContentObserver";
/** Construct the stub at attach it to the interface. */
public Stub()
{
this.attachInterface(this, DESCRIPTOR);
}
/**
 * Cast an IBinder object into an net.sqlcipher.IContentObserver interface,
 * generating a proxy if needed.
 */
public static net.sqlcipher.IContentObserver asInterface(android.os.IBinder obj)
{
if ((obj==null)) {
return null;
}
android.os.IInterface iin = obj.queryLocalInterface(DESCRIPTOR);
if (((iin!=null)&&(iin instanceof net.sqlcipher.IContentObserver))) {
return ((net.sqlcipher.IContentObserver)iin);
}
return new net.sqlcipher.IContentObserver.Stub.Proxy(obj);
}
@Override public android.os.IBinder asBinder()
{
return this;
}
@Override public boolean onTransact(int code, android.os.Parcel data, android.os.Parcel reply, int flags) throws android.os.RemoteException
{
switch (code)
{
case INTERFACE_TRANSACTION:
{
reply.writeString(DESCRIPTOR);
return true;
}
case TRANSACTION_onChange:
{
data.enforceInterface(DESCRIPTOR);
boolean _arg0;
_arg0 = (0!=data.readInt());
this.onChange(_arg0);
return true;
}
}
return super.onTransact(code, data, reply, flags);
}
private static class Proxy implements net.sqlcipher.IContentObserver
{
private android.os.IBinder mRemote;
Proxy(android.os.IBinder remote)
{
mRemote = remote;
}
@Override public android.os.IBinder asBinder()
{
return mRemote;
}
public java.lang.String getInterfaceDescriptor()
{
return DESCRIPTOR;
}
/**
     * This method is called when an update occurs to the cursor that is being
     * observed. selfUpdate is true if the update was caused by a call to
     * commit on the cursor that is being observed.
     */
@Override public void onChange(boolean selfUpdate) throws android.os.RemoteException
{
android.os.Parcel _data = android.os.Parcel.obtain();
try {
_data.writeInterfaceToken(DESCRIPTOR);
_data.writeInt(((selfUpdate)?(1):(0)));
mRemote.transact(Stub.TRANSACTION_onChange, _data, null, android.os.IBinder.FLAG_ONEWAY);
}
finally {
_data.recycle();
}
}
}
static final int TRANSACTION_onChange = (android.os.IBinder.FIRST_CALL_TRANSACTION + 0);
}
/**
     * This method is called when an update occurs to the cursor that is being
     * observed. selfUpdate is true if the update was caused by a call to
     * commit on the cursor that is being observed.
     */
public void onChange(boolean selfUpdate) throws android.os.RemoteException;
}
