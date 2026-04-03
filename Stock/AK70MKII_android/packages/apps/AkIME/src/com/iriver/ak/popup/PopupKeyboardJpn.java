package com.iriver.ak.popup;

import com.iriver.ak.lang.manager.JapaneseManager;
import com.iriver.ak.lang.map.LanguageMap;
import com.iriver.ak.softkeyboard.R;

import android.content.Context;
import android.graphics.Rect;
import android.inputmethodservice.Keyboard.Key;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.MeasureSpec;
import android.widget.Button;
import android.widget.PopupWindow;

public class PopupKeyboardJpn extends PopupWindow{

	private View mView = null;
	private Key mKey = null;
	private Context mContext = null;
	
	// japan simeji virtual key val
	private final int DIRECT_CENTER = 0;
	private final int DIRECT_LEFT = 1;
	private final int DIRECT_TOP = 2;
	private final int DIRECT_RIGHT = 3;
	private final int DIRECT_BOTTOM = 4;
	private int mDirectSideBtn = -1;

	private boolean isLongKeyIntersect = true;
	private boolean isShortCodes = false;

	private Rect longPressKeyRect = null;

	private Button mBtnLeft;
	private Button mBtnTop;
	private Button mBtnRight;
	private Button mBtnBottom;

	public PopupKeyboardJpn(Context context) {
		
		mContext = context;
		
		setBackgroundDrawable(null);
		
		LayoutInflater inflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
		mView = inflater.inflate(R.layout.popupkey_jp, null);
		initButton(mView);
		
		longPressKeyRect = new Rect();
		
		setContentView(mView);
	}
	
	public void init(Context context, Key key, int width, int height){
		mKey = key;
		
		int miniWidth = (int) (key.width * 0.2);
		int miniHeight = (int) (key.height * 0.2);

		longPressKeyRect.set(key.x + miniWidth, key.y + miniHeight, key.x + miniWidth, key.y + miniHeight);

		InitIsShortCodes(key.codes.length);

		ShowSideKeyPopupWindow(key,width, height);
	}
	
	public boolean isLanguageJpn(Key key){
		if(key.codes[0] >= JapaneseManager.KEYCODE_HIRA_START && key.codes[0] <= JapaneseManager.KEYCODE_HIRA_END)
			return true;
		return false;
	}
	
	public boolean SimejiShowVirtualPopupKey(Key key) {
		if (isLanguageJpn(key) || isSimejiSymbolKey(key)) {
			mKey = key;

			int miniWidth = (int) (key.width * 0.2);
			int miniHeight = (int) (key.height * 0.2);

			longPressKeyRect.set(key.x + miniWidth, key.y + miniHeight, key.x + miniWidth, key.y + miniHeight);

			InitIsShortCodes(key.codes.length);

			return true;
		}
		return false;
	}

	public boolean SimejiDismissVirtualPopupKey() {
		
		if (mKey != null && ((mKey.codes[0] >= JapaneseManager.KEYCODE_HIRA_START
				&& mKey.codes[0] < JapaneseManager.KEYCODE_HIRA_END) || isSimejiSymbolKey(mKey))) {
			if (mDirectSideBtn != -1) {
				if (mKey.codes.length < 4 && (mDirectSideBtn == DIRECT_TOP || mDirectSideBtn == DIRECT_BOTTOM)) {
					// 2-direct popup (up & down)
					// Empty
				} else if (mKey.codes.length < 4 && (mDirectSideBtn == DIRECT_LEFT || mDirectSideBtn == DIRECT_RIGHT)) {
					// 2-direct popup (left & right)
					// language & direct = sync
					if (mDirectSideBtn == DIRECT_RIGHT)
						mDirectSideBtn -= 1;

					mListener.setKey(mKey.codes[0] + mDirectSideBtn, mKey.codes);
				} else {
					// ## symbol popup
					if (isSimejiSymbolKey(mKey)) {
						if (mDirectSideBtn != DIRECT_BOTTOM) {
							String symbol = LanguageMap.getHiraganaSymbol(mDirectSideBtn - 1);
							mListener.onSimejiInputKey(symbol);
						}
					} else { // 4-direct popup
						
						mListener.setKey(mKey.codes[0] + mDirectSideBtn, mKey.codes);
					}
				}
			}

			dismissPopupKeyboard();
			isLongKeyIntersect = true;
			mKey = null;

			return true;
		}

		return false;
	}

