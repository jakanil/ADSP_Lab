package com.dsp.speechpipeline;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.atomic.AtomicBoolean;

public class WaveReader implements Runnable{
	
	private long nFrames;
	private long currentIndex;
	private File waveFile;
	private FileInputStream stream;
	private byte[] abuf_byte;
	private short[] abuf_short;
	private ByteBuffer buffer;
	private BlockingQueue<WaveFrame> recycleQueue;
	private BlockingQueue<WaveFrame> output;
	private WaveFrame out;
	private AtomicBoolean read;
	private Thread reader;
	
	public WaveReader(String waveFileName, BlockingQueue<WaveFrame> output, BlockingQueue<WaveFrame> recycleQueue) {
		this.output = output;
		this.recycleQueue = recycleQueue;
		read = new AtomicBoolean(true);
		waveFile = Utilities.getFile(waveFileName);
		nFrames = (waveFile.length())/(2*Settings.stepSize);
		currentIndex = 0;
		
		try {
			stream = new FileInputStream(waveFile);
			if(waveFile.getName().contains(".wav")){
				stream.skip(44);
			}
		} catch (IOException e) {
			e.printStackTrace();
		}
		
		abuf_byte = new byte[2*Settings.stepSize];
		abuf_short = new short[Settings.stepSize];
		
		buffer = ByteBuffer.wrap(abuf_byte);
		buffer.order(ByteOrder.LITTLE_ENDIAN);
		
        reader = new Thread(this);
        reader.start();
	}
	
	public void stop(){
		read.set(false);
	}
	
	public void run() {
		while (read.get()) {
			try {
				stream.read(abuf_byte);
			} catch (IOException e) { 
				e.printStackTrace(); 
			}
			for(int i=0;i<Settings.stepSize;i++) {
				abuf_short[i] = buffer.getShort(2*i);
			}
			
			try {
				out = recycleQueue.take();
				out.setAudio(abuf_short);
				output.put(out);
			} catch (InterruptedException e) {
				e.printStackTrace();
				Thread.currentThread().interrupt();
			}
			
			currentIndex++;
			if(currentIndex == nFrames){
				read.set(false);
			}
		}
		
		try {
			output.put(Settings.STOP);
		} catch (InterruptedException e) {
			e.printStackTrace();
			Thread.currentThread().interrupt();
		}
		
		try {
			stream.close();
		} catch (IOException e) { 
			e.printStackTrace(); 
		}
	}
}
