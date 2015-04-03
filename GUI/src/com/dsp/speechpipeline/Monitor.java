package com.dsp.speechpipeline;

public interface Monitor {
	public int getMode();
	public void done();
	public void notify(String message);
}