	public boolean isSimejiSymbolKey(Key key) {
		return (key.codes[0] == JapaneseManager.KEYCODE_SYMBOL_START);
	}

	public boolean isSimejiKeyboard(Key key) {
		if ((key.codes[0] >= JapaneseManager.KEYCODE_HIRA_START && key.codes[0] <= JapaneseManager.KEYCODE_HIRA_END)
				|| key.codes[0] == JapaneseManager.KEYCODE_SYMBOL_START) {
			return true;
		}
		return false;
	}

	public void dismissPopupKeyboard() {
		if (isShowing()) {
			dismiss();
		}
	}

	public void InitIsShortCodes(int length) {
		if (length < 4)
			isShortCodes = true;
		else
			isShortCodes = false;
	}

	public void ShowSideKeyPopupWindow(Key popupKey, int width, int height) {
		updateBtnText(popupKey);
		
		mView.measure(MeasureSpec.makeMeasureSpec(width, MeasureSpec.AT_MOST),
				MeasureSpec.makeMeasureSpec(height, MeasureSpec.AT_MOST));

		setWidth(mView.getMeasuredWidth());
		setHeight(mView.getMeasuredHeight());
	}

	public void showDirectButton(int x1, int y1, int x2, int y2) {
		int direct = getDirectFromDegree(x1, y1, x2, y2);

		if (direct >= DIRECT_CENTER && direct <= DIRECT_BOTTOM) {
			mDirectSideBtn = direct;
			updateBtnSelect(direct);
		}
	}

	public int getDirectFromDegree(int x1, int y1, int x2, int y2) {
		// 0 : center , 1 : left , 2 : top , 3 : right , 4 : bottom
		if (mKey == null)
			return -1;

		if (isLongKeyIntersect == true) {
			if (mKey.x <= x2 && (mKey.x + mKey.width) >= x2 && mKey.y <= y2 && (mKey.y + mKey.height) >= y2) {
				return -1;
			} else {
				isLongKeyIntersect = false;
			}
		}

		if (longPressKeyRect.left <= x2 && longPressKeyRect.right >= x2 && longPressKeyRect.top <= y2
				&& longPressKeyRect.bottom >= y2) {
			return DIRECT_CENTER;
	}

		int degree = (int) getDegree(x1, y1, x2, y2);

		if (degree < -45 && degree > -135) {
			return DIRECT_LEFT;
		} else if ((degree < -135 && degree > -179) || degree <= 180 && degree > 135) {
			if (isShortCodes == true)
				return -1;
			return DIRECT_TOP;
		} else if (degree < 135 && degree > 45) {
			return DIRECT_RIGHT;
		} else if (degree < 45 && degree > -45) {
			if (isShortCodes == true)
				return -1;
			return DIRECT_BOTTOM;
	}

		return DIRECT_CENTER;
	}

