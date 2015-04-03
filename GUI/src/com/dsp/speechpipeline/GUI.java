package com.dsp.speechpipeline;

import java.io.File;
import java.io.FilenameFilter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.atomic.AtomicBoolean;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.content.res.Configuration;
import android.os.Bundle;
import android.widget.Button;
import android.widget.ScrollView;
import android.view.View;
import android.widget.TextView;

public class GUI extends Activity implements Monitor{
	
	private SharedPreferences preferences;
	private WaveRecorder waveRecorder;
	private WaveReader waveReader;
	private TextView logBox;
	private ScrollView logScrollView;
	private List<Integer> lineSize;
	
	private String[] fileList;
	private String dialogFile;	
	
	private int mode;
	private AtomicBoolean processing;
	
	private BlockingQueue<WaveFrame> recycleQueue;
	private BlockingQueue<WaveFrame> inputFrames;
	private BlockingQueue<WaveFrame> processedFrames;
	
    public void onCreate(Bundle savedInstanceState) {
    	
    	super.onCreate(savedInstanceState);
    	
    	//get asset manager
    	Settings.setAssetManager(getAssets());
    	
    	//set the application layout
        setContentView(R.layout.recorder);
        
        Settings.setCallbackInterface(this);
        
        //hook application preferences interface
        preferences = PreferenceManager.getDefaultSharedPreferences(this);
        
        //setup ListPreference values
    	Settings.debugOutputNames = getResources().getStringArray(R.array.debugTextOptions);
    	Settings.debugLevels = getResources().getStringArray(R.array.debugOptions);
    	Settings.audioOutputs = getResources().getStringArray(R.array.outputOptions);
        
        //hook UI elements
        setupListeners();
        
        //setup the textview 
        lineSize = new ArrayList<Integer>();
        logBox = (TextView)findViewById(R.id.LogView);
        logScrollView = (ScrollView)findViewById(R.id.LogScrollView);
        
        //get saved application settings
        updateSettings();
        settingSummary();
        Settings.changed = false;
        
        //create directory to save data into
        //application directory is defined in res/values/strings.xml
        Utilities.prepareDirectory(getString(R.string.appDirectory));
        
        //Check for supported sampling rates
        new Thread(WaveRecorder.checkSamplingRate).start();
        
        //prepare to process data
        processing = new AtomicBoolean();
        
        //create queues for data to be stored in during processing
        recycleQueue = new ArrayBlockingQueue<WaveFrame>(Settings.queueSize);
        inputFrames = new ArrayBlockingQueue<WaveFrame>(Settings.queueSize);
        processedFrames = new ArrayBlockingQueue<WaveFrame>(Settings.queueSize);
        
        enableButtons(false);
    }
    
    private void setupListeners() {
    	((Button)findViewById(R.id.buttonSettings)).setOnClickListener(buttonClick);
    	((Button)findViewById(R.id.buttonStart)).setOnClickListener(buttonClick);
    	((Button)findViewById(R.id.buttonStop)).setOnClickListener(buttonClick);
    	((Button)findViewById(R.id.buttonPCM)).setOnClickListener(buttonClick);
    }

	private void enableButtons(boolean flag) {
		((Button)findViewById(R.id.buttonSettings)).setEnabled(!flag);
		((Button)findViewById(R.id.buttonStart)).setEnabled(!flag);
		((Button)findViewById(R.id.buttonStop)).setEnabled(flag);
		
		if(flag){
			((Button)findViewById(R.id.buttonPCM)).setText(Settings.getOutput());
		} else {
			((Button)findViewById(R.id.buttonPCM)).setText(getString(R.string.file));
		}
	}
    
	private View.OnClickListener buttonClick = new View.OnClickListener() {
		public void onClick(View v) {
			switch (v.getId()) {
				case R.id.buttonStart: {
					buttonStart();
					break;
				}
				case R.id.buttonStop: {
					buttonStop();
					break;
				}
				case R.id.buttonPCM: {
					buttonPCM();
					break;
				}
				case R.id.buttonSettings: {
					buttonSettings();
					break;
				}
			}
		}
	};
	
	private void buttonStart(){
		
		//setup data processing
		inputFrames.clear();
    	processedFrames.clear();
    	recycleQueue.clear();
        for(int i=0;i<Settings.queueSize;i++){
        	recycleQueue.add(new WaveFrame());
        }
    	new Processing(inputFrames, processedFrames);
    	
		if(dialogFile == null){
	        //setup audio recording component
	    	waveRecorder = new WaveRecorder(recycleQueue, inputFrames);
	    	mode = 2;
	    	appendTextView("Recording and processing.");
			new WaveSaver(getString(R.string.appDirectory) + File.separator + System.currentTimeMillis(), processedFrames, recycleQueue);
		} else {
			mode = 1;
			appendTextView("Reading from file: " + dialogFile);
			waveReader = new WaveReader(getString(R.string.appDirectory) + File.separator + dialogFile, inputFrames, recycleQueue);
			new WaveSaver((getString(R.string.appDirectory) + File.separator + dialogFile.replace(".wav", "")).replace(".pcm", ""), processedFrames, recycleQueue);
		}
		
		processing.set(true);
		enableButtons(true);
	}
	
