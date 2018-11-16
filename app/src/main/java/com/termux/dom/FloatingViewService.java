package com.termux.dom;

import android.Manifest;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.graphics.PixelFormat;
import android.net.Uri;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.speech.RecognizerIntent;
import android.speech.SpeechRecognizer;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.view.animation.Animation;
import android.view.animation.BounceInterpolator;
import android.view.animation.ScaleAnimation;
import android.widget.CompoundButton;
import android.widget.ImageView;
import android.widget.Toast;
import android.widget.ToggleButton;

import com.termux.R;
import com.termux.app.TermuxActivity;

import java.util.Locale;

import static com.termux.dom.AisCoreUtils.BROADCAST_EVENT_ON_SPEECH_COMMAND;
import static com.termux.dom.AisCoreUtils.BROADCAST_EVENT_ON_SPEECH_COMMAND_TEXT;
import static com.termux.dom.AisCoreUtils.BROADCAST_ON_SPEECH_TO_TEXT_ERROR;
import static com.termux.dom.AisCoreUtils.getAisDomUrl;
import static com.termux.dom.AisCoreUtils.BROADCAST_ON_END_SPEECH_TO_TEXT;


public class FloatingViewService extends Service {
    private WindowManager mWindowManager;
    private View mFloatingView;
    private static ToggleButton mBtnSpeak;
    public static SpeechRecognizer mSpeech = null;
    public static RecognitionProgressView mRecognitionProgressView = null;
    private static Intent mRecognizerIntent;
    private final int REQUEST_RECORD_PERMISSION = 100;
    final String TAG = FloatingViewService.class.getName();

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }


    @Override
    public void onCreate() {
        super.onCreate();
        //Inflate the floating view layout we created
        mFloatingView = LayoutInflater.from(this).inflate(R.layout.dom_layout_floating_widget, null);
        int LAYOUT_FLAG;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            LAYOUT_FLAG = WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY;
        } else {
            LAYOUT_FLAG = WindowManager.LayoutParams.TYPE_PHONE;
        }
        //Add the view to the window.
        final WindowManager.LayoutParams params = new WindowManager.LayoutParams(
            WindowManager.LayoutParams.WRAP_CONTENT,
            WindowManager.LayoutParams.WRAP_CONTENT,
            LAYOUT_FLAG,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
            PixelFormat.TRANSLUCENT);

        //Specify the view position
        params.gravity = Gravity.TOP | Gravity.LEFT;        //Initially view will be added to top-left corner
        params.x = 0;
        params.y = 100;

        //Add the view to the window
        mWindowManager = (WindowManager) getSystemService(WINDOW_SERVICE);
        mWindowManager.addView(mFloatingView, params);

        //The root element of the collapsed view layout
        final View collapsedView = mFloatingView.findViewById(R.id.collapse_view);
        //The root element of the expanded view layout
        final View expandedView = mFloatingView.findViewById(R.id.expanded_container);

        // Set the mic button
        mBtnSpeak = (ToggleButton) mFloatingView.findViewById(R.id.collapsed_iv);
        mBtnSpeak.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton buttonView,
                                         boolean isChecked) {
                if (isChecked) {
                    startTheSpeechToText();
                } else {
                    stopTheSpeechToText();

                }
            }
        });

        //
        createRecognitionView();

        //
        // handler for received data from recognition service
        IntentFilter filter = new IntentFilter();
        filter.addAction(BROADCAST_ON_END_SPEECH_TO_TEXT);
        filter.addAction(BROADCAST_EVENT_ON_SPEECH_COMMAND);
        filter.addAction(BROADCAST_ON_SPEECH_TO_TEXT_ERROR);
        filter.addAction(BROADCAST_ON_SPEECH_TO_TEXT_ERROR);
        LocalBroadcastManager bm = LocalBroadcastManager.getInstance(this);
        bm.registerReceiver(mBroadcastReceiver, filter);

        //Set the close button
        ImageView closeButtonCollapsed = (ImageView) mFloatingView.findViewById(R.id.close_btn);
        closeButtonCollapsed.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                //close the service and remove the from from the window
                stopSelf();
            }
        });

        //Set the view while floating view is expanded.
        //Set the play button.
        ImageView playButton = (ImageView) mFloatingView.findViewById(R.id.play_btn);
        playButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Toast.makeText(FloatingViewService.this, "Playing the song.", Toast.LENGTH_LONG).show();
            }
        });


        //Set the next button.
        ImageView nextButton = (ImageView) mFloatingView.findViewById(R.id.next_btn);
        nextButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Toast.makeText(FloatingViewService.this, "Playing next song.", Toast.LENGTH_LONG).show();
            }
        });


        //Set the pause button.
        ImageView prevButton = (ImageView) mFloatingView.findViewById(R.id.prev_btn);
        prevButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Toast.makeText(FloatingViewService.this, "Playing previous song.", Toast.LENGTH_LONG).show();
            }
        });


        //Set the close button
        ImageView closeButton = (ImageView) mFloatingView.findViewById(R.id.close_button);
        closeButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                collapsedView.setVisibility(View.VISIBLE);
                expandedView.setVisibility(View.GONE);
            }
        });


        //Open the application on the button click
        ImageView openButton = (ImageView) mFloatingView.findViewById(R.id.open_button);
        openButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                //Open the application  click.
                Uri uri = Uri.parse(getAisDomUrl());
                Intent intent = new Intent(Intent.ACTION_VIEW, uri);
                startActivity(intent);

                //close the service and remove view from the view hierarchy
                stopSelf();
            }
        });

        //Drag and move the microphone
        mFloatingView.findViewById(R.id.collapsed_iv).setOnTouchListener(new View.OnTouchListener() {
            private int initialX;
            private int initialY;
            private float initialTouchX;
            private float initialTouchY;


            @Override
            public boolean onTouch(View v, MotionEvent event) {
                switch (event.getAction()) {
                    case MotionEvent.ACTION_DOWN:

                        //remember the initial position.
                        initialX = params.x;
                        initialY = params.y;

                        //get the touch location
                        initialTouchX = event.getRawX();
                        initialTouchY = event.getRawY();
                        return true;
                    case MotionEvent.ACTION_UP:
                        int Xdiff = (int) (event.getRawX() - initialTouchX);
                        int Ydiff = (int) (event.getRawY() - initialTouchY);


                        //The check for Xdiff <10 && YDiff< 10 because sometime elements moves a little while clicking.
                        //So that is click event.
                        if (Xdiff < 10 && Ydiff < 10) {
                            if (isViewCollapsed()) {
                                //When user clicks on the image view of the microphone layout,
                                if (!mBtnSpeak.isChecked()) {
                                    mBtnSpeak.setChecked(true);
                                } else {
                                    mBtnSpeak.setChecked(false);
                                }
                            }
                        }
                        return true;
                    case MotionEvent.ACTION_MOVE:
                        //Calculate the X and Y coordinates of the view.
                        params.x = initialX + (int) (event.getRawX() - initialTouchX);
                        params.y = initialY + (int) (event.getRawY() - initialTouchY);


                        //Update the layout with new X & Y coordinate
                        mWindowManager.updateViewLayout(mFloatingView, params);
                        return true;
                }
                return false;
            }
        });

        //Drag and move floating view using user's touch action.
        mFloatingView.findViewById(R.id.ais_recognition_view).setOnTouchListener(new View.OnTouchListener() {
            private int initialX;
            private int initialY;
            private float initialTouchX;
            private float initialTouchY;


            @Override
            public boolean onTouch(View v, MotionEvent event) {
                switch (event.getAction()) {
                    case MotionEvent.ACTION_DOWN:

                        //remember the initial position.
                        initialX = params.x;
                        initialY = params.y;

                        //get the touch location
                        initialTouchX = event.getRawX();
                        initialTouchY = event.getRawY();
                        return true;
                    case MotionEvent.ACTION_UP:
                        int Xdiff = (int) (event.getRawX() - initialTouchX);
                        int Ydiff = (int) (event.getRawY() - initialTouchY);


                        //The check for Xdiff <10 && YDiff< 10 because sometime elements moves a little while clicking.
                        //So that is click event.
                        if (Xdiff < 10 && Ydiff < 10) {
                            if (isViewCollapsed()) {
                                //When user clicks on the image view of the collapsed layout,
                                //visibility of the collapsed layout will be changed to "View.GONE"
                                //and expanded view will become visible.
                                collapsedView.setVisibility(View.GONE);
                                expandedView.setVisibility(View.VISIBLE);
                            }
                        }
                        return true;
                    case MotionEvent.ACTION_MOVE:
                        //Calculate the X and Y coordinates of the view.
                        params.x = initialX + (int) (event.getRawX() - initialTouchX);
                        params.y = initialY + (int) (event.getRawY() - initialTouchY);


                        //Update the layout with new X & Y coordinate
                        mWindowManager.updateViewLayout(mFloatingView, params);
                        return true;
                }
                return false;
            }
        });
    }


    /**
     * Detect if the floating view is collapsed or expanded.
     *
     * @return true if the floating view is collapsed.
     */
    private boolean isViewCollapsed() {
        return mFloatingView == null || mFloatingView.findViewById(R.id.collapse_view).getVisibility() == View.VISIBLE;
    }


    private void startTheSpeechToText() {
        ScaleAnimation scaleAnimation = new ScaleAnimation(0.7f, 1.0f, 0.7f, 1.0f,
            Animation.RELATIVE_TO_SELF, 0.7f, Animation.RELATIVE_TO_SELF, 0.7f);
        scaleAnimation.setDuration(500);
        BounceInterpolator bounceInterpolator = new BounceInterpolator();
        scaleAnimation.setInterpolator(bounceInterpolator);
        mBtnSpeak.startAnimation(scaleAnimation);

        // reset animation
        mRecognitionProgressView.stop();
        mRecognitionProgressView.play();


        mSpeech.startListening(mRecognizerIntent);

        // to stop the TTS
        //Intent intent = new Intent(AisPanelService.BROADCAST_EVENT_DO_STOP_TTS);
        //LocalBroadcastManager bm = LocalBroadcastManager.getInstance(getApplicationContext());
        //bm.sendBroadcast(intent);
    }

    private void stopTheSpeechToText() {
        ScaleAnimation scaleAnimation = new ScaleAnimation(0.7f, 1.0f, 0.7f, 1.0f,
            Animation.RELATIVE_TO_SELF, 0.7f, Animation.RELATIVE_TO_SELF, 0.7f);
        scaleAnimation.setDuration(500);
        BounceInterpolator bounceInterpolator = new BounceInterpolator();
        scaleAnimation.setInterpolator(bounceInterpolator);
        mBtnSpeak.startAnimation(scaleAnimation);

        mRecognitionProgressView.animate();
        final Handler handler = new Handler();
        handler.postDelayed(new Runnable() {
            @Override
            public void run() {
                //reset animation after 2sec
                mRecognitionProgressView.rotate();
            }
        }, 2000);
        handler.postDelayed(new Runnable() {
            @Override
            public void run() {
                //reset animation after 3sec
                mRecognitionProgressView.stop();
                mRecognitionProgressView.play();
            }
        }, 3000);

        mSpeech.stopListening();

    }

    private void createRecognitionView() {
        int[] colors = {
            ContextCompat.getColor(this, R.color.color1),
            ContextCompat.getColor(this, R.color.color2),
            ContextCompat.getColor(this, R.color.color3)
        };

        int[] heights = {70, 45, 15};
        mRecognitionProgressView = (RecognitionProgressView) mFloatingView.findViewById(R.id.ais_recognition_view);
        mRecognitionProgressView.bringToFront();

        mSpeech = SpeechRecognizer.createSpeechRecognizer(this);
        mRecognitionProgressView.setSpeechRecognizer(mSpeech);
        mRecognitionProgressView.setRecognitionListener(new AisRecognitionListener(FloatingViewService.this, mSpeech));
        mRecognitionProgressView.setColors(colors);
        mRecognitionProgressView.setBarMaxHeightsInDp(heights);
        mRecognitionProgressView.setCircleRadiusInDp(5);
        mRecognitionProgressView.setSpacingInDp(7);
        mRecognitionProgressView.setIdleStateAmplitudeInDp(0);
        mRecognitionProgressView.setRotationRadiusInDp(10);
        mRecognitionProgressView.play();

        //
        mRecognizerIntent = new Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH);
        mRecognizerIntent.putExtra(RecognizerIntent.EXTRA_LANGUAGE, Locale.getDefault().toString());
        mRecognizerIntent.putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL, RecognizerIntent.LANGUAGE_MODEL_FREE_FORM);
        mRecognizerIntent.putExtra(RecognizerIntent.EXTRA_SPEECH_INPUT_COMPLETE_SILENCE_LENGTH_MILLIS, new Long(5000));
        mRecognizerIntent.putExtra(RecognizerIntent.EXTRA_CALLING_PACKAGE, "pl.sviete.dom");
        mRecognizerIntent.putExtra(RecognizerIntent.EXTRA_MAX_RESULTS, 1);
    }


    @Override
    public void onDestroy() {
        super.onDestroy();
        try {
            unregisterReceiver(mBroadcastReceiver);
        } catch (Exception e) {
            Log.e(TAG, e.toString());
        }
        if (mFloatingView != null) mWindowManager.removeView(mFloatingView);

    }


    private final BroadcastReceiver mBroadcastReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equals(BROADCAST_ON_END_SPEECH_TO_TEXT)) {
                Log.e(TAG, BROADCAST_ON_END_SPEECH_TO_TEXT + " mBtnSpeak.setChecked(false)");
                mBtnSpeak.setChecked(false);
            } else if (intent.getAction().equals(BROADCAST_EVENT_ON_SPEECH_COMMAND)) {
                Log.d(TAG, BROADCAST_EVENT_ON_SPEECH_COMMAND + " going to publishSpeechCommand");
                final String command = intent.getStringExtra(BROADCAST_EVENT_ON_SPEECH_COMMAND_TEXT);
                DomWebInterface.publishMessage(command, "speech_command");
            }
        }
    };
}
