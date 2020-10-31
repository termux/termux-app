package com.termux.app;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.graphics.ColorFilter;
import android.graphics.ColorMatrixColorFilter;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import android.util.Log;
import android.view.Gravity;
import android.widget.Button;
import android.widget.GridLayout;
import android.widget.ImageButton;

import com.termux.view.TerminalView;

import me.xdrop.fuzzywuzzy.FuzzySearch;


public final class SuggestionBarView extends GridLayout {

    private static final int TEXT_COLOR = 0xFFC0B18B;
    private TermuxPreferences settings;
    private List<SuggestionBarButton> allSuggestionButtons;
    private List<SuggestionBarButton> defaultButtons;
    public SuggestionBarView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }


    private List<SuggestionBarButton> searchButtons(List<SuggestionBarButton> list, String input, int buttonCount, boolean fuzzy){
        List<SuggestionBarButton> newList = new ArrayList<>();
        int tolerance = settings.getSearchTolerance();
        input = input.toLowerCase();
        for(int i=0;i<list.size();i++){
            SuggestionBarButton currentButton = list.get(i);
            if(fuzzy){
                int ratio = FuzzySearch.partialRatio(input,currentButton.getText().toLowerCase());
                if(ratio >=tolerance){
                    currentButton.setRatio(ratio);
                    newList.add(currentButton);
                }
            }else if(currentButton.getText().toLowerCase().startsWith(input)){
                newList.add(currentButton);
            }

        }
        if(fuzzy){
            Collections.sort(newList, new Comparator<SuggestionBarButton>() {
                @Override
                public int compare(SuggestionBarButton suggestionBarButton, SuggestionBarButton t1) {
                    int r1 = suggestionBarButton.getRatio();
                    int r2 = t1.getRatio();
                    if (r1 >r2){
                        return -1;
                    }else if(r1< r2){
                        return 1;
                    }else{
                        return 0;
                    }
                }
            });
        }

        return newList;

    }
    void reloadWithInput(String input, final TerminalView terminalView){
        if(allSuggestionButtons == null){
            allSuggestionButtons = getInstalledAppButtons();
        }
        List<SuggestionBarButton> suggestionButtons = new ArrayList<>(allSuggestionButtons);
        setRowCount(suggestionButtons.size());
        setColumnCount(suggestionButtons.size());
        removeAllViews();
        int buttonCount = settings.getButtonCount();
        int addedCount = 0;
        if(input.length()> 0 && !input.equals(" ")){
            //List<SuggestionBarButton> oldList = suggestionButtons;
            suggestionButtons = searchButtons(suggestionButtons,input, buttonCount,input.length() >2);
            /*for(int i=0;suggestionButtons.size()<buttonCount && i<oldList.size();i++){
                boolean found = false;
                SuggestionBarButton newButton = oldList.get(i);
                for(int j = 0;j<suggestionButtons.size();j++){
                    if(suggestionButtons.get(j).equals(newButton)){
                        found = true;
                        break;
                    }
                }
                if(!found){
                    suggestionButtons.add(newButton);
                }
            }*/
        }else{
            if(defaultButtons == null) {
                defaultButtons = new ArrayList<SuggestionBarButton>();
                ArrayList<String> defaultButtonStrings = settings.getDefaultButtons();
                Collections.reverse(defaultButtonStrings);
                for (int i = 0; i < defaultButtonStrings.size(); i++) {
                    List<SuggestionBarButton> currentButtons = searchButtons(suggestionButtons, defaultButtonStrings.get(i), 1, false);
                    if (currentButtons.size() > 0) {
                        SuggestionBarButton currentButton = currentButtons.get(0);
                        if (suggestionButtons.indexOf(currentButton) >= 0) {
                            suggestionButtons.remove(suggestionButtons.indexOf(currentButton));
                        }
                        suggestionButtons.add(0, currentButton);
                        defaultButtons.add(currentButton);
                    }

                }
                Collections.reverse(defaultButtonStrings);
            }else{
                for(int i=0; i<defaultButtons.size();i++){
                    SuggestionBarButton currentButton = defaultButtons.get(i);
                    if (suggestionButtons.indexOf(currentButton) >= 0) {
                        suggestionButtons.remove(suggestionButtons.indexOf(currentButton));
                    }
                    suggestionButtons.add(0, currentButton);
                }
            }

        }
        for (int col = 0; col < suggestionButtons.size() && col <buttonCount; col++) {
            final SuggestionBarButton currentButton = suggestionButtons.get(col);
            LayoutParams param = new GridLayout.LayoutParams();
            param.width = 0;
            param.height = 0;
            param.setMargins(0, 0, 0, 0);
            param.columnSpec = GridLayout.spec(col, GridLayout.FILL, 1.f);
            param.rowSpec = GridLayout.spec(0, GridLayout.FILL, 1.f);
            if(currentButton.hasIcon() && settings.isShowIcons()){
                ImageButton imageButton = new ImageButton(getContext(),null,android.R.attr.buttonBarButtonStyle);
                imageButton.setImageDrawable(currentButton.getIcon());
                if(settings.getBandW()){
                    float[] colorMatrix = {
                        0.33f, 0.33f, 0.33f, 0, 0, //red
                        0.33f, 0.33f, 0.33f, 0, 0, //green
                        0.33f, 0.33f, 0.33f, 0, 03, //blue
                        0, 0, 0, 1, 0    //alpha
                    };
                    ColorFilter colorFilter = new ColorMatrixColorFilter(colorMatrix);
                    imageButton.setColorFilter(colorFilter);

                }
                imageButton.setLayoutParams(param);
                imageButton.setOnClickListener(v->{
                    currentButton.click();
                    if(terminalView != null){
                        terminalView.clearInputLine();
                        reloadWithInput("",terminalView);
                    }
                });
                addView(imageButton);
            }else{
                Button button;
                button = new Button(getContext(), null, android.R.attr.buttonBarButtonStyle);
                button.setTextSize(settings.getTextSize());
                button.setText(currentButton.getText());
                button.setTextColor(TEXT_COLOR);
                button.setPadding(0, 0, 0, 0);
                button.setOnClickListener(v->{
                    currentButton.click();
                    if(terminalView != null){
                        terminalView.clearInputLine();
                        reloadWithInput("",terminalView);
                    }
                });
                addView(button);
            }
            addedCount++;
        }
        int missing_buttons = buttonCount-addedCount;
        if(suggestionButtons.size() > 0 && missing_buttons > 0){
            Drawable filler = suggestionButtons.get(0).getIcon();
            for(int i=0;i<missing_buttons;i++){
                LayoutParams param = new GridLayout.LayoutParams();
                param.width = 0;
                param.height = 0;
                param.setMargins(0, 0, 0, 0);
                param.columnSpec = GridLayout.spec(i+addedCount, GridLayout.FILL, 1.f);
                param.rowSpec = GridLayout.spec(0, GridLayout.FILL, 1.f);
                ImageButton imageButton = new ImageButton(getContext(),null,android.R.attr.buttonBarButtonStyle);
                imageButton.setImageDrawable(filler);
                imageButton.setLayoutParams(param);
                imageButton.setVisibility(INVISIBLE);
                addView(imageButton);
            }
        }

    }

    private List<ResolveInfo> getInstalledApps() {
        /**Context context = getRootView().getContext();
        PackageManager packageManager = context.getPackageManager();
        List<SuggestionBarButton> apps = new ArrayList<SuggestionBarButton>();
        List<PackageInfo> packs = packageManager.getInstalledPackages(0);
        //List<PackageInfo> packs = getPackageManager().getInstalledPackages(PackageManager.GET_PERMISSIONS);
        for (int i = 0; i < packs.size(); i++) {
            PackageInfo p = packs.get(i);
            String appName = p.applicationInfo.loadLabel(packageManager).toString();
            Drawable icon = p.applicationInfo.loadIcon(packageManager);
            String packages = p.applicationInfo.packageName;
            apps.add(new SuggetionBarAppButton(context,packages,appName,icon));
        }*/
        PackageManager packageManager = getContext().getPackageManager();
        Intent main=new Intent(Intent.ACTION_MAIN, null);

        main.addCategory(Intent.CATEGORY_LAUNCHER);

        List<ResolveInfo> launchables=packageManager.queryIntentActivities(main, 0);

        Collections.sort(launchables,
            new ResolveInfo.DisplayNameComparator(packageManager));
        return launchables;
    }

    private List<SuggestionBarButton> getInstalledAppButtons(){
        PackageManager packageManager = getContext().getPackageManager();
        List<ResolveInfo> launchables = getInstalledApps();
        List<SuggestionBarButton> buttons = new ArrayList<>();
        for(int i=1; i<launchables.size();i++){
            ActivityInfo info = launchables.get(i).activityInfo;
            String packageName = info.packageName;
            String appName = info.loadLabel(packageManager).toString();
            Drawable icon = info.loadIcon(packageManager);

            buttons.add(new SuggetionBarAppButton(getContext(),packageName,appName,icon));
        }
        return buttons;
    }
    /**
     * Reload the view given parameters in termux.properties
     *
     * param infos matrix as defined in termux.properties extrakeys
     * Can Contain The Strings CTRL ALT TAB FN ENTER LEFT RIGHT UP DOWN or normal strings
     * Some aliases are possible like RETURN for ENTER, LT for LEFT and more (@see controlCharsAliases for the whole list).
     * Any string of length > 1 in total Uppercase will print a warning
     *
     * Examples:
     * "ENTER" will trigger the ENTER keycode
     * "LEFT" will trigger the LEFT keycode and be displayed as "←"
     * "→" will input a "→" character
     * "−" will input a "−" character
     * "-_-" will input the string "-_-"
     */
    @SuppressLint("ClickableViewAccessibility")
    void reload(TermuxPreferences settings) {
        this.settings = settings;
        reloadWithInput("",null);
    }

}