	private void buttonStop(){
		switch(mode) {
			case 1: {
				waveReader.stop();
				waveReader = null;
				break;
			}
			case 2: {
				waveRecorder.stopRecording();
				waveRecorder = null;
				break;
			}
			default: {
				break;
			}
		}
	}
	
	private void buttonPCM(){
		if(!processing.get()){
			getDirectoryContents();
			createFilePrompt().show();
		} else {
			if(Settings.output.get() == 0) {
				Settings.setOutput(1);
				preferences.edit().putString(getString(R.string.prefOutputStream), Settings.audioOutputs[1]).commit();
			} else {
				Settings.setOutput(0);
				preferences.edit().putString(getString(R.string.prefOutputStream), Settings.audioOutputs[0]).commit();
			}
			((Button)findViewById(R.id.buttonPCM)).setText(Settings.getOutput());
			String[] result = {"Playback set to original output.", "Playback set to filtered output."};
			notify(result[Settings.output.get()]);
	        Settings.changed = false;
		}
	}
	
	private void buttonSettings(){
		Intent intent = new Intent(this, PreferencesUI.class);
		startActivityForResult(intent, 42);
	}
	
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);

		switch (requestCode) {
		case 42:
			if(Settings.changed){
				settingSummary();
				Settings.changed = false;
			}
			break;
		}
	}
	
	public void onBackPressed() {
	    if(processing.get()){
	    	buttonStop();
	    }
	    super.onBackPressed();
	}

	private void updateSettings(){
		Settings.setPlayback(preferences.getBoolean(getString(R.string.prefPlayback), false));
		Settings.setOutput(Arrays.asList(Settings.audioOutputs).indexOf((preferences.getString(getString(R.string.prefOutputStream), Settings.audioOutputs[0]))));
		Settings.setSamplingFrequency(Integer.parseInt(preferences.getString(getString(R.string.prefSamplingFreq), "8000")));
		Settings.setWindowSize(Float.parseFloat(preferences.getString(getString(R.string.prefWindowTime), "11.0")));
		Settings.setStepSize(Float.parseFloat(preferences.getString(getString(R.string.prefStepTime), "5.0")));
		Settings.setDecisionBufferLength(Integer.parseInt(preferences.getString(getString(R.string.prefDecisionBufferLength), "200")));
		Settings.setDebugLevel(Arrays.asList(Settings.debugLevels).indexOf((preferences.getString(getString(R.string.prefDebug), Settings.debugLevels[0]))));
		Settings.setDebugOutput(Arrays.asList(Settings.debugOutputNames).indexOf((preferences.getString(getString(R.string.prefDebugOutput), Settings.debugOutputNames[0]))));
	}
	
	//otherwise, everything resets when the screen is changed.
	public void onConfigurationChanged(Configuration newConfig) {
		  super.onConfigurationChanged(newConfig);
	}
	
	//File selection dialog methods.
	private void getDirectoryContents() {
	    File directory = Utilities.prepareDirectory(getString(R.string.appDirectory));
	    if(directory.exists()) {
	        FilenameFilter filter = new FilenameFilter() {
	            public boolean accept(File dir, String filename) {
	                return filename.contains(".pcm") || filename.contains(".wav");
	            }
	        };
	        fileList = directory.list(filter);
	    } else {
	        fileList = new String[0];
	    }
	}
	
	private Dialog createFilePrompt() {
	    AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Choose file");
        builder.setItems(fileList, 
	        new DialogInterface.OnClickListener() {
	            public void onClick(DialogInterface dialog, int which) {
	                dialogFile = fileList[which];
	                buttonStart();
	            }
	        }
        );
	    return builder.create();
	}
	
	private void appendTextView(String text){
		text = "\n" + text ;
		lineSize.add(Integer.valueOf(text.length()));
		logBox.append(text);
		while(lineSize.size() > 250) {
			logBox.getEditableText().delete(0, lineSize.get(0).intValue());
			lineSize.remove(0);
		}
		
		logScrollView.post(new Runnable(){
			public void run() {
				logScrollView.fullScroll(View.FOCUS_DOWN);
			}
		});
	}
	
	private void settingSummary(){
		appendTextView("Rate: " + Settings.Fs + "Hz | Window: " + Settings.windowTime + "ms | Step: " + Settings.stepTime + "ms");
		appendTextView("Samples - Window: " + Settings.windowSize + " | Step: " + Settings.stepSize + " | Overlap: " + (Settings.windowSize - Settings.stepSize));
        appendTextView("Audio: " + Settings.getOutput() + " | Debug: " + Settings.getDebugLevel() + ((Settings.debugLevel==2 || Settings.debugLevel==4)?" | Text: " + Settings.getDebugOutput():""));
        if(Settings.playback){
            appendTextView("Audio playback enabled.");
        }
	}
	
	public int getMode(){
		return mode;
	}
	
	public void done(){
		dialogFile = null;
		if(processing.getAndSet(false)){
			runOnUiThread(
				new Runnable(){
					 public void run() {
						 if(mode == 1) {
							 appendTextView("File read completed.");
						 } else if (mode == 2) {
							 appendTextView("Recording finished.");
						 }
						 enableButtons(false);
					 }
				}
			);
		}
	}
	
	public synchronized void notify(final String message){
		runOnUiThread(
			new Runnable(){
				 public void run() {
					 appendTextView(message);
				 }
			}
		);
	}	
	static {
		System.loadLibrary("SpeechProcessing");
	}
}
