package com.termux.app;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;

public final class TermuxFailsafeActivity extends Activity {

    public static final String TERMUX_FAILSAFE_SESSION_ACTION = "com.termux.app.failsafe_session";

    @Override
    public void onCreate(Bundle bundle) {
        super.onCreate(bundle);
        Intent intent = new Intent(TermuxFailsafeActivity.this, TermuxActivity.class);
        intent.putExtra(TERMUX_FAILSAFE_SESSION_ACTION, true);
        startActivity(intent);
        finish();
    }
}
