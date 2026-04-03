package com.iriver.ak.softkeyboard;

import java.util.ArrayList;

public class KeyboardSwitcher {

	public static final int INDEX_DEFAULT = 0;

	public static final int KEYBOARD_TYPE_MAIN = 1;
	public static final int KEYBOARD_TYPE_ENG = 2;
	public static final int KEYBOARD_TYPE_SYM = 3;
	public static final int KEYBOARD_TYPE_NUM = 4;
	public static final int KEYBOARD_TYPE_KOR = 5;
	public static final int KEYBOARD_TYPE_CHS = 6;
	public static final int KEYBOARD_TYPE_CHT = 7;
	public static final int KEYBOARD_TYPE_RUS = 8;
	public static final int KEYBOARD_TYPE_JPN = 9;
	public static final int KEYBOARD_TYPE_JP_QWERTY = 10;
	public static final int KEYBOARD_TYPE_JP_NUMBER = 11;
	public static final int KEYBOARD_TYPE_JP_SYMBOL = 12;
	public static final int KEYBOARD_TYPE_JP_HIRA_QERTY = 13;
	

	private ArrayList<KeyboardItem> mKeyboardList = new ArrayList<KeyboardItem>();
	private int mIndex = INDEX_DEFAULT;

	public KeyboardSwitcher() {
	}

	public void setDefaultKeyboardIndex() {
		mIndex = INDEX_DEFAULT;
	}

	public void setKeyboardIndex(int index) {
		mIndex = index;
	}

	public int size() {
		return mKeyboardList.size();
	}

	public void clear() {
		mIndex = INDEX_DEFAULT;
		if (mKeyboardList != null)
			mKeyboardList.clear();
	}

	public void addKeyboard(LatinKeyboard keyboard, int type) {
		if (mKeyboardList != null && !mKeyboardList.contains(keyboard)
				&& isCompare(keyboard)) {
			KeyboardItem item = new KeyboardItem(keyboard, type);
			mKeyboardList.add(item);
		}
	}

	private boolean isCompare(LatinKeyboard keyboard) {
		for (KeyboardItem item : mKeyboardList) {
			if (item.keyboard == keyboard)
				return false;
		}
		return true;
	}

	private boolean isCompare(int type) {
		for (KeyboardItem item : mKeyboardList) {
			if (type == item.type)
				return false;
		}
		return true;
	}

	private LatinKeyboard getFindKeyboard(int type) {
		for (int i = 0; i < mKeyboardList.size(); i++) {
			KeyboardItem item = mKeyboardList.get(i);

			if (item.type == type) {
				mIndex = i;
				return item.keyboard;
			}
		}
		return getKeyboard(0);
	}

	public LatinKeyboard getSymbolKeyboard() {
		return getFindKeyboard(KEYBOARD_TYPE_SYM);
	}

	public LatinKeyboard getQwertyKeyboard() {
		return getFindKeyboard(KEYBOARD_TYPE_ENG);
	}

	public LatinKeyboard next(int resId) {

		int size = mKeyboardList.size();
		int nextIndex = mIndex + 1;

		if (nextIndex < size) {
			if (resId == mKeyboardList.get(nextIndex).getKeyboard()
					.getXmlResId()) {
				++mIndex;
			}
		}
		++mIndex;

		if (mIndex >= size) {
			mIndex = INDEX_DEFAULT;
		}
		return getSelectKeyboard();
	}
	
	public LatinKeyboard prev() {
		int prevIndex = mIndex - 1;
		
		if(prevIndex >= 0) {
			mIndex = prevIndex;
		}
		
		return getSelectKeyboard();
	}

	public void setDefaultKeyboard(LatinKeyboard keyboard) {
		if (mKeyboardList != null && mKeyboardList.size() > INDEX_DEFAULT) {
			mIndex = INDEX_DEFAULT;
			mKeyboardList.get(INDEX_DEFAULT);
		}
	}

	public LatinKeyboard getDefaultKeyboard() {
		if (mKeyboardList != null && mKeyboardList.size() > INDEX_DEFAULT) {
			return mKeyboardList.get(INDEX_DEFAULT).keyboard;
		}
		return null;
	}

	public LatinKeyboard getKeyboard(int index) {
		if (mKeyboardList != null && mKeyboardList.size() > index)
			return mKeyboardList.get(index).keyboard;
		return null;
	}

	public LatinKeyboard getSelectKeyboard() {
		if (mKeyboardList != null && mKeyboardList.size() > mIndex && mIndex >= 0)
			return mKeyboardList.get(mIndex).keyboard;
		return null;
	}

	class KeyboardItem {
		public LatinKeyboard keyboard = null;
		public int type = KEYBOARD_TYPE_MAIN;

		public KeyboardItem(LatinKeyboard keyboard, int type) {
			// TODO Auto-generated constructor stub
			this.keyboard = keyboard;
			this.type = type;
		}

		public LatinKeyboard getKeyboard() {
			return keyboard;
		}
	}
}
