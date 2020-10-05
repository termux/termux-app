package com.termux.app.packagemanager

import android.app.NotificationManager
import android.content.Context
import android.os.Handler
import android.os.StatFs
import androidx.core.app.NotificationCompat
import com.termux.R
import com.termux.app.packagemanager.Constants.Companion.APK_REPO_URL
import com.termux.app.packagemanager.PackageInstaller.Companion.log
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch
import java.io.File
import java.io.FileNotFoundException
import java.io.FileOutputStream
import java.net.*


// Download status constants

const val ENTERED = -1
const val QUEUED = 0
const val STARTED = 1
const val RUNNING = 2
const val COMPLETED = 3
const val ERROR = 4
const val NOTIFICATION_CHANNEL_ID = "termux_notification_channel"

class PackageDownloader(val context: Context) {

    private val TERMUX_CACHE_DIRECTORY = "${context.cacheDir}${Constants.TERMUX_CACHE_PKG_DIRECTORY_SUBFOLDER}"

    private lateinit
    var notificationManager: NotificationManager
    private lateinit var builder: NotificationCompat.Builder

    interface ProgressListener {
        fun onProgress(data: DownloadData)
    }

    interface StartListener {
        fun onStart(data: DownloadData)
    }

    interface CompleteListener {
        fun onComplete(data: DownloadData)
    }

    interface ErrorListener {
        fun onError(data: ErrorData)
    }

    private lateinit var progressListener: ProgressListener
    private lateinit var errorListener: ErrorListener
    private lateinit var completeListener: CompleteListener
    private lateinit var startListener: StartListener
    private lateinit var downloadingJob: Job


    fun initListeners(progressL: ProgressListener, errorL: ErrorListener, completeL: CompleteListener, startL: StartListener) {
        this.progressListener = progressL
        this.completeListener = completeL
        this.errorListener = errorL
        this.startListener = startL
    }

    fun download(packageName: String) {
        notificationManager = context.getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
        builder = NotificationCompat.Builder(context, "termux_notification_channel").setChannelId(NOTIFICATION_CHANNEL_ID)

        File(TERMUX_CACHE_DIRECTORY).let { file ->
            if (!file.exists()) {
                file.mkdir()
            }
        }

        var isStartNotified = false

        var percent20 = false
        var percent40 = false
        var percent60 = false
        var percent80 = false

        val fileUrl = "$APK_REPO_URL$packageName.apk"
        "URL -> $fileUrl".log()
        try {
            downloadingJob = GlobalScope.launch(Dispatchers.IO) {
                val downloadData = DownloadData(packageName, 0, 0, 0, ENTERED)
                try {
                    showNotification(downloadData)
                    val downloadFile = File("$TERMUX_CACHE_DIRECTORY/${packageName}.apk")
                    deleteFileIfExists(downloadFile)
                    "Fetching the file size...".log()
                    val url = URL(fileUrl)
                    val connection: URLConnection = url.openConnection()
                    connection.connect()
                    val lengthOfFile: Int = connection.contentLength
                    var total = 0

                    if ((getFreeSpace() * 2) > lengthOfFile) {
                        downloadData.totalKB = lengthOfFile.toKB()
                        downloadData.Status = QUEUED
                        "Queuing the download...".log()
                        FileOutputStream(downloadFile).use { out ->
                            url.openStream().use { `in` ->
                                val buffer = ByteArray(1024)
                                var read: Int
                                while (`in`.read(buffer).also { read = it } >= 0) {
                                    total += read
                                    out.write(buffer, 0, read)
                                    downloadData.progressInKB = total.toKB()

                                    if (total != 0 && !isStartNotified) {
                                        downloadData.Status = STARTED
                                        startListener.onStart(downloadData)
                                        isStartNotified = true
                                    }
                                    downloadData.Status = RUNNING
                                    GlobalScope.launch(Dispatchers.Main) {

                                    }
                                    val percent = (total * 100) / lengthOfFile
                                    fun updateProgress(percent: Int) {
                                        downloadData.progressPercent = percent
                                        progressListener.onProgress(downloadData)
                                    }
                                    updateNotification(downloadData)
                                    if (percent % 20 == 0 && total != lengthOfFile) {
                                        // Can be simplified
                                        percent.let {
                                            if (it == 20 && !percent20) {
                                                percent20 = true
                                                updateProgress(it)
                                            } else if (it == 40 && !percent40) {
                                                percent40 = true
                                                updateProgress(it)
                                            } else if (it == 60 && !percent60) {
                                                percent60 = true
                                                updateProgress(it)
                                            } else if (it == 80 && !percent80) {
                                                percent80 = true
                                                updateProgress(it)
                                            }
                                        }
                                    }

                                    if (total == lengthOfFile) {
                                        downloadData.progressPercent = percent
                                        downloadData.Status = COMPLETED
                                        removeNotification(downloadData)
                                        completeListener.onComplete(downloadData)
                                    }
                                }
                            }
                        }
                    } else {
                        throw InsufficientStorageException("Insufficient Storage. Please clear some data before installing.")
                    }

                } catch (e: FileNotFoundException) {
                    val errorData = ErrorData(packageName = packageName, Status = ERROR, error = "Package $packageName does not exists!", notificationID = downloadData.notificationID)
                    packageName.clearThingsUp(errorData)
                    errorListener.onError(errorData)
                } catch (e: UnknownHostException) {
                    val errorData = ErrorData(packageName = packageName, Status = ERROR, error = "Cannot connect to internet or server unavailable. Aborting the installation.", notificationID = downloadData.notificationID)
                    packageName.clearThingsUp(errorData)
                    errorListener.onError(errorData)
                } catch (e: SocketException) {
                    val errorData = ErrorData(packageName = packageName, Status = ERROR, error = "Cannot connect to internet or server unavailable. Aborting the installation.", notificationID = downloadData.notificationID)
                    packageName.clearThingsUp(errorData)
                    errorListener.onError(errorData)
                } catch (e: ConnectException) {
                    val errorData = ErrorData(packageName = packageName, Status = ERROR, error = "Cannot connect to internet or server unavailable. Aborting the installation.", notificationID = downloadData.notificationID)
                    packageName.clearThingsUp(errorData)
                    errorListener.onError(errorData)
                } catch (e: InsufficientStorageException) {
                    val errorData = ErrorData(packageName = packageName, Status = ERROR, error = "Insufficient Storage. Please clear some data before installing.", notificationID = downloadData.notificationID)
                    errorListener.onError(ErrorData(packageName = packageName, Status = ERROR, error = ""))
                    packageName.clearThingsUp(errorData)
                    errorListener.onError(errorData)
                } catch (e: Exception) {
                    "Error installing $packageName $e".log()
                    val errorData = ErrorData(packageName = packageName, Status = ERROR, error = e.toString(), notificationID = downloadData.notificationID)
                    packageName.clearThingsUp(errorData)
                    errorListener.onError(errorData)
                }
            }

        } catch (e: Exception) {
            "Error installing $packageName $e".log()
            val errorData = ErrorData(packageName = packageName, Status = ERROR, error = e.toString())
            packageName.clearThingsUp(errorData)
            errorListener.onError(errorData)
        }
    }