	public double getDegree(int x1, int y1, int x2, int y2) {
		int dx = x2 - x1;
		int dy = y2 - y1;

		double rad = Math.atan2(dx, dy);
		double degree = (rad * 180) / Math.PI;

		return degree;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Add, 170418 yunsuk [Popup Update]

	private void initButton(View v) {
		mBtnLeft = (Button) v.findViewById(R.id.jp_btn_left);
		mBtnTop = (Button) v.findViewById(R.id.jp_btn_top);
		mBtnRight = (Button) v.findViewById(R.id.jp_btn_right);
		mBtnBottom = (Button) v.findViewById(R.id.jp_btn_bottom);
	}

	private void updateBtnSelect(int direct) {
		if (mBtnLeft != null) {
			mBtnLeft.setSelected(direct == DIRECT_LEFT);
		}
		if (mBtnTop != null) {
			mBtnTop.setSelected(direct == DIRECT_TOP);
		}
		if (mBtnRight != null) {
			mBtnRight.setSelected(direct == DIRECT_RIGHT);
		}
		if (mBtnBottom != null) {
			mBtnBottom.setSelected(direct == DIRECT_BOTTOM);
		}
	}

	public void updateBtnText(Key popupKey) {
		String leftText = null;
		String topText = null;
		String rightText = null;
		String bottomText = null;

		if (isSimejiSymbolKey(popupKey) == true) {
			// ,º\?!
			leftText = LanguageMap.getHiraganaSymbol(0);
			topText = LanguageMap.getHiraganaSymbol(1);
			rightText = LanguageMap.getHiraganaSymbol(2);
		} else {
			int length = popupKey.codes.length;

			if (length == 3) {
				leftText = LanguageMap.getSimeji(popupKey.codes[1] - JapaneseManager.KEYCODE_HIRA_START);
				rightText = LanguageMap.getSimeji(popupKey.codes[2] - JapaneseManager.KEYCODE_HIRA_START);
			} else if (length == 4) {
				leftText = LanguageMap.getSimeji(popupKey.codes[1] - JapaneseManager.KEYCODE_HIRA_START);
				topText = LanguageMap.getSimeji(popupKey.codes[2] - JapaneseManager.KEYCODE_HIRA_START);
				rightText = LanguageMap.getSimeji(popupKey.codes[3] - JapaneseManager.KEYCODE_HIRA_START);
			} else {
				leftText = LanguageMap.getSimeji(popupKey.codes[1] - JapaneseManager.KEYCODE_HIRA_START);
				topText = LanguageMap.getSimeji(popupKey.codes[2] - JapaneseManager.KEYCODE_HIRA_START);
				rightText = LanguageMap.getSimeji(popupKey.codes[3] - JapaneseManager.KEYCODE_HIRA_START);
				bottomText = LanguageMap.getSimeji(popupKey.codes[4] - JapaneseManager.KEYCODE_HIRA_START);
		}
	}
	
		if (mBtnLeft != null) {
			if (leftText != null) {
				mBtnLeft.setText(leftText);
				mBtnLeft.setVisibility(View.VISIBLE);
			} else {
				mBtnLeft.setVisibility(View.GONE);
		}
	}

		if (mBtnTop != null) {
			if (topText != null) {
				mBtnTop.setText(topText);
				mBtnTop.setVisibility(View.VISIBLE);
			} else {
				mBtnTop.setVisibility(View.GONE);
			}
		}

		if (mBtnRight != null) {
			if (rightText != null) {
				mBtnRight.setText(rightText);
				mBtnRight.setVisibility(View.VISIBLE);
			} else {
				mBtnRight.setVisibility(View.GONE);
			}
		}

		if (mBtnBottom != null) {
			if (bottomText != null) {
				mBtnBottom.setText(bottomText);
				mBtnBottom.setVisibility(View.VISIBLE);
			} else {
				mBtnBottom.setVisibility(View.GONE);
		}
	}
	}
	
	// //////////////////////////////////////////////////////////////////////////////////////////////
	// Listner
	// //////////////////////////////////////////////////////////////////////////////////////////////
	private onJpnPopupKeyboardListener mListener = null;
	
	public void setOnJpnPopupKeyboardListener(onJpnPopupKeyboardListener listener){
		mListener = listener;
	}

	public interface onJpnPopupKeyboardListener{
		void setKey(int code, int codes[]);

		void onSimejiInputKey(String keyText);
	}
}
