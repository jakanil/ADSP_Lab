package com.dsp.speechpipeline;

import android.content.res.AssetManager;

public class SpeechProcessing {
	
	long pointer;
	
	public SpeechProcessing(){
		pointer = initialize(Settings.getAssetManager(), Settings.Fs, Settings.stepSize, Settings.windowSize, Settings.decisionBufferLength);
	}
	
	public void release(){
		finish(pointer);
	}
	
	public void process(short[] in){
		compute(pointer, in);
	}
	
	public float getTime(){
		return getTime(pointer);
	}
	
	public float[] getDebug(){
		return getDebug(pointer, Settings.debugOutput);
	}
	
	public float[] getClassification(){
		return getDebug(pointer, 13);
	}
	
	public void getOutput(short[] output){
		getOutput(pointer, Settings.output.get(), output);
	}

	//JNI Method Calls	
	private static native void compute(long memoryPointer, short[] in);
	private static native long initialize(AssetManager assetManager, int frequency, int stepSize, int windowSize, int decisionBufferLength);
	private static native void finish(long memoryPointer);
	private static native float getTime(long memoryPointer);
	private static native float[] getDebug(long memoryPointer, int debugOutput);
	private static native void getOutput(long memoryPointer, int outputSelect, short[] output);

}
