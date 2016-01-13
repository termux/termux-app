package com.termux.app;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.text.Selection;
import android.util.TypedValue;
import android.view.KeyEvent;
import android.view.ViewGroup.LayoutParams;
import android.view.WindowManager;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;

final class DialogUtils {

	public interface TextSetListener {
		void onTextSet(String text);
	}

	static void textInput(Activity activity, int titleText, int positiveButtonText, String initialText, final TextSetListener onPositive,
										 int neutralButtonText, final TextSetListener onNeutral) {
		final EditText input = new EditText(activity);
		input.setSingleLine();
		if (initialText != null) {
			input.setText(initialText);
			Selection.setSelection(input.getText(), initialText.length());
		}

		final AlertDialog[] dialogHolder = new AlertDialog[1];
		input.setImeActionLabel(activity.getResources().getString(positiveButtonText), KeyEvent.KEYCODE_ENTER);
		input.setOnEditorActionListener(new TextView.OnEditorActionListener() {
			@Override
			public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
				onPositive.onTextSet(input.getText().toString());
				dialogHolder[0].dismiss();
				return true;
			}
		});

		float dipInPixels = TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 1, activity.getResources().getDisplayMetrics());
		// https://www.google.com/design/spec/components/dialogs.html#dialogs-specs
		int paddingTopAndSides = Math.round(16 * dipInPixels);
		int paddingBottom = Math.round(24 * dipInPixels);

		LinearLayout layout = new LinearLayout(activity);
		layout.setOrientation(LinearLayout.VERTICAL);
		layout.setLayoutParams(new LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));
		layout.setPadding(paddingTopAndSides, paddingTopAndSides, paddingTopAndSides, paddingBottom);
		layout.addView(input);

		AlertDialog.Builder builder = new AlertDialog.Builder(activity)
				.setTitle(titleText).setView(layout)
				.setPositiveButton(positiveButtonText, new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface d, int whichButton) {
						onPositive.onTextSet(input.getText().toString());
				}
			})
				.setNegativeButton(android.R.string.cancel, null);

		if (onNeutral != null) {
			builder.setNeutralButton(neutralButtonText, new DialogInterface.OnClickListener() {
				@Override
				public void onClick(DialogInterface dialog, int which) {
					onNeutral.onTextSet(input.getText().toString());
				}
			});
		}

		dialogHolder[0] = builder.create();
		dialogHolder[0].getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_VISIBLE);
		dialogHolder[0].show();
	}

}
