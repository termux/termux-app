package com.termux.dom;

/**
 * Created by andrzej on 29.01.18.
 */
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.speech.RecognitionListener;
import android.speech.SpeechRecognizer;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Random;



public class AisRecognitionListener implements RecognitionListener {
    public final String TAG = AisRecognitionListener.class.getName();
    public Context context;
    public static SpeechRecognizer speechRecognizer;
    public AisRecognitionListener (Context context, SpeechRecognizer speechRecognizer) {
        this.context = context;
        this.speechRecognizer = speechRecognizer;
    }
    public static Random random = new Random();
    public static List<String> errorAnswers = Arrays.asList(
        "Nie rozpoznaję mowy, spróbuj ponownie.",
        "Brak danych głosowych, spróbuj ponownie.",
        "Nie rozumiem, spróbuj ponownie.",
        "Nie słyszę, powtórz proszę.",
        "Co mówiłeś?",
        "Przepraszam, powtórz proszę.",
        "Nie rozumiem, czy możesz powtórzyć?",
        "Powiedz jeszcze raz proszę.",
        "Nie wiem o co chodzi, proszę powtórzyć.",
        "Coś nie słychać, spróbuj ponownie.",
        "Powiedz proszę jeszcze raz.",
        "Proszę powtórzyć.",
        "Powtórz proszę jeszcze raz.",
        "Czy możesz powtórzyć?");

    @Override
    public void onReadyForSpeech(Bundle params) {
        Log.d(TAG, "AisRecognitionListener onReadyForSpeech");
    }

    @Override
    public void onBeginningOfSpeech() {
        Log.d(TAG, "AisRecognitionListener onBeginningOfSpeech");
        Intent intent = new Intent(com.termux.dom.AisCoreUtils.BROADCAST_ON_START_SPEECH_TO_TEXT);
        LocalBroadcastManager bm = LocalBroadcastManager.getInstance(context);
        bm.sendBroadcast(intent);
    }

    @Override
    public void onRmsChanged(float rmsdB) {
        Log.d(TAG, "AisRecognitionListener onRmsChanged");
    }

    @Override
    public void onBufferReceived(byte[] buffer) {
        Log.d(TAG, "AisRecognitionListener onBufferReceived");
    }


    @Override
    public void onEndOfSpeech() {
        Log.d(TAG, "AisRecognitionListener BROADCAST_ON_END_SPEECH_TO_TEXT");
        Intent intent = new Intent(com.termux.dom.AisCoreUtils.BROADCAST_ON_END_SPEECH_TO_TEXT);
        LocalBroadcastManager bm = LocalBroadcastManager.getInstance(context);
        bm.sendBroadcast(intent);

        // FloatingViewService.onEndSpeechToText(errorMessage);
    }

    @Override
    public void onError(int errorCode) {
        String errorMessage = getErrorText(errorCode);
        Log.d(TAG, "AisRecognitionListener onError, FAILED " + errorMessage);
        Intent intent = new Intent(com.termux.dom.AisCoreUtils.BROADCAST_ON_SPEECH_TO_TEXT_ERROR);
        intent.putExtra(com.termux.dom.AisCoreUtils.SPEECH_TO_TEXT_ERROR_VALUE, errorMessage);
        LocalBroadcastManager bm = LocalBroadcastManager.getInstance(context);
        bm.sendBroadcast(intent);
        // end of speech to text
        onEndOfSpeech();
    }

    @Override
    public void onResults(Bundle results) {
        Log.d(TAG, "AisRecognitionListener onResults");
        ArrayList<String> matches = results
            .getStringArrayList(SpeechRecognizer.RESULTS_RECOGNITION);
        Intent intent = new Intent(com.termux.dom.AisCoreUtils.BROADCAST_EVENT_ON_SPEECH_COMMAND);
        intent.putExtra(com.termux.dom.AisCoreUtils.BROADCAST_EVENT_ON_SPEECH_COMMAND_TEXT, matches.get(0));
        LocalBroadcastManager bm = LocalBroadcastManager.getInstance(context);
        bm.sendBroadcast(intent);
    }

    @Override
    public void onPartialResults(Bundle partialResults) {
        Log.d(TAG, "AisRecognitionListener onPartialResults");
    }

    @Override
    public void onEvent(int eventType, Bundle params) {
        Log.d(TAG, "AisRecognitionListener onEvent " + eventType + " "  + params.toString());
    }

    public static String getErrorText(int errorCode) {
        String message;
        int index = random.nextInt(errorAnswers.size());
        message = errorAnswers.get(index);
        switch (errorCode) {
            case SpeechRecognizer.ERROR_AUDIO:
                message = "Błąd nagrywania mowy.";
                break;
            case SpeechRecognizer.ERROR_CLIENT:
                if (AisCoreUtils.onBox()) {
                    // Jeśli błąd się powtarza to sprawdź klucz sprzętowy USB.
                    message = "Problem z mikrofonem, spróbuj ponownie.";
                } else {
                    message = "Błąd, spróbuj ponownie.";
                }
                break;
            case SpeechRecognizer.ERROR_INSUFFICIENT_PERMISSIONS:
                message = "Niewystarczające uprawnienia";
                break;
            case SpeechRecognizer.ERROR_NETWORK:
                message = "Błąd sieci, spróbuj ponownie.";
                break;
            case SpeechRecognizer.ERROR_NETWORK_TIMEOUT:
                message = "Limit czasu sieci.";
                break;
            case SpeechRecognizer.ERROR_NO_MATCH:
                message = message;
                break;
            case SpeechRecognizer.ERROR_RECOGNIZER_BUSY:
                // "stopListening called by other caller than startListening - ignoring"
                message = "Usługa rozpoznawania mowy jest zajęta.";
                speechRecognizer.stopListening();
                speechRecognizer.cancel();
                break;
            case SpeechRecognizer.ERROR_SERVER:
                message = "Problem z siecią, spróbuj ponownie.";
                break;
            case SpeechRecognizer.ERROR_SPEECH_TIMEOUT:
                message = message;
                break;
            default:
                message = message;
                break;
        }
        return message;
    }

}


