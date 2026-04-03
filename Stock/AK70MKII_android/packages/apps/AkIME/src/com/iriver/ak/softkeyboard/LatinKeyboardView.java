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

import java.util.ArrayList;
import java.util.List;

import com.iriver.ak.lang.manager.JapaneseManager;
import com.iriver.ak.popup.PopupKeyboardEn;
import com.iriver.ak.popup.PopupKeyboardEn.popupKeyboardListener;
import com.iriver.ak.popup.PopupKeyboardJpn;
import com.iriver.ak.popup.PopupKeyboardJpn.onJpnPopupKeyboardListener;
import com.iriver.ak.softkeyboard.R;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.inputmethodservice.Keyboard;
import android.inputmethodservice.Keyboard.Key;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Gravity;
import android.view.MotionEvent;

public class LatinKeyboardView extends KeyboardView {

	private ArrayList<Key> mModifierKeys = null;
	private ArrayList<Key> mBlankKeys = null;

	private boolean mIsLimitKeyRegion = false;
	private Point mKeyDownXY = new Point(0, 0);
	private Rect mKeyBounds = new Rect();
	private int m_x = 0;
	private int m_y = 0;

	/** Eng Keyboard **/
	private PopupKeyboardEn mRomajiPopup = null;
	
	/** Jpn **/
	private PopupKeyboardJpn mPopupJpn = null;
	private boolean isLongClick_Jpn = false;
	private boolean mIsLongPress = false;

	/** Animation (AK500N) **/
	private boolean mIsAnimTextStart = true;
	private long mAnimTextStartTime = 0;
	private Paint mAnimKeyPaint = null;
	private Paint mAnimTextPaint = null;

	
	public LatinKeyboardView(Context context, AttributeSet attrs) {
		super(context, attrs);
		mContext = context;
	}

	public LatinKeyboardView(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		mContext = context;
	}


	private void initAnimKeyText() {
		mIsAnimTextStart = true;
		mAnimTextStartTime = 0;
		
		if (mAnimTextPaint == null) {
			mAnimTextPaint = new Paint();
			mAnimTextPaint.setTextSize(getResources().getDimension(
					R.dimen.font_size_space));
			mAnimTextPaint.setColor(0xccffffff);
			mAnimTextPaint.setAntiAlias(true);
		}
		if (mAnimTextPaint != null) {
			mAnimTextPaint.setAlpha(255);
		}
		
		if (mAnimKeyPaint == null) {
			mAnimKeyPaint = new Paint();
		}
		if (mAnimKeyPaint != null) {
			mAnimKeyPaint.setAlpha(0);
		}
	}
	
	@Override
	public boolean onTouchEvent(MotionEvent me) {
		// TODO Auto-generated method stub

		m_x = (int) me.getX();
		m_y = (int) me.getY();

		if (mBlankKeys != null) {
			for (Key key : mBlankKeys) {
				if (key.isInside(m_x, m_y)) {
					return true;
				}
			}
		}

		switch (me.getAction()) {
		case MotionEvent.ACTION_DOWN:

			mKeyDownXY.set((int) me.getX(), (int) me.getY());

			// search modifier key
			mIsLimitKeyRegion = isModifierKeyContains(m_x, m_y);

			
			break;
		case MotionEvent.ACTION_MOVE:

			if (isLongClick_Jpn) {
				mPopupJpn
						.showDirectButton(mKeyDownXY.x, mKeyDownXY.y, m_x, m_y);
				return false;
			} else if (mIsLimitKeyRegion) {
				return true;
			}

			break;
		case MotionEvent.ACTION_UP:
//			dismissPreviewPopup();

			if (isLongClick_Jpn) {
				boolean isSimejiCode = mPopupJpn.SimejiDismissVirtualPopupKey();

				if (isSimejiCode == false && m_y < 0)
					me.setAction(MotionEvent.ACTION_CANCEL);

			} else if (mIsLongPress) {
				mIsLongPress = false;
				me.setAction(MotionEvent.ACTION_CANCEL);
				return false;

			} else if (m_y < 0 && isLongClick_Jpn == false) {
				me.setAction(MotionEvent.ACTION_CANCEL);
			}

			// init variable
			mIsLimitKeyRegion = false;

			keyboardViewListener.onTouchUp(m_x, m_y);

			break;
		default:
			Log.e(getClass().getSimpleName(),
					"### ERROR TouchEvent : " + me.getAction());
			break;
		}
		return super.onTouchEvent(me);
	}

