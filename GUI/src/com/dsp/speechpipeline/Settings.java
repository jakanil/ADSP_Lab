package com.dsp.speechpipeline;

import java.util.concurrent.atomic.AtomicInteger;

import android.content.res.AssetManager;
import android.media.AudioFormat;
import android.media.MediaRecorder;

public class Settings {
	
	public static final int FORMAT = AudioFormat.ENCODING_PCM_16BIT;
	public static final int SOURCE = MediaRecorder.AudioSource.DEFAULT;
	public static final WaveFrame STOP = new WaveFrame(new short[] {1,2,4,8});
	public static final int queueSize = 10;
	public static int Fs = 8000;
	public static float stepTime = 5.0f;
	public static float windowTime = 11.0f;
	public static int stepSize = Math.round(stepTime*Fs*0.001f);
	public static int windowSize = Math.round(windowTime*Fs*0.001f);
	public static boolean playback = false;
	public static AtomicInteger output = new AtomicInteger();
	public static boolean changed = false;
	public static int debugLevel = 0;
	public static int debugOutput = 0;
	public static String[] debugOutputNames = {"Default"};
	public static String[] debugLevels = {"Default"};
	public static String[] audioOutputs = {"Default"};
	public static String[] noiseClasses = {"Unavailable", "Machinery", "Babble", "Car"};
	private static Monitor main;
	private static AssetManager assetManager;
	public static int decisionBufferLength = 200;
	
	// UI update interval
	public static int secondConstant = Fs/stepSize;
	
	//supported sampling rates
	public static CharSequence[] samplingRates = {"8000 Hz"};
	public static CharSequence[] samplingRateValues = {"8000"};
	
	public static void setCallbackInterface(Monitor uiInterface) {
		main = uiInterface;
	}
	
	public static Monitor getCallbackInterface() {
		return main;
	}
	
	public static AssetManager getAssetManager() {
		return assetManager;
	}

	public static void setAssetManager(AssetManager assetManager) {
		Settings.assetManager = assetManager;
	}

	public static boolean setStepSize(float steptime){
		if (steptime > 0) {
			int stepsize = Math.round(steptime*Fs*0.001f);
			if (stepSize != stepsize && stepsize <= windowSize) {
				stepSize = stepsize;
				stepTime = steptime;
				secondConstant = Fs/stepsize;
				changed = true;
				return true;
			}
		}
		return false;
	}
	
	public static boolean setWindowSize(float windowtime){
		if(windowtime > 0) {
			int windowsize = Math.round(windowtime*Fs*0.001f);
			if (windowSize != windowsize && windowsize >= stepSize) {
				windowSize = windowsize;
				windowTime = windowtime;
				changed = true;
				return true;
			}
		}
		return false;
	}
	
	public static boolean setSamplingFrequency(int freq){
		if(Fs != freq){
			Fs = freq;
			stepSize = Math.round(stepTime*Fs*0.001f);
			windowSize = Math.round(windowTime*Fs*0.001f);
			secondConstant = Fs/stepSize;
			changed = true;
			return true;
		}
		return false;
	}
	
	public static boolean setDecisionBufferLength(int length) {
		if(length > 0 && length != decisionBufferLength) {
			decisionBufferLength = length;
			changed = true;
			return true;
		}
		return false;
	}

	public static boolean setPlayback(boolean flag){
		if(playback != flag){
			playback = flag;
			changed = true;
			return true;
		}
		return false;
	}
	
	public static boolean setOutput(int stream){
		if(stream < 0 || stream > audioOutputs.length) {
			return false;
		} else if(stream != output.getAndSet(stream)) {
			changed = true;
			return true;
		}
		return false;
	}
	
	public static String getOutput(){
		return audioOutputs[output.get()];
	}
	
	public static String getDebugLevel(){
		return debugLevels[debugLevel];
	}
	
	public static boolean setDebugLevel(int level){
		if(level < 0 || level > debugLevels.length){
			return false;
		} else if (debugLevel != level){
			debugLevel = level;
			changed = true;
			return true;
		}
		return false;
	}
	
	public static String getDebugOutput(){
		return debugOutputNames[debugOutput];
	}
	
	public static boolean setDebugOutput(int output){
		if(output < 0 || output > debugOutputNames.length) {
			return false;
		} else if(debugOutput != output){
			debugOutput = output;
			changed = true;
			return true;
		}
		return false;
	}
	
	public static void setRates(CharSequence[] rates){
		samplingRates = rates;
	}
	
	public static void setRateValues(CharSequence[] rateValues){
		samplingRateValues = rateValues;
	}
}
