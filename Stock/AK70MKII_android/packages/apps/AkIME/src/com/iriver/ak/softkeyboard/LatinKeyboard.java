/*
 * Copyright (C) 2008-2009 Google Inc.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

package com.iriver.ak.softkeyboard;

import android.content.Context;
import android.content.res.Resources;
import android.content.res.XmlResourceParser;
import android.graphics.drawable.Drawable;
import android.inputmethodservice.Keyboard;
import android.os.Build;
import android.util.Log;
import android.view.inputmethod.EditorInfo;

public class LatinKeyboard extends Keyboard {

	public static final short HANGUL_BACKSPACE = 30;
	public static final int KEYCODE_SPACE = 32;
	public static final int KEYCODE_ENTER = 10;
	public static final int KEYCODE_GLOBAL = -101;
	public static final int KEYCODE_CLOSE = -220; // hide key
	
	// Change, 180227 [-5 -> -6]
	public static final int KEYCODE_BACKSPACE = -6; // [-5] -> [-6]
	
	public static final int KEYCODE_OPTIONS = -100;
	public static final int KEYCODE_SYMBOL = -2;

	public static final int KEYCODE_EMPTY_BLANK = 900;
	public static final int KEYCODE_SYMBOL_01 = 901;
	public static final int KEYCODE_SYMBOL_02 = 902;
	public static final int KEYCODE_SYMBOL_03 = 903;
	public static final int KEYCODE_SYMBOL_04 = 904;
	public static final int KEYCODE_SYMBOL_05 = 905;
	public static final int KEYCODE_SYMBOL_06 = 906;
	public static final int KEYCODE_SYMBOL_07 = 907;
	public static final int KEYCODE_SYMBOL_08 = 908;
	public static final int KEYCODE_SYMBOL_09 = 909;
	public static final int KEYCODE_SYMBOL_10 = 910;
	public static final int KEYCODE_SYMBOL_11 = 911;
	public static final int KEYCODE_SYMBOL_12 = 912;
	public static final int KEYCODE_SYMBOL_13 = 913;
	public static final int KEYCODE_SYMBOL_14 = 914;
	public static final int KEYCODE_SYMBOL_15 = 915;

	public static final int KEYCODE_ENTER_SEARCH_KEY = 1001;

	private Drawable mEnterPreviewIcon = null; // yunsuk 2014.03.13

	private Keyboard mBeforeKeyboard = null;
	private Key mCompleteKey = null;
	private Key mShiftKey = null; // yunsuk 2014.02.18

	private String mGlobalText = "";
	private int mImeOptions;
	private int mXmlResId;

	public LatinKeyboard(Context context, int xmlLayoutResId) {
		super(context, xmlLayoutResId);
		mXmlResId = xmlLayoutResId;
		setGlobalKeyImage(context);
	}

	public LatinKeyboard(Context context, int layoutTemplateResId,
			CharSequence characters, int columns, int horizontalPadding) {
		super(context, layoutTemplateResId, characters, columns,
				horizontalPadding);
	}

	public LatinKeyboard(Context context, int xmlLayoutResId, int mode) {
		super(context, xmlLayoutResId, mode);
		mXmlResId = xmlLayoutResId;
	}

	public void setBeforeKeyboard(Keyboard current) {
		mBeforeKeyboard = current;
	}

	public Keyboard getBeforeKeyboard() {
		return mBeforeKeyboard;
	}

	public int getXmlResId() {
		return mXmlResId;
	}

	public int getImeOptions() {
		return mImeOptions;
	}
	
	public void setChangeKeyText(String keyText, int keycode) {
		for (Key k : getKeys()) {
			if (k.codes[0] == keycode)
				k.label = keyText;
		}
	}

	
	
	public void setShiftIcon(boolean isShiftLock, Drawable icon) {
		if (mShiftKey != null) {
			mShiftKey.on = isShiftLock;
			// mShiftKey.pressed = isShiftLock;
			if (icon != null)
				mShiftKey.icon = icon;
		} else {
			for (Key k : getKeys()) {
				if (k.codes[0] == Keyboard.KEYCODE_SHIFT) {
					mShiftKey = k;
					setShiftIcon(isShiftLock, icon);
					break;
				}
			}
		}
	}

	public Key getShiftKey() {
		return mShiftKey;
	}

	@Override
	protected Key createKeyFromXml(Resources res, Row parent, int x, int y,
			XmlResourceParser parser) {
		Key key = new LatinKey(res, parent, x, y, parser);

		if (key.codes[0] == KEYCODE_ENTER) {
			mCompleteKey = key;
			
			// 
			String model = Build.MODEL;
			Log.d("LatinKeyboard", "createKeyFromXml model = " + model);
			if (Build.MODEL_SE200.equals(model)) {
				mCompleteKey.icon = res.getDrawable(R.drawable.key_enter_bg_n_se200);
			} else { 	
				mCompleteKey.icon = res.getDrawable(R.drawable.key_enter_bg_n);
			}
		}
		return key;
	}

	public void setGlobalText(String text) {
		mGlobalText = text;
	}
	public String getGlobalText() {
		return mGlobalText;
	}

	private void setGlobalKeyImage(Context context) {
		switch (mXmlResId) {
		case R.xml.japan_12key_hira:
		case R.xml.japan_12key_number:
		case R.xml.japan_12key_s_qwerty:
		case R.xml.japan_12key_symbol:
		case R.xml.japan_qwerty:
			mGlobalText = context.getResources().getString(
					R.string.keyboard_lan_jpn);
			break;
		case R.xml.hangul:
		case R.xml.hangul_shift:
			mGlobalText = context.getResources().getString(
					R.string.keyboard_lan_kor);
			break;
		case R.xml.qwerty:
		case R.xml.us_qwerty:
			mGlobalText = context.getResources().getString(
					R.string.keyboard_lan_eng);
			break;
		case R.xml.changjie:
			mGlobalText = context.getResources().getString(
					R.string.keyboard_lan_tw);
			break;
		case R.xml.pinyin_qwerty:
			mGlobalText = context.getResources().getString(
					R.string.keyboard_lan_cn);
			break;
		case R.xml.ru_rus:
			mGlobalText = context.getResources().getString(
					R.string.keyboard_lan_rus);
			break;
		}
	}

	/**
	 * This looks at the ime options given by the current editor, to set the
	 * appropriate label on the keyboard's enter key (if it has one).
	 */
	void setImeOptions(Resources res, int options) {

		mImeOptions = options;

		if (options == KEYCODE_ENTER_SEARCH_KEY) {
			if (mCompleteKey == null) {
				for (Key key : getKeys()) {
					if (key.codes[0] == KEYCODE_ENTER) {
						if (mEnterPreviewIcon == null)
							mEnterPreviewIcon = key.iconPreview;
						mCompleteKey = key;
						break;
					}
				}
			}
			if (mCompleteKey != null) {
				mCompleteKey.iconPreview = null;
				mCompleteKey.icon = res
						.getDrawable(R.drawable.btn_search_a);
				return;
			}
		}

		if (mCompleteKey == null)
			return;

		switch (options
				& (EditorInfo.IME_MASK_ACTION | EditorInfo.IME_FLAG_NO_ENTER_ACTION)) {
		case EditorInfo.IME_ACTION_GO:
		case EditorInfo.IME_ACTION_NEXT:
		case EditorInfo.IME_ACTION_SEARCH:
		case EditorInfo.IME_ACTION_SEND:
		default:
			mCompleteKey.iconPreview = mEnterPreviewIcon;
			mCompleteKey.icon = res.getDrawable(R.drawable.key_enter_bg_n);
			break;
		}
	}

	static class LatinKey extends Keyboard.Key {
		public LatinKey(Resources res, Keyboard.Row parent, int x, int y,
				XmlResourceParser parser) {
			super(res, parent, x, y, parser);
		}

		@Override
		public boolean isInside(int x, int y) {
			return super.isInside(x, codes[0] == KEYCODE_CANCEL ? y - 10 : y);
		}
	}
}
