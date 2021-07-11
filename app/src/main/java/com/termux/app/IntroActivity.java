package com.termux.app;

import android.os.Bundle;
import android.view.View;


import com.termux.R;

import java.util.ArrayList;
import java.util.List;


import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.ContextCompat;
import androidx.viewpager2.widget.ViewPager2;
import android.annotation.SuppressLint;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;

public class IntroActivity extends AppCompatActivity {
    private IntroAdapter onboardingAdapter;
    private LinearLayout layoutOnboardingIndicator;
    private Button buttonOnboardingAction;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.intro);
        layoutOnboardingIndicator = findViewById(R.id.layoutOnboardingIndicators);
        buttonOnboardingAction = findViewById(R.id.buttonOnBoardingAction);
        setOnboardingItem();
        ViewPager2 onboardingViewPager = findViewById(R.id.onboardingViewPager);
        onboardingViewPager.setAdapter(onboardingAdapter);
        setOnboadingIndicator();
        setCurrentOnboardingIndicators(0);
        onboardingViewPager.registerOnPageChangeCallback(new ViewPager2.OnPageChangeCallback() {
            @Override
            public void onPageSelected(int position) {
                super.onPageSelected(position);
                setCurrentOnboardingIndicators(position);
            }
        });
        buttonOnboardingAction.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (onboardingViewPager.getCurrentItem() + 1 < onboardingAdapter.getItemCount()) {
                    onboardingViewPager.setCurrentItem(onboardingViewPager.getCurrentItem() + 1);
                } else {
                    finish();
                }
            }
        });
    }
    private void setOnboadingIndicator() {
        ImageView[] indicators = new ImageView[onboardingAdapter.getItemCount()];
        LinearLayout.LayoutParams layoutParams = new LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT
        );
        layoutParams.setMargins(8, 0, 8, 0);
        for (int i = 0; i < indicators.length; i++) {
            indicators[i] = new ImageView(getApplicationContext());
            indicators[i].setImageDrawable(ContextCompat.getDrawable(
                getApplicationContext(), R.drawable.ic_baseline_blur_circular_24
            ));
            indicators[i].setLayoutParams(layoutParams);
            layoutOnboardingIndicator.addView(indicators[i]);
        }
    }
    @SuppressLint("SetTextI18n")
    private void setCurrentOnboardingIndicators(int index) {
        int childCount = layoutOnboardingIndicator.getChildCount();
        for (int i = 0; i < childCount; i++) {
            ImageView imageView = (ImageView) layoutOnboardingIndicator.getChildAt(i);
            if (i == index) {
                imageView.setImageDrawable(ContextCompat.getDrawable(getApplicationContext(), R.drawable.ic_baseline_check_circle_24));
            } else {
                imageView.setImageDrawable(ContextCompat.getDrawable(getApplicationContext(), R.drawable.ic_baseline_blur_circular_24));
            }
        }
        if (index == onboardingAdapter.getItemCount() - 1){
            buttonOnboardingAction.setText("Finish");
        }else {
            buttonOnboardingAction.setText("Next");
        }
    }
    private void setOnboardingItem() {
        List<IntroItem> onBoardingItems = new ArrayList<>();
        IntroItem item1 = new IntroItem();
        item1.setTitle(getString(R.string.intro_1_title));
        item1.setDescription(getString(R.string.intro_1_text));
        item1.setImage(R.drawable.ic_launcher);

        IntroItem item2 = new IntroItem();
        item2.setTitle(getString(R.string.intro_2_title));
        item2.setDescription(getString(R.string.intro_2_text));
        item2.setImage(R.drawable.ic_undraw_mindfulness);

        IntroItem item3 = new IntroItem();
        item3.setTitle(getString(R.string.intro_3_title));
        item3.setDescription(getString(R.string.intro_3_text));
        item3.setImage(R.drawable.ic_undraw_team_spirit);

        IntroItem item4 = new IntroItem();
        item4.setTitle(getString(R.string.intro_4_title));
        item4.setDescription(getString(R.string.intro_4_text));
        item4.setImage(R.drawable.ic_undraw_notify);

        IntroItem item5 = new IntroItem();
        item5.setTitle(getString(R.string.intro_5_title));
        item5.setDescription(getString(R.string.intro_5_text));
        item5.setImage(R.drawable.ic_undraw_baby);

        IntroItem item6 = new IntroItem();
        item6.setTitle(getString(R.string.intro_finish_title));
        item6.setDescription(getString(R.string.intro_finish_text));
        item6.setImage(R.drawable.ic_undraw_upgrade);

        onBoardingItems.add(item1);
        onBoardingItems.add(item2);
        onBoardingItems.add(item4);
        onBoardingItems.add(item5);
        onBoardingItems.add(item3);
        onBoardingItems.add(item6);
        onboardingAdapter = new IntroAdapter(onBoardingItems);
    }
}
