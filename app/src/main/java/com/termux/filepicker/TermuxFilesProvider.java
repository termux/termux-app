package com.termux.filepicker;

import static android.provider.DocumentsContract.Document.MIME_TYPE_DIR;
import static android.system.OsConstants.S_IFLNK;
import static android.system.OsConstants.S_IFMT;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.ProviderInfo;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.os.Bundle;
import android.os.CancellationSignal;
import android.os.ParcelFileDescriptor;
import android.provider.DocumentsContract;
import android.provider.DocumentsContract.Document;
import android.provider.DocumentsContract.Root;
import android.provider.DocumentsProvider;
import android.system.ErrnoException;
import android.system.Os;
import android.system.StructStat;
import android.webkit.MimeTypeMap;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.List;
import java.util.Objects;

/*
 come from https://github.com/ewt45/winlator-fork/blob/extra2/app/src/main/java/bin/mt/file/content/MTDataFilesProvider.java
mt 2.15.2 的 文件提供器
开头的uri： content:// + manifest里provider的authorities（MTDataFilesProvider）+ /tree/ + 包名
创建文件：父文件id+路径分隔符（可选）+显示名（文件名？）+防冲突序号（可选）
将根目录的documentId设置为app包名

如果getFileForDocId发现docId是包名，则返回null，表明这个是虚拟根目录。然后includeFile里添加这个虚拟根目录的子目录时，应该显示那几个应用专属目录
|-com.example   对应文件路径：无
|--data         对应文件路径：/data/data/com.example
|---files       对应文件路径：/data/data/com.example/files
第三级目录开始，都是对应其实际的文件名。所以只需要assert第一级目录是包名，然后将第二级目录替换成对应的真实文件目录（这几个会存在成员变量中）。再往后只需要添加文件名即可

includeFile里返回虚拟根目录的Cursor时，display name变成包名了（不知道为啥mt注入的是正确显示应用名的，手动改一下就行了，termux应该也是改这里）
 */
public class TermuxFilesProvider extends DocumentsProvider {
//    private static final String TAG = "MTDataFilesProvider";
    private static final String[] g_DEFAULT_ROOT_PROJECTION = {
            Root.COLUMN_ROOT_ID,
            Root.COLUMN_MIME_TYPES,
            Root.COLUMN_FLAGS,
            Root.COLUMN_ICON,
            Root.COLUMN_TITLE,
            Root.COLUMN_SUMMARY,
            Root.COLUMN_DOCUMENT_ID
    };
    // The default columns to return information about a document if no specific
    // columns are requested in a query.
    private static final String[] h_DEFAULT_DOCUMENT_PROJECTION = {
            Document.COLUMN_DOCUMENT_ID,
            Document.COLUMN_MIME_TYPE,
            Document.COLUMN_DISPLAY_NAME,
            Document.COLUMN_LAST_MODIFIED,
            Document.COLUMN_FLAGS,
            Document.COLUMN_SIZE,
            "mt_extras"
    };
    /**
     * 应用的包名
     */
    private String b_pkgName;
    /**
     * 应用专有目录根目录，例如 /data/data/包名
     */
    private File c_dataDir;
    /**
     * 类似 {@link #c_dataDir}
     * 如果privateRootDir路径以/data/user开头，那么就会实例化这个变量，开头换成/data/user_de/
     */
    private File d_user_deDataDir;
    /**
     * 外部存储目录下的应用专有目录，例如：/storage/emulated/0/Android/data/包名
     */
    private File e_androidDataDir;
    private File f_androidObbDir;

    /**
     * 删除文件或文件夹内全部内容。符号链接的话当做文件直接删除
     */
    private static boolean a_deleteFileOrDirectory(File file) {
        if (file.isDirectory()) {
            boolean isSymlink = false; //符号链接的话当做文件直接删除
            try {
                isSymlink = (Os.lstat(file.getPath()).st_mode & S_IFMT/*8进制0170000*/) == S_IFLNK/*8进制0120000*/; //文件类型是否为符号链接
            } catch (ErrnoException e) {
                e.printStackTrace();
            }

            File[] subFiles = file.listFiles();
            if (!isSymlink && subFiles != null)
                for (File sub : subFiles)
                    if (!a_deleteFileOrDirectory(sub))
                        return false;
        }
        return file.delete();
    }

