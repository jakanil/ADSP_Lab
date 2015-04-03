package com.dsp.speechpipeline;

import java.util.Arrays;

public class WaveFrame {
	private float[] debugData;
	private short[] audioSamples;
	
	public WaveFrame(){
		this.debugData = null;
		this.audioSamples = new short[Settings.stepSize];
	}
	
	public WaveFrame(short[] samples){
		this.debugData = null;
		this.audioSamples = Arrays.copyOf(samples, samples.length);
	}
	
	public void setDebug(float[] debugdata){
		this.debugData = debugdata;
	}
	
	public float[] getDebug(){
		return debugData;
	}
	
	public short[] getAudio(){ 
		return audioSamples;
	}
	
	public void setAudio(short[] samples){
		for(int i=0;i<Settings.stepSize;i++){
			this.audioSamples[i] = samples[i];
		}
	}
}