	public void KeyInit() {
		/** Modifier **/
		if (mModifierKeys == null) {
			mModifierKeys = new ArrayList<Key>();
		}
		if (!mModifierKeys.isEmpty()) {
			mModifierKeys.clear();
		}

		/** Blank Key **/
		if (mBlankKeys == null) {
			mBlankKeys = new ArrayList<Keyboard.Key>();
		}
		if (!mBlankKeys.isEmpty()) {
			mBlankKeys.clear();
		}

		List<Key> keys = getKeyboard().getKeys();
		for (Key key : keys) {
			if (key.modifier) {
				mModifierKeys.add(key);
			}
			if (key.codes[0] == LatinKeyboard.KEYCODE_EMPTY_BLANK) {
				mBlankKeys.add(key);
			}
		}

		/** Init Spacebar Language Animation**/
			initAnimKeyText();
		}

	private Key getKey(int x, int y) {
		List<Key> keys = getKeyboard().getKeys();
		for (Key key : keys) {
			if(key.isInside(x, y)) {
				return key;
			}
		}
		return null;
	}

	private boolean isModifierKeyContains(int x, int y) {
		for (Key key : mModifierKeys) {
			if (key.isInside(x, y)) {
				return true;
			}
		}
		return false;
	}

	// //////////////////////////////////////////////////////////////////////////////////
	// LongPress
	// //////////////////////////////////////////////////////////////////////////////////
	@Override
	protected boolean onLongPress(Key key) {

		/** Simeji 12Key **/ 
		if (SimejiShowVirtualPopupKey(key)) {
			isLongClick_Jpn = true;
			return true;
		}

		/** Romaji Popup **/
		if (isRomajiPopup(key)) {
			mIsLongPress = true;
			showRomajiPopupKeyboard(key);
			return true;
		}

		if (key.codes[0] == Keyboard.KEYCODE_CANCEL) {
			getOnKeyboardActionListener().onKey(LatinKeyboard.KEYCODE_OPTIONS,
					null);
			return true;
		} else if (key.codes[0] == Keyboard.KEYCODE_SHIFT) {
			keyboardViewListener.onLongPressKey(key);
		}
		return super.onLongPress(key);
	}

	public boolean SimejiShowVirtualPopupKey(Key key) {
		
		if (isSimejiMode()
				&& ((key.codes[0] >= JapaneseManager.KEYCODE_HIRA_START 
				&& key.codes[0] <= JapaneseManager.KEYCODE_HIRA_END) 
				|| key.codes[0] == JapaneseManager.KEYCODE_SYMBOL_START)) {

			if (mPopupJpn == null) {
				mPopupJpn = new PopupKeyboardJpn(getContext());
				mPopupJpn
						.setOnJpnPopupKeyboardListener(new onJpnPopupKeyboardListener() {

							@Override
							public void setKey(int code, int codes[]) {
								// TODO Auto-generated method stub
								getOnKeyboardActionListener()
										.onKey(code, codes);
							}

							@Override
							public void onSimejiInputKey(String keyText) {
								// TODO Auto-generated method stub
								keyboardViewListener.onSimejiInputKey(keyText);
							}
						});

			}
			mPopupJpn.init(getContext(), key, getWidth(), getHeight());

			float btnSize = getResources().getDimension(R.dimen.jp_popup_btn_size);
			float btnPadding = getResources().getDimension(R.dimen.jp_popup_btn_padding_size);

			float widthScale = (btnSize / key.width);
			float heightScale = (btnSize / key.height);
			
			int layoutX = key.x - (int)btnPadding;
			int layoutY = key.y + (int) (btnPadding * 0.25);

//			mPopupJpn.showAsDropDown(this, key.x, -key.y);
			mPopupJpn.showAtLocation(this, Gravity.NO_GRAVITY, layoutX, layoutY);
			return true;
		}
		return false;
	}

	public boolean isSimejiMode() {
		return (getKeyboard().getXmlResId() == R.xml.japan_12key_hira);
	}

	// /////////////////////////////////////////////////////////////////////////////////////////
	// PopupKeyboard 2014.05.09 yunsuk
	// /////////////////////////////////////////////////////////////////////////////////////////
	public boolean isRomajiPopupKeyboardShow() {
		if (mRomajiPopup != null && mRomajiPopup.isShowing())
			return true;
		return false;
	}

	public void dismissRomajiPopupKeyboard() {
		if (isRomajiPopupKeyboardShow()) {
			mRomajiPopup.dismiss();
		}
	}

