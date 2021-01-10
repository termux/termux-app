package com.termux.app;

import android.os.Bundle;
import android.view.View;

import androidx.annotation.FloatRange;
import androidx.annotation.Nullable;


import com.codemybrainsout.onboarder.AhoyOnboarderActivity;
import com.codemybrainsout.onboarder.AhoyOnboarderCard;
import com.termux.R;

import java.util.ArrayList;
import java.util.List;


public class IntroActivity  extends AhoyOnboarderActivity {


    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);

        //ahoyOnboarderCard1.setIconLayoutParams(iconWidth, iconHeight, marginTop, marginLeft, marginRight, marginBottom);
        List<AhoyOnboarderCard> pages = new ArrayList<>();
        AhoyOnboarderCard card1 = createImgCard(" ",getString(R.string.intro_1_text), R.mipmap.ic_launcher);
        card1.setDescriptionTextSize(dpToPixels(8, this));
        AhoyOnboarderCard card2 = createImgCard(" ",getString(R.string.intro_2_text), R.drawable.ic_baseline_self_improvement_24);
        AhoyOnboarderCard card3 = createImgCard(" ",getString(R.string.intro_3_text), R.drawable.ic_baseline_emoji_people_24);
        AhoyOnboarderCard card4 = createImgCard(" ",getString(R.string.intro_4_text), R.drawable.ic_baseline_warning_24);
        AhoyOnboarderCard card5 = createImgCard(" ",getString(R.string.intro_5_text), R.drawable.ic_baseline_child_friendly_24);

        AhoyOnboarderCard card6 = createImgCard(" ", getString(R.string.intro_finish_text),R.drawable.ic_baseline_wifi_24);
        pages.add(card1);
        pages.add(card2);
        pages.add(card3);
        pages.add(card4);
        pages.add(card5);
        pages.add(card6);

        setOnboardPages(pages);
        setColorBackground(R.color.ic_launcher_background);
        setFinishButtonTitle(getString(R.string.intro_finish_button));
    }

    private AhoyOnboarderCard createCard(String title, String description){
        AhoyOnboarderCard ahoyOnboarderCard = new AhoyOnboarderCard(title, description);
        ahoyOnboarderCard.setBackgroundColor(R.color.black_transparent);
        ahoyOnboarderCard.setTitleColor(R.color.white);
        ahoyOnboarderCard.setDescriptionColor(R.color.grey_200);
        ahoyOnboarderCard.setTitleTextSize(dpToPixels(10, this));
        ahoyOnboarderCard.setDescriptionTextSize(dpToPixels(5, this));
        return ahoyOnboarderCard;

    }
    private AhoyOnboarderCard createImgCard(String title,  String description, int image){
        AhoyOnboarderCard ahoyOnboarderCard = createCard(title, description);
        ahoyOnboarderCard.setImageResourceId(image);
        return ahoyOnboarderCard;
    }
    @Override
    public void onFinishButtonPressed() {
       finish();
    }
}
