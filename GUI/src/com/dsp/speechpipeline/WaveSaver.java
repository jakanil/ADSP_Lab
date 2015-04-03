package com.dsp.speechpipeline;

import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Arrays;
import java.util.concurrent.BlockingQueue;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;

public class WaveSaver implements Runnable{
	
	private FileOutputStream audioWriter, debugWriter;
	private BlockingQueue<WaveFrame> recycleQueue;
	private BlockingQueue<WaveFrame> outputQueue;
	private boolean saveData, debugData;
	private String outputName;
	private Thread writer;
	private Monitor main;
	private WaveFrame outputFrame;
	private AudioTrack sound;
	
	public WaveSaver(String outputName, BlockingQueue<WaveFrame> outputQueue, BlockingQueue<WaveFrame> recycleQueue) {
		main = Settings.getCallbackInterface();
		this.outputName = outputName;
		this.outputQueue = outputQueue;
		this.recycleQueue = recycleQueue;
		
		switch(Settings.debugLevel){
			case(4): {
						enablePCMOutput();
			}
			case(2): {	enableDebugOutput();
						break;
			}
			case(3): {	enablePCMOutput();
						break;
			}
			case(0): break;
			case(1): break;
			default: break;
		}
		
		if(Settings.playback){
			int size = AudioTrack.getMinBufferSize(Settings.Fs, AudioFormat.CHANNEL_OUT_MONO, Settings.FORMAT);
			sound = new AudioTrack(AudioManager.STREAM_MUSIC, Settings.Fs, AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT, size, AudioTrack.MODE_STREAM);
		}
		
        writer = new Thread(this);
        writer.start();
	}
	
	public String getOutputName(){
		return outputName;
	}
	
	private void enablePCMOutput() {
		try {
			if(main.getMode() == 2){
				audioWriter = new FileOutputStream (Utilities.getFile(outputName + ".pcm"));
				saveData = true;
			}
		} catch (IOException e){
			saveData = false;
			e.printStackTrace();
		}
	}
	
	private void enableDebugOutput() {
		try {
			debugWriter = new FileOutputStream (Utilities.getFile(outputName + ".txt"));
			debugData = true;
		} catch (IOException e){
			debugData = false;
			e.printStackTrace();
		}
	}

	public void run() {
		if(Settings.playback) {
			sound.play();
		}
		
		loop:while(true) {
			outputFrame = null;
			
			try {
				outputFrame = outputQueue.take();
			} catch (InterruptedException e) {
				Thread.currentThread().interrupt();
				e.printStackTrace();
			}
			
			if(outputFrame != null){
				
				if(outputFrame == Settings.STOP){
					break loop;
				}
				
				if(Settings.playback) {
					sound.write(outputFrame.getAudio(), 0, Settings.stepSize);
				}
				
				if(saveData){
					try {
						audioWriter.write(Utilities.getByteArray(outputFrame.getAudio()));
					} catch (IOException e) {
						e.printStackTrace();
					}
				}
				
				if(debugData){
					try {
						debugWriter.write((Arrays.toString(outputFrame.getDebug()).replace("[", "").replace("]","")+"\n").getBytes());
					} catch (IOException e) {
						e.printStackTrace();
					}
				}
				
				try {
					recycleQueue.put(outputFrame);
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
			}
		}

		main.done();
		
		if(sound != null){
			sound.pause();
			sound.flush();
			sound.release();
		}
		
		if(saveData){
			try {
				audioWriter.flush();
				audioWriter.close();
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
		if(debugData){
			try {
				debugWriter.flush();
				debugWriter.close();
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
	}
}