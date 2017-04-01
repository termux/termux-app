package com.termux.window;

import android.annotation.TargetApi;
import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.provider.Settings;
import android.view.View;

@TargetApi(23)
public class TermuxFloatPermissionActivity extends Activity {

	public static int OVERLAY_PERMISSION_REQ_CODE = 1234;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_permission);
	}

	public void onOkButton(View view) {
		Intent intent = new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION, Uri.parse("package:" + getPackageName()));
		startActivityForResult(intent, OVERLAY_PERMISSION_REQ_CODE);
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		if (requestCode == OVERLAY_PERMISSION_REQ_CODE) {
			startService(new Intent(this, TermuxFloatService.class));
			finish();
		}
	}

}
