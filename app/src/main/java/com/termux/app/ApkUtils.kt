package com.termux.app

import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.util.Log
import com.downloader.*
import com.termux.terminal.EmulatorDebug
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.launch
import java.io.File


const val TERMUX_APK_SUFFIX = "net.termux."

open class ApkUtils(val context: Context) : OnStartOrResumeListener, OnCancelListener, OnProgressListener, OnDownloadListener {


    /*
    * Class ApkUtils provides a helper functions for the package management with APK files
    * */

    /*
    * @param downloadHashMap  Holds the termux package name and the dataModel of PackageDownloadData
    * @data class PackageDownloadData  Holds the termux package name, download ID and other data associated with the package installation including if the download was successful.
    * @param totalPackages  Holds the total number of packages to be downloaded
    * */
    private val downloadHashMap: HashMap<String, PackageDownloadData> = hashMapOf()
    private var totalPackages = 0

    fun downloadAPK(context: Context, packages: Array<String>) {
        val config = PRDownloaderConfig.newBuilder()
            .setReadTimeout(50_000)
            .build()

        PRDownloader.initialize(context, config)

        GlobalScope.launch(Dispatchers.IO) {
            totalPackages = packages.size
            Log.i(EmulatorDebug.LOG_TAG, "$totalPackages are requested to download...")

            packages.forEach { packageName ->

                val path = "${TermuxService.FILES_PATH}/"
                val url = "https://termux.net/apks/$packageName.apk"

                val downloadId = PRDownloader.download(url, path, "$packageName.apk")
                    .build()
                    .setOnStartOrResumeListener(this@ApkUtils)
                    .setOnCancelListener(this@ApkUtils)
                    .setOnProgressListener(this@ApkUtils)
                    .start(this@ApkUtils)

                downloadHashMap[packageName] = PackageDownloadData(downloadId, null)
            }
        }
    }

    override fun onStartOrResume() {
        downloadHashMap.forEach { (packageName, downloadData) ->
            if (Status.RUNNING == PRDownloader.getStatus(downloadData.downloadID)) {
                Log.i(EmulatorDebug.LOG_TAG, "Downloading ${packageName}...")
            }
        }
    }

    override fun onCancel() {
        downloadHashMap.forEach { (packageName, downloadData) ->
            if (Status.CANCELLED == PRDownloader.getStatus(downloadData.downloadID)) {
                Log.i(EmulatorDebug.LOG_TAG, "Download of $packageName was cancelled.")
            }
        }
    }

    override fun onProgress(progress: Progress?) {
        downloadHashMap.forEach { (_, downloadData) ->
            if (Status.RUNNING == PRDownloader.getStatus(downloadData.downloadID)) {
                val progressPercent = progress!!.currentBytes * 100 / progress.totalBytes
                // Log.i(EmulatorDebug.LOG_TAG, "Download of $packageName has been ${progressPercent.toInt()}% completed.")
            }
        }
        //todo Update the progress on stout
    }

    override fun onDownloadComplete() {
        downloadHashMap.forEach { (packageName, downloadData) ->
            if (Status.COMPLETED == PRDownloader.getStatus(downloadData.downloadID)) {
                Log.i(EmulatorDebug.LOG_TAG, "Downloading $packageName Completed!")
                downloadData.isSuccessful = true
                checkIfDownloadsAreCompleted()
            }
        }

    }


    override fun onError(error: Error?) {
        downloadHashMap.forEach { (packageName, downloadData) ->
            if (Status.FAILED == PRDownloader.getStatus(downloadData.downloadID)) {
                Log.i(EmulatorDebug.LOG_TAG, "Downloading $packageName error with $error!")
                downloadData.isSuccessful = false
                checkIfDownloadsAreCompleted()
            }
        }
    }

    private fun checkIfDownloadsAreCompleted() {
        var counter = 0
        downloadHashMap.forEach { (_, downloadData) ->
            if (downloadData.isSuccessful != null)
                counter++
        }
        if (counter == totalPackages) {
            //All the packages have been processed which means that either they are failed or completed successfully
            proceedToInstallation()
        }
    }


    private fun proceedToInstallation() {
        downloadHashMap.forEach { (packageName, downloadData) ->
            if (downloadData.isSuccessful!!) {
                Log.i(EmulatorDebug.LOG_TAG, "$packageName is proceeding to get installed.")
                installAPK(File("${TermuxService.FILES_PATH}/$packageName.apk"), context)
            } else {
                Log.i(EmulatorDebug.LOG_TAG, "$packageName has failed while downloading.")
            }
        }
    }

    private fun installAPK(file: File, context: Context) {

        //todo ACTION_INSTALL_PACKAGE has been depreciated in api 29 so we have to check and implement the new packageManager
        GlobalScope.launch(Dispatchers.IO) {
            // if (android.os.Build.VERSION.SDK_INT >= 29) {
            val installIntent = Intent(Intent.ACTION_INSTALL_PACKAGE)
            installIntent.data = Uri.parse("content://com.termux.files" + file.absolutePath)
            installIntent.flags = Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_GRANT_READ_URI_PERMISSION
            context.startActivity(installIntent)
        }
    }

    fun getInstalledPackages(): ArrayList<String> {
        val installedTermuxPackageList = arrayListOf<String>()
        val pm: PackageManager = context.packageManager
        val installedPackages = pm.getInstalledApplications(PackageManager.GET_META_DATA)

        installedPackages.forEach { appInfo ->
            val packagesName = appInfo.packageName
            if (packagesName.startsWith(TERMUX_APK_SUFFIX)) {
                installedTermuxPackageList.add(appInfo.packageName.replace(TERMUX_APK_SUFFIX, ""))
            }
        }
        return installedTermuxPackageList
    }

    fun uninstallAPK(pkg: String) {
        val intent = Intent(Intent.ACTION_DELETE)
        intent.flags = Intent.FLAG_ACTIVITY_NEW_TASK
        intent.data = Uri.parse("package:${TERMUX_APK_SUFFIX}${pkg}")
        context.startActivity(intent)

        //todo maybe register for a package delete broadcast as a call back
    }

    data class PackageDownloadData(var downloadID: Int, var isSuccessful: Boolean?)
}