    private static String c_getMimeType(File file) {
        if (file.isDirectory())
            return MIME_TYPE_DIR;

        String name = file.getName();
        int lastDot = name.lastIndexOf('.');
        if (lastDot >= 0) {
            String extension = name.substring(lastDot + 1).toLowerCase();
            String mime = MimeTypeMap.getSingleton().getMimeTypeFromExtension(extension);
            if (mime != null) return mime;
        }
        return "application/octet-stream";
    }

    @SuppressLint("SdCardPath")
    @Override // android.provider.DocumentsProvider, android.content.ContentProvider
    public final void attachInfo(Context context, ProviderInfo info) {
        super.attachInfo(context, info);
//        Log.d(TAG, "attachInfo会在onCreate之后吗？attachInfo: ");
        this.b_pkgName = context.getPackageName();
        this.c_dataDir = Objects.requireNonNull(context.getFilesDir().getParentFile());
        String path = c_dataDir.getPath();
        if (path.startsWith("/data/user/"))
            this.d_user_deDataDir = new File("/data/user_de/" + path.substring("/data/user/".length()));
        File externalFilesDir = context.getExternalFilesDir(null);
        if (externalFilesDir != null)
            this.e_androidDataDir = externalFilesDir.getParentFile();
        this.f_androidObbDir = context.getObbDir();
    }

    /**
     * 通过documentId获取对应的File，可能为null
     *
     * @param documentId documentId, 一般是父函数传过来
     * @param z          ？
     * @return 对应文件。可能为null。为null时说明这个是根目录，其documentId为应用包名，其子目录应该是那几个应用专属目录
     */
    private final File b_getFileForDocId(String documentId, boolean z) throws FileNotFoundException {
        String realPath;
        if (!documentId.startsWith(this.b_pkgName))
            throw new FileNotFoundException(documentId.concat(" not found"));

        //根目录下那几个虚拟文件夹，表示应用专属目录的
        String virtualName = documentId.substring(this.b_pkgName.length());
        if (virtualName.startsWith("/")) {
            virtualName = virtualName.substring(1);
        }
        //如果是根目录自身，直接返回null
        if (virtualName.isEmpty()) {
            return null;
        }
        String[] split = virtualName.split("/", 2);
        virtualName = split[0];
        realPath = split.length > 1 ? split[1] : "";

        File targetFile;
        if (virtualName.equalsIgnoreCase("data")) {
            targetFile = new File(this.c_dataDir, realPath);
        } else if (virtualName.equalsIgnoreCase("android_data") && this.e_androidDataDir != null) {
            targetFile = new File(this.e_androidDataDir, realPath);
        } else if (virtualName.equalsIgnoreCase("android_obb") && this.f_androidObbDir != null) {
            targetFile = new File(this.f_androidObbDir, realPath);
        } else if (virtualName.equalsIgnoreCase("user_de_data") && this.d_user_deDataDir != null) {
            targetFile = new File(this.d_user_deDataDir, realPath);
        } else
            throw new FileNotFoundException(documentId.concat(" not found"));

        if (z) {
            try {
                Os.lstat(targetFile.getPath());
            } catch (Exception unused) {
                throw new FileNotFoundException(documentId.concat(" not found"));
            }
        }
        return targetFile;
    }

