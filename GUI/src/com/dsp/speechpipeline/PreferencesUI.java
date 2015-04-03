package com.dsp.speechpipeline;

import android.os.Bundle;
import android.preference.PreferenceActivity;

public class PreferencesUI extends PreferenceActivity{
	
	protected void onCreate(Bundle savedInstanceState){
		super.onCreate(savedInstanceState);
		getFragmentManager().beginTransaction().replace(android.R.id.content, new PreferencesFragment()).commit();
	}
}
