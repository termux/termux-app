package com.termux.app.packagemanager

import android.content.Context
import android.content.pm.PackageManager
import com.termux.app.packagemanager.PackageInstaller.Companion.log


class PackageLister(val context: Context) {
    // This class can be used to implement other stuff in the future relating to packages


    fun listPackages() {
        val termuxPackagesList: ArrayList<String> = arrayListOf()
        val pm: PackageManager = context.packageManager
        val packages = pm.getInstalledApplications(PackageManager.GET_META_DATA)

        packages.forEach { packageInfo ->
            val packageName = packageInfo.packageName
            if (packageName.startsWith(TERMUX_APK_SUFFIX)) {
                termuxPackagesList.add(packageName.replace(TERMUX_APK_SUFFIX, ""))
            }
        }

        if (termuxPackagesList.isEmpty())
            ("No package is currently installed").log()
        else {
            ("Here are the installed termux packages -> ").log()
            termuxPackagesList.forEach { (it).log() }
        }
    }
}
