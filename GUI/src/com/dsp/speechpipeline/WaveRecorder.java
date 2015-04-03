package com.dsp.speechpipeline;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.atomic.AtomicBoolean;

import android.media.AudioFormat;
import android.media.AudioRecord;

public class WaveRecorder implements Runnable{

	private WaveFrame out;
	private AtomicBoolean isRecording;
	private BlockingQueue<WaveFrame> recycleQueue;
	private BlockingQueue<WaveFrame> output;
	private AudioRecord recorder;
	private int recBufLen;
	private Thread recThread;
	
	public WaveRecorder(BlockingQueue<WaveFrame> recycleQueue, BlockingQueue<WaveFrame> output){
		this.output = output;
		this.recycleQueue = recycleQueue;
		isRecording = new AtomicBoolean();
		
		recBufLen = AudioRecord.getMinBufferSize(Settings.Fs, AudioFormat.CHANNEL_IN_MONO, Settings.FORMAT);
		recorder = new AudioRecord(Settings.SOURCE, Settings.Fs, AudioFormat.CHANNEL_IN_MONO, Settings.FORMAT, recBufLen);
		
		recThread = new Thread(this);
		recThread.start();
	}
	
	public void stopRecording(){
		isRecording.set(false);
	}
	
	public boolean isRecording(){
		return isRecording.get();
	}

	public void run() {
        android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO);
        
        try{
        	if(recorder != null){
        		recorder.stop();
        		recorder.startRecording();
        		isRecording.set(true);
        	}
        } catch (IllegalStateException e){
        	isRecording.set(false);
        }
        
		try {
			loop:while(true){
				if(isRecording.get()) {
					out = recycleQueue.take();
					recorder.read(out.getAudio(), 0, Settings.stepSize);
					output.put(out);
				} else {
					output.put(Settings.STOP);
					break loop;
				}
			}
		} catch (InterruptedException e) {
			Thread.currentThread().interrupt();
			e.printStackTrace();
		}
		
		try{
			if(recorder != null){
				recorder.stop();
	        	recorder.release();
	        	recorder = null;
			}
        } catch (IllegalStateException e){
        	e.printStackTrace();
        }
		
	}
	
    public static Runnable checkSamplingRate = new Runnable(){
    	public void run(){
    		int[] rates =  {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 96000, 192000};
        	List<String> listRateValues = new ArrayList<String>();
        	List<String> listRates = new ArrayList<String>();
        	AudioRecord testRecorder = null;
        	boolean good = false;
        	
        	for (int rate : rates) {
        	    int bufferSize = AudioRecord.getMinBufferSize(rate, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT);
    	    	if (bufferSize != AudioRecord.ERROR || bufferSize != AudioRecord.ERROR_BAD_VALUE) {
    	    		//Sampling rate should be supported
    	    		
    	    		try{
    	    			good = true;
    	    			testRecorder = new AudioRecord(Settings.SOURCE, rate, AudioFormat.CHANNEL_IN_MONO, Settings.FORMAT, bufferSize);
    	    			testRecorder.stop();
    	    			testRecorder.release();
    		    		testRecorder = null;
    	    		}catch(Exception e){
    	    			good = false;
    	    		}
    	    		
    	    		if(good){
    		    		listRateValues.add(Integer.toString(rate));
    		    		listRates.add(rate + " Hz");
    	    		}
    	    	}
        	}
        	Settings.setRates(listRates.toArray(new CharSequence[listRates.size()]));
        	Settings.setRateValues(listRateValues.toArray(new CharSequence[listRateValues.size()]));
    	}   	
	};
}
