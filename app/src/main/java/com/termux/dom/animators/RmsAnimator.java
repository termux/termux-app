package com.termux.dom.animators;

/**
 * Created by andrzej on 29.01.18.
 */

import java.util.ArrayList;
import java.util.List;

import com.termux.dom.RecognitionBar;
import com.termux.dom.animators.*;
import com.termux.dom.animators.BarRmsAnimator;

public class RmsAnimator implements com.termux.dom.animators.BarParamsAnimator {
    final private List<com.termux.dom.animators.BarRmsAnimator> barAnimators;


    public RmsAnimator(List<RecognitionBar> recognitionBars) {
        this.barAnimators = new ArrayList<>();
        for (RecognitionBar bar : recognitionBars) {
            barAnimators.add(new com.termux.dom.animators.BarRmsAnimator(bar));
        }
    }

    @Override
    public void start() {
        for (com.termux.dom.animators.BarRmsAnimator barAnimator : barAnimators) {
            barAnimator.start();
        }
    }

    @Override
    public void stop() {
        for (com.termux.dom.animators.BarRmsAnimator barAnimator : barAnimators) {
            barAnimator.stop();
        }
    }

    @Override
    public void animate() {
        for (com.termux.dom.animators.BarRmsAnimator barAnimator : barAnimators) {
            barAnimator.animate();
        }
    }

    public void onRmsChanged(float rmsDB) {
        for (BarRmsAnimator barAnimator : barAnimators) {
            barAnimator.onRmsChanged(rmsDB);
        }
    }
}
