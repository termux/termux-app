package com.termux.app.packagemanager

import android.app.PendingIntent
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.content.pm.PackageInstaller
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.provider.Settings
import android.text.TextUtils
import android.util.Log
import android.widget.Toast
import com.termux.app.packagemanager.Constants.Companion.PACKAGE_INSTALLED_ACTION
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.launch
import java.io.File
import java.io.FileInputStream
import java.io.IOException
import java.io.OutputStream


class PackageInstaller(val context: Context) : PackageDownloader.ErrorListener, PackageDownloader.ProgressListener, PackageDownloader.StartListener, PackageDownloader.CompleteListener {

    private val TERMUX_CACHE_DIRECTORY = "${context.cacheDir}${Constants.TERMUX_CACHE_PKG_DIRECTORY_SUBFOLDER}"
    private val downloadHashMap: HashMap<String, LocalDownloadData> = hashMapOf()
    private val installationResponseHashMap: HashMap<String, String> = hashMapOf()
    private var packagesToInstall: ArrayList<String> = arrayListOf()
    private val packageDownloader = PackageDownloader(context)
    private var currentPosition = 0
    private var totalLength = 0

    fun initDownloader(packageList: Array<String>) {

        TERMUX_CACHE_DIRECTORY.log()

        if (isInstallationOfApkAllowed()) {
            context.registerReceiver(broadcastReceiver, IntentFilter(PACKAGE_INSTALLED_ACTION))
            packageDownloader.initListeners(this, this, this, this)
            val verifiedPackageList = packageList.removeRepetition()
            verifiedPackageList.forEach { packageName ->
                startDownload(packageName)
            }
        } else {
            GlobalScope.launch(Dispatchers.Main) {
                "Permission Insufficient. Please provide the following permission and rerun the command.".log()
                Toast.makeText(context, "Please allow installation from unknown sources for Termux.", Toast.LENGTH_SHORT).show()
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                    //This weirdly messes up the activity stack
                    val permIntent = Intent(Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES, Uri.parse("package:com.termux"))
                    permIntent.flags = Intent.FLAG_ACTIVITY_NEW_TASK
                    context.startActivity(permIntent)
                }
            }

        }
    }

    private fun startDownload(packageName: String) {
        downloadHashMap[packageName] = LocalDownloadData(packageName, null)
        packageDownloader.download(packageName)
    }

    private fun Array<String>.removeRepetition(): Array<String> {
        return this.toList().distinct().toTypedArray()
    }

    override fun onProgress(data: DownloadData) {
        "${data.packageName} has been ${data.progressPercent}% downloaded.".log()
    }

    override fun onStart(data: DownloadData) {
        "Downloading ${data.packageName}...".log()
    }

    override fun onComplete(data: DownloadData) {
        downloadHashMap[data.packageName] = LocalDownloadData(data.packageName, true, "Successfully downloaded!")
        "Completed downloading ${data.packageName}...".log()
        checkIfAllPackagesAreDownloaded()
    }

    override fun onError(data: ErrorData) {
        downloadHashMap[data.packageName] = LocalDownloadData(data.packageName, true, "download aborted -> ${data.error}")
        "Error downloading ${data.packageName} --> ${data.error}...".log()
        checkIfAllPackagesAreDownloaded()
    }

    private fun checkIfAllPackagesAreDownloaded() {
        var counter = 0
        downloadHashMap.forEach { (_, installData) ->
            if (installData.isDownloaded == null) {
                //packageLeft
                ++counter
            }
        }
        if (counter == 0) {
            endDownloadSession()
            getApkListInFileSystem()
            proceedToInstallation()
        }
    }


    /*---------------------------------------- INSTALLATION------------------------------------------*/


    private fun installAPK(packageName: String) {
        "Proceeding to write $packageName".log()
        if (isInstallationOfApkAllowed()) {

            GlobalScope.launch(Dispatchers.IO) {
                var session: PackageInstaller.Session? = null
                try {
                    val packageInstaller: PackageInstaller = context.packageManager.packageInstaller
                    val params = PackageInstaller.SessionParams(
                        PackageInstaller.SessionParams.MODE_FULL_INSTALL)
                    val sessionId = packageInstaller.createSession(params)
                    session = packageInstaller.openSession(sessionId)
                    addApkToInstallSession(session, packageName)
                    val installBroadcast = PendingIntent.getBroadcast(context, 0, Intent(PACKAGE_INSTALLED_ACTION).putExtra("packageName", packageName), PendingIntent.FLAG_UPDATE_CURRENT)
                    session.commit(installBroadcast.intentSender)
                    session.close()
                } catch (e: IOException) {
                    throw RuntimeException("Couldn't install package", e)
                } catch (e: RuntimeException) {
                    session?.abandon()
                    throw e
                } finally {
                    session?.close()
                }
            }
        } else {
            GlobalScope.launch(Dispatchers.Main) {
                "Permission Insufficient. Please provide the following permission and rerun the command.".log()
                Toast.makeText(context, "Please allow installation from unknown sources for Termux.", Toast.LENGTH_SHORT).show()
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                    context.startActivity(Intent(Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES, Uri.parse("package:com.termux")))
                }
            }

        }
    }

    private fun addApkToInstallSession(session: PackageInstaller.Session,
                                       packageName: String) {
        val file = File("$TERMUX_CACHE_DIRECTORY/$packageName.apk")
        val packageInSession: OutputStream = session.openWrite(packageName, 0, -1)
        val inputStream = FileInputStream(file)
        try {
            var c: Int
            val buffer = ByteArray(16384)
            while (inputStream.read(buffer).also { c = it } >= 0) {
                packageInSession.write(buffer, 0, c)
            }
        } catch (e: IOException) {
            "IOEX".log()
        } finally {
            try {
                packageInSession.close()
                inputStream.close()
            } catch (e: IOException) {
                ("IOEX in closing the stream").log()
            }
        }
    }

    private val broadcastReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            val extras = intent.extras
            val status = extras!!.getInt(PackageInstaller.EXTRA_STATUS)
            val message = extras.getString(PackageInstaller.EXTRA_STATUS_MESSAGE)
            val packageName = extras.getString("packageName")!!
            if (PACKAGE_INSTALLED_ACTION == intent.action) {
                when (status) {
                    PackageInstaller.STATUS_PENDING_USER_ACTION -> {
                        // This test app isn't privileged, so the user has to confirm the install.
                        val confirmIntent = extras[Intent.EXTRA_INTENT] as Intent
                        confirmIntent.flags = Intent.FLAG_ACTIVITY_NEW_TASK
                        context.startActivity(confirmIntent)
                    }
                    PackageInstaller.STATUS_SUCCESS -> {
                        Toast.makeText(context, "Install succeeded!", Toast.LENGTH_LONG).show()
                        ("$packageName Install succeeded!").log()
                        installationResponseHashMap[packageName] = "installation successful!"
                        proceedToInstallation(true)
                    }
                    PackageInstaller.STATUS_FAILURE, PackageInstaller.STATUS_FAILURE_ABORTED, PackageInstaller.STATUS_FAILURE_BLOCKED, PackageInstaller.STATUS_FAILURE_CONFLICT, PackageInstaller.STATUS_FAILURE_INCOMPATIBLE, PackageInstaller.STATUS_FAILURE_INVALID, PackageInstaller.STATUS_FAILURE_STORAGE -> {
                        Toast.makeText(context, "Install failed! $status, $message",
                            Toast.LENGTH_LONG).show()
                        ("$packageName Install failed!").log()
                        //can separate cases if that's important
                        installationResponseHashMap[packageName] = "installation failed! | $message"
                        proceedToInstallation(true)
                    }
                    else -> {
                        ("$packageName Unrecognized status received from installer: $status").log()
                        Toast.makeText(context, "Unrecognized status received from installer: $status",
                            Toast.LENGTH_LONG).show()
                        installationResponseHashMap[packageName] = "installation failed! | $message"
                        proceedToInstallation(true)
                        //  exitActivity("Package failed to install -> Unknown Error!")
                    }
                }
            }
        }

    }

    private fun proceedToInstallation(next: Boolean = false) {

        if (!next) {
            if (packagesToInstall.isEmpty()) {
                endInstallationSession()
            } else {
                totalLength = packagesToInstall.size - 1
                installAPK(packagesToInstall[currentPosition])
            }
        } else {
            if (currentPosition == totalLength) {
                endInstallationSession()
            } else {
                installAPK(packagesToInstall[++currentPosition])
            }
        }
    }

    private fun getApkListInFileSystem() {
        downloadHashMap.forEach { (packageName) ->
            //Setting up a default response
            installationResponseHashMap[packageName] = "the request package was either not downloaded or just doesn't exist!"
            val apkFileToBeInstalled = File("$TERMUX_CACHE_DIRECTORY/$packageName.apk")
            if (apkFileToBeInstalled.exists()) {
                packagesToInstall.add(packageName)
            }
        }
    }


    private fun endDownloadSession() {
        "DOWNLOADS COMPLETED".log()
        "Here are the logs...".log()
        downloadHashMap.forEach { (packageName, installData) ->
            "$packageName -> ${installData.extraLogs}".log()
        }

    }

    private fun endInstallationSession() {
        "INSTALLATION COMPLETED".log()
        "Here are the logs...".log()
        installationResponseHashMap.forEach { (packageName, response) ->
            "$packageName -> $response".log()
        }
        context.unregisterReceiver(broadcastReceiver)
        this.packagesToInstall.clear()
        this.installationResponseHashMap.clear()
        this.downloadHashMap.clear()
    }

    private fun isInstallationOfApkAllowed(): Boolean {
        val packageManager = context.packageManager
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            packageManager.canRequestPackageInstalls()
        } else
            true
    }

    companion object {
        private const val TERMUX_PACKAGE_TAG = "Termux Package Management"
        fun Any.log() {
            Log.i(TERMUX_PACKAGE_TAG, this.toString())
        }
    }

    /*--------------------------------------- Play Store Download -----------------------------------------*/

    fun downloadFromPlayStore(packageList: Array<String>) {
        if (isGooglePlayPresent()) {
            "Google Play Store Present".log()
            context.startActivity(Intent(context, GooglePlayTransparentActivity::class.java).setFlags(Intent.FLAG_ACTIVITY_NEW_TASK).putExtra("packages", packageList))
        } else {
            //Falling back to the external repository for downloads
            initDownloader(packageList)
        }
    }

    private fun isGooglePlayPresent(): Boolean {
        return try {
            val info = context.packageManager.getPackageInfo("com.android.vending", PackageManager.GET_ACTIVITIES)
            val label = info.applicationInfo.loadLabel(context.packageManager) as String
            TextUtils.isEmpty(label) && label.startsWith("Google Play")
        } catch (e: PackageManager.NameNotFoundException) {
            false
        }
    }
}

data class LocalDownloadData(var packageName: String, var isDownloaded: Boolean?, var extraLogs: String = "")

