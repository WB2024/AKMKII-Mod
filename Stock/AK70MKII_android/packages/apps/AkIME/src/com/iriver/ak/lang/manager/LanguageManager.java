package com.iriver.ak.lang.manager;

public class LanguageManager {

	public static final int KEYBOARD_LANGUAGE_ERROR = -1;
	public static final int KEYBOARD_LANGUAGE_EN = 1;
	public static final int KEYBOARD_LANGUAGE_JP = 3;
	public static final int KEYBOARD_LANGUAGE_CN = 4;
	public static final int KEYBOARD_LANGUAGE_TW = 5;
	public static final int KEYBOARD_LANGUAGE_RU = 6;
	
	public static String mLocalLanguageJp = "ja";
	public static String mLocalCountryJp = "JP";
	public static String mLocalLanguageCn = "zh";
	public static String mLocalCountryCn = "CN";
	public static String mLocalLanguageTw = "zh";
	public static String mLocalCountryTw = "TW";
	public static String mLocalLanguageRu = "ru";
	public static String mLocalCountryRu = "RU";
	
	
	public static int getLocalLanguage(String language, String country){
		if ((language.equals(mLocalLanguageJp) && country.equals(mLocalCountryJp))) {
			return KEYBOARD_LANGUAGE_JP;
		}else if ((language.equals(mLocalLanguageCn) && country.equals(mLocalCountryCn))) {
			return KEYBOARD_LANGUAGE_CN;
		}else if ((language.equals(mLocalLanguageTw) && country.equals(mLocalCountryTw))) {
			return KEYBOARD_LANGUAGE_TW;
		}else if ((language.equals(mLocalLanguageRu) && country.equals(mLocalCountryRu))) {
			return KEYBOARD_LANGUAGE_RU;
		}else{
			return KEYBOARD_LANGUAGE_EN;
		}
	}
}
