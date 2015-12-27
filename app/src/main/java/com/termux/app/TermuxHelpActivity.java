package com.termux.app;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.ViewGroup;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;

/** Basic embedded browser for viewing help pages. */
public final class TermuxHelpActivity extends Activity {

	private WebView mWebView;
	private ProgressBar mProgressBar;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		final RelativeLayout progressLayout = new RelativeLayout(this);
		RelativeLayout.LayoutParams lParams = new RelativeLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
		lParams.addRule(RelativeLayout.CENTER_IN_PARENT);
		mProgressBar = new ProgressBar(this);
		mProgressBar.setIndeterminate(true);
		mProgressBar.setLayoutParams(lParams);
		progressLayout.addView(mProgressBar);

		mWebView = new WebView(this);
		WebSettings settings = mWebView.getSettings();
		settings.setCacheMode(WebSettings.LOAD_NO_CACHE);
		settings.setAppCacheEnabled(false);
		setContentView(progressLayout);
		mWebView.clearCache(true);

		mWebView.setWebViewClient(new WebViewClient() {
			@Override
			public boolean shouldOverrideUrlLoading(WebView view, String url) {
				if (url.startsWith("https://termux.com")) {
					// Inline help.
					setContentView(progressLayout);
					return false;
				}

				try {
					startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(url)).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK));
				} catch (ActivityNotFoundException e) {
					// Android TV does not have a system browser.
					setContentView(progressLayout);
					return false;
				}
				return true;
			}

			@Override
			public void onPageFinished(WebView view, String url) {
				setContentView(mWebView);
			}
		});
		mWebView.loadUrl("https://termux.com/help.html");
	}

	@Override
	public void onBackPressed() {
		if (mWebView.canGoBack()) {
			mWebView.goBack();
		} else {
			super.onBackPressed();
		}
	}

}
