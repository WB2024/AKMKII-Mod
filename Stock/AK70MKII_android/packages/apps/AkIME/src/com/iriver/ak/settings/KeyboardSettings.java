package com.iriver.ak.settings;

import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceActivity;

public class KeyboardSettings extends PreferenceActivity implements Preference.OnPreferenceChangeListener{
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
	}
	
	@Override
	public boolean onPreferenceChange(Preference preference, Object newValue) {
		// TODO Auto-generated method stub
		return false;
	}
}
