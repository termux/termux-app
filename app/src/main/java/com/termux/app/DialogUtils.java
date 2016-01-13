package com.termux.app;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.util.TypedValue;
import android.view.ViewGroup.LayoutParams;
import android.widget.EditText;
import android.widget.LinearLayout;

final class DialogUtils {

	public interface TextSetListener {
		void onTextSet(String text);
	}

	static void textInput(Activity activity, int titleText, int positiveButtonText, String initialText, final TextSetListener onPositive,
										 int neutralButtonText, final TextSetListener onNeutral) {
		final EditText input = new EditText(activity);
		input.setSingleLine();
		if (initialText != null) input.setText(initialText);

		float dipInPixels = TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 1, activity.getResources().getDisplayMetrics());
		// https://www.google.com/design/spec/components/dialogs.html#dialogs-specs
		int paddingTopAndSides = Math.round(16 * dipInPixels);
		int paddingBottom = Math.round(24 * dipInPixels);

		LinearLayout layout = new LinearLayout(activity);
		layout.setOrientation(LinearLayout.VERTICAL);
		layout.setLayoutParams(new LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));
		// layout.setGravity(Gravity.CLIP_VERTICAL);
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

		builder.show();
		input.requestFocus();
	}

}