    @Override // android.provider.DocumentsProvider, android.content.ContentProvider
    public final Bundle call(String method, String arg, Bundle extras) {
        Bundle call = super.call(method, arg, extras);
        if (call != null)
            return call;

        if (!method.startsWith("mt:"))
            return null;

        Bundle customBundle = new Bundle();
        customBundle.putBoolean("result", false);
        try {
            List<String> pathSegments = ((Uri) extras.getParcelable("uri")).getPathSegments();
            String documentId = pathSegments.size() >= 4 ? pathSegments.get(3) : pathSegments.get(1);
            switch (method) {
                case "mt:setPermissions": { //-1645162251
                    File file = b_getFileForDocId(documentId, true);
                    if (file != null) {
                        Os.chmod(file.getPath(), extras.getInt("permissions"));
                        customBundle.putBoolean("result", true);
                    }
                    return customBundle;

                }
                case "mt:createSymlink": {
                    File file = b_getFileForDocId(documentId, false);
                    if (file != null) {
                        Os.symlink(extras.getString("path"), file.getPath());
                        customBundle.putBoolean("result", true);
                    }
                    return customBundle;
                }
                case "mt:setLastModified": {
                    File file = b_getFileForDocId(documentId, true);
                    if (file != null)
                        customBundle.putBoolean("result", file.setLastModified(extras.getLong("time")));
                    return customBundle;
                }
                default:
                    throw new RuntimeException("Unsupported method: ".concat(method));
            }
        } catch (Exception e) {
            customBundle.putBoolean("result", false);
            customBundle.putString("message", e.toString());
            return customBundle;
        }
    }

