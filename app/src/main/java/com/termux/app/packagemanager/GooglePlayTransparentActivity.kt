package com.termux.app.packagemanager


import android.app.Activity
import android.content.ActivityNotFoundException
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.widget.Toast
import com.termux.app.packagemanager.PackageInstaller.Companion.log

class GooglePlayTransparentActivity : Activity() {

    private var currentPosition = 0
    private var onResumeCount = 0
    private lateinit var packageList: Array<String>

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        packageList = intent.getStringArrayExtra("packages")!!
        openStoreLink(packageList[currentPosition])
    }

    override fun onResume() {
        super.onResume()
        if (++onResumeCount > 1) {
            var temp = currentPosition
            if (++temp == packageList.size) {
                Toast.makeText(this, "PlayStore redirection list has exhausted. Finishing the activity...", Toast.LENGTH_SHORT).show()
                "Play Redirect Done!".log()
                finish()
            } else {
                openStoreLink(packageList[++currentPosition])
            }
        }
    }

    private fun openStoreLink(packageName: String) {
        try {
            startActivity(Intent(Intent.ACTION_VIEW, Uri.parse("market://details?id=net.termux.$packageName")).setFlags(Intent.FLAG_ACTIVITY_NEW_TASK))
        } catch (e: ActivityNotFoundException) {
            startActivity(Intent(Intent.ACTION_VIEW, Uri.parse("https://play.google.com/store/apps/details?id=net.termux.$packageName")).setFlags(Intent.FLAG_ACTIVITY_NEW_TASK))
        }
    }
}
