package com.dsp.speechpipeline;

import java.text.DecimalFormat;
import java.util.concurrent.BlockingQueue;

public class Processing implements Runnable{
	
	private BlockingQueue<WaveFrame> input;
	private BlockingQueue<WaveFrame> output;
	private Thread speechThread;
	private SpeechProcessing speechProcessor;
	private int counter;
	private DecimalFormat decimalFormat;
	
	public Processing(BlockingQueue<WaveFrame> input, BlockingQueue<WaveFrame> output) {
		this.input = input;
		this.output = output;
		this.counter = 0;
		decimalFormat = new DecimalFormat();
        decimalFormat.setMaximumFractionDigits(2);
		speechProcessor = new SpeechProcessing();
        speechThread = new Thread(this);
        speechThread.start();
        
	}

	public void run() {
		try {
			loop:while(true) {
				WaveFrame currentFrame = null;
				currentFrame = input.take();
				if(currentFrame == Settings.STOP){
					Settings.getCallbackInterface().notify("Average Frame Processing Time: " + speechProcessor.getTime() + " ms");
					speechProcessor.release();
					output.put(currentFrame);
					break loop;
				}
				speechProcessor.process(currentFrame.getAudio());
				if(Settings.playback || Settings.debugLevel > 2) {
					speechProcessor.getOutput(currentFrame.getAudio());
				}
				if(Settings.debugLevel == 2 || Settings.debugLevel == 4){
					currentFrame.setDebug(speechProcessor.getDebug());
				}
				if(counter == Settings.secondConstant){
					if(Settings.debugLevel == 1) {
						Settings.getCallbackInterface().notify("Frame Processing Time: " + decimalFormat.format(speechProcessor.getTime()) + " ms | Class: " + Settings.noiseClasses[((int)speechProcessor.getClassification()[0])+1]);
					} else {
						Settings.getCallbackInterface().notify("Frame Processing Time: " + decimalFormat.format(speechProcessor.getTime()) + " ms");
					}
					counter = 0;
				} else {
					counter++;
				}
				output.put(currentFrame);
			}
		} catch (InterruptedException e) {
			Thread.currentThread().interrupt();
			e.printStackTrace();
		}
	}
}