    @Override // android.provider.DocumentsProvider
    public final String createDocument(String parentDocumentId, String mimeType, String displayName) throws FileNotFoundException {
        //父文件id+路径分隔符（可选）+显示名+防冲突序号（可选）
        File parentFile = b_getFileForDocId(parentDocumentId, true);
        if (parentFile != null) {
            File newFile = new File(parentFile, displayName);
            int noConflictId = 2;
            while (newFile.exists()) {
                newFile = new File(parentFile, displayName + " (" + noConflictId + ")");
                noConflictId++;
            }
            try {
                boolean succeeded = MIME_TYPE_DIR.equals(mimeType)
                        ? newFile.mkdir()
                        : newFile.createNewFile();

                if (succeeded) {
                    return parentDocumentId +
                            (parentDocumentId.endsWith("/") ? "" : "/") +
                            newFile.getName();
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        throw new FileNotFoundException("Failed to create document in " + parentDocumentId + " with name " + displayName);
    }

    /**
     * Add a representation of a file to a cursor.
     *
     * @param result the cursor to modify
     * @param docId  the document ID representing the desired file (不能为null)
     * @param file   the File object representing the desired file (may be null if given docID)
     */
    private void d_includeFile(MatrixCursor result, String docId, File file) throws FileNotFoundException {
        //v1, 0x1
        if (file == null) {
            file = b_getFileForDocId(docId, true);
        }
        //是否为普通文件（不是那几个data的链接）
        boolean isNormalFile = false;
        //没有对应docId的文件，就将其id设为包名？在b_getFileForDocId函数里默认全部docId都是以包名开头的，所以这个是根目录？
        if (file == null) {
            Context ctx = getContext();
            String title = ctx == null ? "ctx=null?!" : ctx.getApplicationInfo().loadLabel(getContext().getPackageManager()).toString();

            MatrixCursor.RowBuilder row = result.newRow();
            row.add(Document.COLUMN_DOCUMENT_ID, this.b_pkgName);
            row.add(Document.COLUMN_DISPLAY_NAME, title); //为啥mt注入的虚拟根目录会正常显示应用名，但代码里用的包名
            row.add(Document.COLUMN_SIZE, 0);
            row.add(Document.COLUMN_MIME_TYPE, MIME_TYPE_DIR);
            row.add(Document.COLUMN_LAST_MODIFIED, 0);
            row.add(Document.COLUMN_FLAGS, 0);
            return;
        }

        int flags = 0;
        if (file.isDirectory()) {
            if (file.canWrite())
                flags |= Document.FLAG_DIR_SUPPORTS_CREATE;
        } else if (file.canWrite()) {
            flags |= Document.FLAG_SUPPORTS_WRITE;
        }

        if (file.getParentFile().canWrite())
            flags |= Document.FLAG_SUPPORTS_DELETE;

        String displayName;

        //如果是几个特殊的文件夹
        String path = file.getPath();
        if (path.equals(this.c_dataDir.getPath())) {
            displayName = "data";
        } else if (e_androidDataDir != null && path.equals(e_androidDataDir.getPath())) {
            displayName = "android_data";
        } else if (f_androidObbDir != null && path.equals(f_androidObbDir.getPath())) {
            displayName = "android_obb";
        } else if (d_user_deDataDir != null && path.equals(d_user_deDataDir.getPath())) {
            displayName = "user_de_data";
        } else {
            displayName = file.getName();
            isNormalFile = true;
        }
        MatrixCursor.RowBuilder row = result.newRow();
        row.add(DocumentsContract.Document.COLUMN_DOCUMENT_ID, docId);
        row.add(DocumentsContract.Document.COLUMN_DISPLAY_NAME, displayName);
        row.add(DocumentsContract.Document.COLUMN_SIZE, file.length());
        row.add(DocumentsContract.Document.COLUMN_MIME_TYPE, c_getMimeType(file));
        row.add(DocumentsContract.Document.COLUMN_LAST_MODIFIED, file.lastModified());
        row.add(DocumentsContract.Document.COLUMN_FLAGS, flags);
        row.add("mt_path", file.getAbsolutePath());
        if (isNormalFile) {
            try {
                StructStat lstat = Os.lstat(path);
                StringBuilder sb = new StringBuilder()
                        .append(lstat.st_mode)
                        .append("|").append(lstat.st_uid)
                        .append("|").append(lstat.st_gid);
                if ((lstat.st_mode & S_IFMT) == S_IFLNK) {
                    sb.append("|").append(Os.readlink(path));
                }
                row.add("mt_extras", sb.toString());
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    @Override // android.provider.DocumentsProvider
    public final void deleteDocument(String documentId) throws FileNotFoundException {
        File file = b_getFileForDocId(documentId, true);
        if (file == null || !a_deleteFileOrDirectory(file))
            throw new FileNotFoundException("Failed to delete document ".concat(documentId));
    }

    @Override // android.provider.DocumentsProvider
    public final String getDocumentType(String documentId) throws FileNotFoundException {
        File file = b_getFileForDocId(documentId, true);
        return file == null ? MIME_TYPE_DIR : c_getMimeType(file);
    }

    @Override // android.provider.DocumentsProvider
    public final boolean isChildDocument(String parentDocumentId, String documentId) {
        return documentId.startsWith(parentDocumentId);
    }

    @Override // android.provider.DocumentsProvider
    public final String moveDocument(String sourceDocumentId, String sourceParentDocumentId, String targetParentDocumentId) throws FileNotFoundException {
        File sourceFile = b_getFileForDocId(sourceDocumentId, true);
        File targetParentFile = b_getFileForDocId(targetParentDocumentId, true);
        if (sourceFile != null && targetParentFile != null) {
            File targetFile = new File(targetParentFile, sourceFile.getName());
            if (!targetFile.exists() && sourceFile.renameTo(targetFile)) {
                return targetParentDocumentId
                        + (targetParentDocumentId.endsWith("/") ? "" : "/")
                        + targetFile.getName();
            }
        }
        throw new FileNotFoundException("Failed to move document " + sourceDocumentId + " to " + targetParentDocumentId);
    }

    @Override // android.content.ContentProvider
    public final boolean onCreate() {
//        Log.d(TAG, "attachInfo会在onCreate之后吗？onCreate: ");
        return true;
    }

    @Override // android.provider.DocumentsProvider
    public final ParcelFileDescriptor openDocument(String documentId, String mode, CancellationSignal cancellationSignal) throws FileNotFoundException {
        File file = b_getFileForDocId(documentId, false);
        if (file != null)
            return ParcelFileDescriptor.open(file, ParcelFileDescriptor.parseMode(mode));
        else
            throw new FileNotFoundException(documentId + " not found");
    }

    @Override // android.provider.DocumentsProvider
    public final Cursor queryChildDocuments(String parentDocumentId, String[] projection, String sortOrder) throws FileNotFoundException {
        if (parentDocumentId.endsWith("/")) {
            parentDocumentId = parentDocumentId.substring(0, parentDocumentId.length() - 1);
        }
        MatrixCursor cursor = new MatrixCursor(projection != null ? projection : h_DEFAULT_DOCUMENT_PROJECTION);
        File parent = b_getFileForDocId(parentDocumentId, true);
        //如果是虚拟根目录，则显示那几个应用专属目录
        if (parent == null) {
            d_includeFile(cursor, parentDocumentId.concat("/data"), this.c_dataDir);

            if (e_androidDataDir != null && e_androidDataDir.exists())
                d_includeFile(cursor, parentDocumentId.concat("/android_data"), this.e_androidDataDir);

            if (f_androidObbDir != null && f_androidObbDir.exists())
                d_includeFile(cursor, parentDocumentId.concat("/android_obb"), this.f_androidObbDir);

            if (d_user_deDataDir != null && d_user_deDataDir.exists())
                d_includeFile(cursor, parentDocumentId.concat("/user_de_data"), this.d_user_deDataDir);
        }
        //否则正常处理
        else {
            File[] children = parent.listFiles();
            if (children != null)
                for (File child : children)
                    d_includeFile(cursor, parentDocumentId + "/" + child.getName(), child);
        }
        return cursor;
    }

    @Override // android.provider.DocumentsProvider
    public final Cursor queryDocument(String documentId, String[] projection) throws FileNotFoundException {
        MatrixCursor result = new MatrixCursor(projection != null ? projection : h_DEFAULT_DOCUMENT_PROJECTION);
        d_includeFile(result, documentId, null);
        return result;
    }

    @Override // android.provider.DocumentsProvider
    public final Cursor queryRoots(String[] projection) {
        ApplicationInfo appInfo = Objects.requireNonNull(getContext()).getApplicationInfo();
        //虚拟根目录的标题 从ApplicationInfo获取label
        String title = appInfo.loadLabel(getContext().getPackageManager()).toString();
//        Log.d(TAG, "queryRoots: 获取不到应用名？为什么会变成uri的最后一段"+title);

        MatrixCursor result = new MatrixCursor(projection != null ? projection : g_DEFAULT_ROOT_PROJECTION);
        MatrixCursor.RowBuilder row = result.newRow();
        row.add(Root.COLUMN_ROOT_ID, this.b_pkgName);
        row.add(Root.COLUMN_DOCUMENT_ID, this.b_pkgName);
        row.add(Root.COLUMN_SUMMARY, this.b_pkgName);
        row.add(Root.COLUMN_FLAGS, Root.FLAG_SUPPORTS_CREATE | Root.FLAG_SUPPORTS_IS_CHILD); //mt的比termux少了一个 DocumentsContract.Root.FLAG_SUPPORTS_SEARCH
        row.add(Root.COLUMN_TITLE, title);
        row.add(Root.COLUMN_MIME_TYPES, "*/*");
        row.add(Root.COLUMN_ICON, appInfo.icon);
        return result;
    }

    /**
     * 比 {@link #deleteDocument(String)} 多了一个 父docId。如果此文件存在多个父文件，则比较有用。
     */
    @Override // android.provider.DocumentsProvider
    public final void removeDocument(String documentId, String parentDocumentId) throws FileNotFoundException {
        deleteDocument(documentId);
    }

    @Override // android.provider.DocumentsProvider
    public final String renameDocument(String documentId, String displayName) throws FileNotFoundException {
        File b = b_getFileForDocId(documentId, true);
        if (b == null || !b.renameTo(new File(b.getParentFile(), displayName))) {
            throw new FileNotFoundException("Failed to rename document " + documentId + " to " + displayName);
        }
        int parentIdx = documentId.lastIndexOf('/', documentId.length() - 2);//因为最后一个字符可能是分隔符，所以从倒数第二个开始找，确保分隔出旧文件名
        return documentId.substring(0, parentIdx) + "/" + displayName;
    }
}
