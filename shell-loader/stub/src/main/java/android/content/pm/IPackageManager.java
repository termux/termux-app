package android.content.pm;

public interface IPackageManager {
    PackageInfo getPackageInfo(String packageName, long flags, int userId) throws android.os.RemoteException;
    PackageInfo getPackageInfo(String packageName, int flags, int userId) throws android.os.RemoteException;
    String[] getPackagesForUid(int uid) throws android.os.RemoteException;
}