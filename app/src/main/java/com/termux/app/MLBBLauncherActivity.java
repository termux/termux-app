package com.termux.app;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.view.Gravity;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

/**
 * UI foundation. The resource editors and Shizuku file broker will be integrated
 * after the renamed bootstrap has been rebuilt and verified.
 */
public class MLBBLauncherActivity extends Activity {
    private LinearLayout root;
    private void label(String s, float size) {
        TextView t = new TextView(this);
        t.setText(s); t.setTextSize(size); t.setTextColor(Color.WHITE);
        t.setPadding(12,18,12,18); root.addView(t);
    }
    private void category(String title,String subtitle) {
        Button button = new Button(this);
        button.setAllCaps(false); button.setText(title + "  ·  Coming soon");
        root.addView(button);
        button.setOnClickListener(v -> new android.app.AlertDialog.Builder(this)
            .setTitle(title).setMessage(subtitle+" is planned, not implemented in this prototype.")
            .setPositiveButton("OK",null).show());
    }
    @Override protected void onCreate(Bundle state) {
        super.onCreate(state);
        ScrollView scroll = new ScrollView(this);
        root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(20,30,20,30);
        root.setBackgroundColor(Color.rgb(15,17,29));
        scroll.addView(root);setContentView(scroll);
        label("MLBB Resource Editor",26);
        label("Termux-based resource editor - GUI foundation",14);
        category("Audio","BNK browsing, sound preview, Shizuku replacement and backup/restore");
        category("Models","Model editor");
        category("Emotes","Emote editor");
        category("Images","Image resource editor");
        category("Other Resources","Additional resource format support");
        Button terminal=new Button(this);
        terminal.setAllCaps(false);
        terminal.setText("Open Developer Terminal");
        root.addView(terminal);
        terminal.setOnClickListener(v->startActivity(new Intent(this,TermuxActivity.class)));
        label("This development build still uses the Termux package prefix. Do not install as com.farizgd.mlbbeditor until its bootstrap has been rebuilt.",12);
    }
}