	public void showRomajiPopupKeyboard(Key key) {
		if (mRomajiPopup == null) {
			mRomajiPopup = new PopupKeyboardEn(getContext());
		}
		mRomajiPopup.init(getContext(), key, getWidth(), getHeight(),
				getKeyboard().isShifted());

		mRomajiPopup.showAsDropDown(this, 0, 1);
		mRomajiPopup.setOnPopupKeyboardListener(new popupKeyboardListener() {

			@Override
			public void onText(String txt) {
				// TODO Auto-generated method stub
				keyboardViewListener.onCommitText(txt);
			}
		});
	}

	public boolean isRomajiPopup(Key key) {
		if (key != null && key.codes[0] >= 'a' && key.codes[0] <= 'z'
				&& key.popupCharacters != null
				&& key.popupCharacters.length() > 0) {
			return true;
		}
		return false;
	}

	@Override
	public LatinKeyboard getKeyboard() {
		return (LatinKeyboard) super.getKeyboard();
	}

	// /////////////////////////////////////////////////////////////////////////////////////////
	// Listener
	// /////////////////////////////////////////////////////////////////////////////////////////
	private onKeyboardViewListener keyboardViewListener;
	public interface onKeyboardViewListener {
		public void onBlankEvent();
		public void onSimejiInputKey(String keyText);
		public void onLongPressKey(Key key);
		public void onTouchUp(int x, int y);
		public void onCommitText(String txt);
	}
	
	public void setOnKeyboardViewListener(onKeyboardViewListener listener) {
		keyboardViewListener = listener;
	}
	
	
	@Override
	public void onDraw(Canvas canvas) {
		super.onDraw(canvas);

			drawModifierKey(canvas);
		}

	private void drawModifierKey(Canvas canvas){
		List<Key> keys = getKeyboard().getKeys();
		int keyIndex = 0;
		for (Key key : keys) {
			if (key.modifier) {
				if (key.codes[0] == LatinKeyboard.KEYCODE_SPACE
						|| key.codes[0] == JapaneseManager.KEYCODE_SPACE) {
					
					if (getKeyboard().getGlobalText().isEmpty() == false
							&& mIsAnimTextStart) {
//						Drawable bg = getResources().getDrawable(
//								R.drawable.btn_keyboard_key);
						Drawable bg = getResources().getDrawable(
								R.color.background_color);
						if (bg != null) {
							bg.setBounds(key.x, key.y, key.x + key.width, key.y + key.height);
							bg.draw(canvas);
						}
						
						drawAnimKeyText(canvas, key, keyIndex);
					} else {
						mAnimKeyPaint.setAlpha(255);
					}
					drawKeyIcon(canvas, key);
				}
			}
			keyIndex++;
		}
	}
	

	private void drawAnimKeyText(Canvas canvas, Key key, int keyIndex) {

		if (mAnimTextStartTime == 0) {
			mAnimTextStartTime = System.currentTimeMillis();
			mAnimKeyPaint.setAlpha(0);
		}

		String drawText = getKeyboard().getGlobalText();

		mAnimTextPaint.getTextBounds(drawText, 0, drawText.length(), mKeyBounds);
		float x = key.x + (key.width / 2) - (mAnimTextPaint.measureText(drawText) / 2);
		float y = key.y + (key.height / 2) + (mKeyBounds.height() / 2);
			
		canvas.drawText(drawText, x, y, mAnimTextPaint);

		long time = System.currentTimeMillis() - mAnimTextStartTime;

		if (time > 2000 && time < 3000) {
			int alpha = mAnimTextPaint.getAlpha() - 10;
			if (alpha > 0)
				mAnimTextPaint.setAlpha(alpha);

		} else if (time > 3000) {
			mAnimTextPaint.setAlpha(0);
			mIsAnimTextStart = false;
			mAnimTextStartTime = 0;
			mAnimKeyPaint.setAlpha(255);
		}
		
		invalidateKey(keyIndex);
	}

	
	private void drawKeyIcon(Canvas canvas, Key key) {
		if(key == null || key.icon == null) return;

		Bitmap bitmap = ((BitmapDrawable) key.icon).getBitmap();

		if (key.codes[0] == LatinKeyboard.KEYCODE_SPACE
				|| key.codes[0] == JapaneseManager.KEYCODE_SPACE) {
			canvas.drawBitmap(bitmap,
					key.x + ((key.width / 2) - (bitmap.getWidth() / 2)), key.y
							+ ((key.height / 2) - (bitmap.getHeight() / 2)),
					mAnimKeyPaint);
		} else {
			canvas.drawBitmap(bitmap,
					key.x + ((key.width / 2) - (bitmap.getWidth() / 2)), key.y
							+ ((key.height / 2) - (bitmap.getHeight() / 2)),
					null);
		}
	}
}
