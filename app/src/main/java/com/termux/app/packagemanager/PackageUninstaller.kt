package com.termux.app.packagemanager

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.net.Uri
import com.termux.app.packagemanager.PackageInstaller.Companion.log

const val TERMUX_APK_SUFFIX = "net.termux."

class PackageUninstaller(var context: Context) {

    fun uninstallPackages(packageList: Array<String>) {
        registerBroadcast()
        packageList.forEach { uninstallAPK(it) }
    }

    private fun uninstallAPK(packageName: String) {
        val intent = Intent(Intent.ACTION_DELETE)
        intent.flags = Intent.FLAG_ACTIVITY_NEW_TASK
        intent.data = Uri.parse("package:$TERMUX_APK_SUFFIX${packageName}")
        context.startActivity(intent)
    }


    private fun registerBroadcast() {
        val uninstallApplication: BroadcastReceiver = object : BroadcastReceiver() {
            override fun onReceive(context: Context, intent: Intent) {
                val packageName = intent.data!!.encodedSchemeSpecificPart.toString()
                if (packageName.startsWith(TERMUX_APK_SUFFIX)) packageName.replace(TERMUX_APK_SUFFIX, "")
                ("Package Uninstalled --> $packageName").log()
            }
        }
        val intentFilter = IntentFilter()
        intentFilter.addAction(Intent.ACTION_PACKAGE_REMOVED)
        intentFilter.addDataScheme("package")
        context.registerReceiver(uninstallApplication, intentFilter)
    }
}
