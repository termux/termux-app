package com.termux.app.activities;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.viewpager.widget.PagerAdapter;
import androidx.viewpager.widget.ViewPager;

import com.termux.R;
import com.termux.shared.termux.settings.preferences.TermuxAppSharedPreferences;

public class WelcomeActivity extends AppCompatActivity {

    private ViewPager mViewPager;
    private Button mBtnNext, mBtnSkip;
    private Slide[] mSlides;
    private TermuxAppSharedPreferences mPreferences;
    private LayoutInflater mInflater;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mPreferences = TermuxAppSharedPreferences.build(this);
        if (mPreferences == null || !mPreferences.shouldShowWelcomeScreens()) {
            finish();
            return;
        }

        setContentView(R.layout.activity_welcome);
        mInflater = LayoutInflater.from(this);

        mViewPager = findViewById(R.id.view_pager);
        mBtnNext = findViewById(R.id.btn_next);
        mBtnSkip = findViewById(R.id.btn_skip);

        mSlides = new Slide[]{
                new Slide(R.string.welcome_title_1, R.string.welcome_desc_1),
                new Slide(R.string.welcome_title_2, R.string.welcome_desc_2),
                new Slide(R.string.welcome_title_3, R.string.welcome_desc_3)
        };

        WelcomePagerAdapter welcomePagerAdapter = new WelcomePagerAdapter();
        mViewPager.setAdapter(welcomePagerAdapter);
        mViewPager.addOnPageChangeListener(viewPagerPageChangeListener);

        mBtnSkip.setOnClickListener(v -> launchHomeScreen());

        mBtnNext.setOnClickListener(v -> {
            int next = mViewPager.getCurrentItem() + 1;
            if (next < mSlides.length) {
                mViewPager.setCurrentItem(next);
            } else {
                launchHomeScreen();
            }
        });
    }

    private void launchHomeScreen() {
        if (mPreferences != null) {
            mPreferences.setShouldShowWelcomeScreens(false);
        }
        finish();
    }

    ViewPager.OnPageChangeListener viewPagerPageChangeListener = new ViewPager.OnPageChangeListener() {

        @Override
        public void onPageSelected(int position) {
            if (position == mSlides.length - 1) {
                mBtnNext.setText(getString(R.string.welcome_button_done));
                mBtnSkip.setVisibility(View.GONE);
            } else {
                mBtnNext.setText(getString(R.string.welcome_button_next));
                mBtnSkip.setVisibility(View.VISIBLE);
            }
        }

        @Override
        public void onPageScrolled(int arg0, float arg1, int arg2) {}

        @Override
        public void onPageScrollStateChanged(int arg0) {}
    };

    private static class Slide {
        final int titleResId;
        final int descResId;

        Slide(int titleResId, int descResId) {
            this.titleResId = titleResId;
            this.descResId = descResId;
        }
    }

    public class WelcomePagerAdapter extends PagerAdapter {

        @NonNull
        @Override
        public Object instantiateItem(@NonNull ViewGroup container, int position) {
            View view = mInflater.inflate(R.layout.fragment_welcome_slide, container, false);

            TextView title = view.findViewById(R.id.slide_title);
            TextView desc = view.findViewById(R.id.slide_desc);
            ImageView image = view.findViewById(R.id.slide_image);

            Slide slide = mSlides[position];
            title.setText(slide.titleResId);
            desc.setText(slide.descResId);
            image.setImageResource(R.mipmap.ic_launcher);

            container.addView(view);
            return view;
        }

        @Override
        public int getCount() {
            return mSlides.length;
        }

        @Override
        public boolean isViewFromObject(@NonNull View view, @NonNull Object obj) {
            return view == obj;
        }

        @Override
        public void destroyItem(@NonNull ViewGroup container, int position, @NonNull Object object) {
            container.removeView((View) object);
        }
    }
}