    private fun getFreeSpace(): Long {
        val path = context.dataDir
        val stat = StatFs(path.path)
        val blockSize: Long
        val availableBlocks: Long
        blockSize = stat.blockSizeLong
        availableBlocks = stat.availableBlocksLong
        return availableBlocks * blockSize
    }

    private fun deleteFileIfExists(downloadFile: File) {
        if (downloadFile.exists()) {
            if (downloadFile.delete())
                "File Deleted!".log()
        }
    }

    private fun Int.toKB(): Long {
        return (this * 0.001).toLong()
    }

    private fun String.clearThingsUp(errorData: ErrorData) {
        if (this@PackageDownloader::downloadingJob.isInitialized) {
            if (downloadingJob.isActive) {
                downloadingJob.cancel()
            }
        }
        val downloadFile = File("$TERMUX_CACHE_DIRECTORY/${this}.apk")
        deleteFileIfExists(downloadFile)
        if (errorData.notificationID != 0) {
            notificationManager.cancel(errorData.notificationID)
        }
    }

    /*
    Notification
    */

    private fun showNotification(downloadData: DownloadData) {
        builder
            .setContentTitle("Downloading ${downloadData.packageName}")
            .setSmallIcon(R.drawable.ic_service_notification)
            .setProgress(0, 0, true)

        //setting indeterminate progress

        getNotificationID().let {
            downloadData.notificationID = it
            notificationManager.notify(it, builder.build())
        }
    }

    private fun removeNotification(downloadData: DownloadData) {
        GlobalScope.launch(Dispatchers.Main) {
            if (downloadData.Status == COMPLETED) {
                builder.setContentTitle("Package Downloaded.")
                builder.setContentTitle("${downloadData.packageName} has been download. You can install the package now.")
                builder.setProgress(0, 0, false)
                notificationManager.notify(downloadData.notificationID, builder.build())
                Handler().postDelayed({
                    notificationManager.cancel(downloadData.notificationID)
                }, 5000)
            } else
                notificationManager.cancel(downloadData.notificationID)
        }
    }

    private fun updateNotification(downloadData: DownloadData) {
        downloadData.let {
            builder.setContentTitle("Downloading ${downloadData.packageName}")
            builder.setProgress(it.totalKB.toInt(), it.progressInKB.toInt(), false)
            notificationManager.notify(it.notificationID, builder.build())
        }


    }

    private fun getNotificationID(): Int {
        return (100..999).random()
    }

}


data class DownloadData(
    var packageName: String,
    var totalKB: Long = 0,
    var progressInKB: Long = 0,
    var progressPercent: Int = 0,
    var Status: Int,
    var notificationID: Int = 0
)

class InsufficientStorageException(message: String) : Exception(message)
data class ErrorData(var packageName: String, var error: String, var extraLogs: String = "", var Status: Int, var notificationID: Int = 0)
