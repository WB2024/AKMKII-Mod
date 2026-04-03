package com.iriver.ak.softkeyboard;

import com.iriver.ak.softkeyboard.LatinKeyboardView.onKeyboardViewListener;
import com.iriver.ak.softkeyboard.R;

import java.util.ArrayList;
import java.util.Locale;

import com.iriver.ak.automata.jniHanAutomata;
import com.iriver.ak.changjie.WordProcessor;
import com.iriver.ak.lang.manager.JapaneseManager;
import com.iriver.ak.lang.manager.LanguageManager;
import com.iriver.ak.lang.manager.PinyinManager;
import com.iriver.ak.lang.map.LanguageMap;
import com.iriver.ak.openwnn.WnnWord;

import android.content.Intent;
import android.content.res.Configuration;
import android.graphics.drawable.Drawable;
import android.inputmethodservice.InputMethodService;
import android.inputmethodservice.Keyboard;
import android.inputmethodservice.Keyboard.Key;
import android.os.Handler;
import android.text.method.MetaKeyKeyListener;
import android.util.Log;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.ExtractedText;
import android.view.inputmethod.ExtractedTextRequest;
import android.view.inputmethod.InputConnection;

/**
 * SoftKeyboard
 * 
 * @author yunsuk
 * 
 * @since 2017.04.19
 * @see : AK400 Keyboard
 * 
 * @since 2014.07.07
 * @see : AK500N Keyboard
 * 
 * @since 2013.07.17
 * @see : AK201 Keyboard
 * 
 */
public class SoftKeyboard extends InputMethodService implements KeyboardView.OnKeyboardActionListener {
	static final String TAG = "SoftKeyboard";
	static final boolean DEBUG_LOG = true;
	static final boolean PROCESS_HARD_KEYS = true;
	static final boolean PHYSICAL_KEYBOARD_ENABLE = true; // add, 200420 ian, #Physical & Virtual Keyboard

	static final String ACTION_INPUTMETHOD_VISIBLE = "action inputmethod visible";

	final private int LIMIT_PINYIN = 7;

	// inputType
	private final int TYPE_TEXT_VARIATION_EMAIL_ADDRESS = 208;
	private final int TYPE_TEXT_VARIATION_PASSWORD = 224;

	private int mBKeyboardLanguage = -1;
	private int mKeyboardLanguage = -1;

	private LatinKeyboardView mInputView = null;

	private LatinKeyboard mCurKeyboard = null;
	private LatinKeyboard mBeforeKeyboard = null;
	private LatinKeyboard mSymbolToBeforeKeyboard = null;

	// input
	private KeyboardSwitcher mKeyboardSwitcher = null;

	// symbol
	private LatinKeyboard mSymbolsKeyboard = null;
	private LatinKeyboard mSymbolsShiftedKeyboard = null;

	// us
	private LatinKeyboard mQwertyKeyboard = null;

	// jp
	private LatinKeyboard mJpnKeyboard = null;
	private LatinKeyboard mJpnHiraKeyboard = null;
	private LatinKeyboard mJpnEngKeyboard = null;
	private LatinKeyboard mJpnNumberKeyboard = null;
	private LatinKeyboard mJpnSymbolKeyboard = null;
	private LatinKeyboard mJpnHiraFromQwertyKeyboard = null;

	// ch
	private LatinKeyboard mPinyinKeyboard = null;

	// tw
	private LatinKeyboard mChangjieKeyboard = null;

	// rus
	private LatinKeyboard mRusKeyboard = null;

	private int mLastDisplayWidth;
	private long mMetaState;
	private boolean mPredictionOn;
	private boolean mCapsLock;

	private StringBuilder mComposing = new StringBuilder();
	private String mWordSeparators = "";

	// JpnAutomata
	private JapaneseManager mJpnManager = null;

	// PinyinAutomata
	private PinyinManager mPinyinManager = null;

	// Candidate
	private CandidateView mCandidateView = null;

	private int mJpnCharBeforeIndex = -1;

	// Configuration
	// private int mConfig = Configuration.ORIENTATION_PORTRAIT;

	// input Chanjie
	private WordProcessor wordProcessor = null;
	private char[] charbuffer = new char[5];
	private int strokecount = 0;

	// Add, 180709 yunsuk [Jp Kata Input Recycle Time]
	private long mJapanKataInputTime = 0;

	private Handler mHandler = new Handler();

	private void sendVisibleBroadcast(final boolean isVisible) {

		if (mInputView != null) {
			mInputView.post(new Runnable() {

				@Override
				public void run() {
					float height = mInputView.getHeight();
					if (height == 0) {
						if (mCurKeyboard != null && mCurKeyboard == mRusKeyboard) {
							height = getResources().getDimension(R.dimen.keypad_min_height_ru);
						} else {
							height = getResources().getDimension(R.dimen.keypad_min_height);
						}
						Log.e(TAG, "sendVisibleBroadcast mInputView height null!! Fixed Height : " + height);
					}

					if (DEBUG_LOG) {
						Log.d(TAG, "sendVisibleBroadcast " + isVisible + ", height : " + height);
					}

					Intent intent = new Intent();
					intent.setAction(ACTION_INPUTMETHOD_VISIBLE);
					intent.putExtra("visible", isVisible);
					intent.putExtra("height", height);

					sendBroadcast(intent);
				}
			});
		}
	}

	@Override
	public void onCreate() {
		super.onCreate();
		if (DEBUG_LOG) {
			Log.d(TAG, "onCreate");
		}

		// debugging
		// android.os.Debug.waitForDebugger(); // debug
		mWordSeparators = getResources().getString(R.string.word_separators);

		mJpnManager = new JapaneseManager(this);
		mPinyinManager = new PinyinManager(this);
		mJpnManager.setComposing(mComposing);
		mKeyboardSwitcher = new KeyboardSwitcher();
	}

	@Override
	public void onInitializeInterface() {
		if (DEBUG_LOG) {
			Log.d(TAG, "onInitializeInterface");
		}

		if (mKeyboardSwitcher != null) {
			int displayWidth = getMaxWidth();
			if (displayWidth == mLastDisplayWidth)
				return;
			mLastDisplayWidth = displayWidth;
		}
		setLangKeyboard(true);
	}

	@Override
	public View onCreateInputView() {
		if (DEBUG_LOG) {
			Log.d(TAG, "onCreateInputView");
		}

		mInputView = (LatinKeyboardView) getLayoutInflater().inflate(R.layout.custom_input, null);

		if (mInputView != null && mKeyboardSwitcher != null) {
			mInputView.setOnKeyboardActionListener(this);
			mInputView.setKeyboard(mKeyboardSwitcher.getDefaultKeyboard());
			mKeyboardSwitcher.setDefaultKeyboardIndex();
			mInputView.setPreviewEnabled(true);
			mInputView.KeyInit();
			// mInputView.setKeyboardLanguage(mKeyboardLanguage);
			mInputView.setOnKeyboardViewListener(mOnKeyboardViewListener);
		}

		return mInputView;
	}

	@Override
	public void onFinishInput() {
		super.onFinishInput();
		if (DEBUG_LOG) {
			Log.d(TAG, "onFinishInput");
		}

		mComposing.setLength(0);

		/** Set Default Keyboard **/
		if (mCurKeyboard == null && mKeyboardSwitcher.getDefaultKeyboard() != null) {
			mCurKeyboard = mKeyboardSwitcher.getDefaultKeyboard();
			mKeyboardSwitcher.setDefaultKeyboardIndex();
		}

		initKeyboardState();

		sendVisibleBroadcast(false);
	}

	@Override
	public void onStartInput(EditorInfo attribute, boolean restarting) {
		super.onStartInput(attribute, restarting);
		if (DEBUG_LOG) {
			Log.d(TAG, "onStartInput restarting : " + restarting);
		}

		if (mComposing != null)
			mComposing.setLength(0);

		if (!restarting) {
			mMetaState = 0; // Clear shift states.
		} else {
			// Init
			initKeyboardState();
		}

		if (isInputViewKeyboard()) {
			hideCandidate();
		}

		mPredictionOn = false;

		setLangKeyboard(false);

		switch (attribute.inputType & EditorInfo.TYPE_MASK_CLASS) {
		case EditorInfo.TYPE_CLASS_NUMBER:
		case EditorInfo.TYPE_CLASS_DATETIME:
		case EditorInfo.TYPE_CLASS_PHONE:

			if (mKeyboardLanguage == LanguageManager.KEYBOARD_LANGUAGE_JP) {
				if (mKeyboardSwitcher != null && mKeyboardSwitcher.getSymbolKeyboard() != null)
					mCurKeyboard = mKeyboardSwitcher.getSymbolKeyboard();
			} else {
				if (mSymbolsKeyboard != null)
					mCurKeyboard = mSymbolsKeyboard;
			}
			break;

		case EditorInfo.TYPE_CLASS_TEXT:
			if (mKeyboardSwitcher != null && mKeyboardSwitcher.getDefaultKeyboard() != null) {
				mCurKeyboard = mKeyboardSwitcher.getDefaultKeyboard();
				mKeyboardSwitcher.setDefaultKeyboardIndex();
			}
			mPredictionOn = true;

			if (mKeyboardLanguage != LanguageManager.KEYBOARD_LANGUAGE_ERROR && mBKeyboardLanguage != mKeyboardLanguage
					&& mBeforeKeyboard != null) {
				mCurKeyboard = mBeforeKeyboard;
			}

			int variation = attribute.inputType & EditorInfo.TYPE_MASK_VARIATION;
			if (variation == EditorInfo.TYPE_TEXT_VARIATION_PASSWORD
					|| variation == EditorInfo.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD
					|| variation == EditorInfo.TYPE_TEXT_VARIATION_EMAIL_ADDRESS
					|| variation == EditorInfo.TYPE_TEXT_VARIATION_URI
					|| variation == EditorInfo.TYPE_TEXT_VARIATION_FILTER
					|| variation == TYPE_TEXT_VARIATION_EMAIL_ADDRESS || variation == TYPE_TEXT_VARIATION_PASSWORD) {

				if (mKeyboardSwitcher != null && mKeyboardSwitcher.getQwertyKeyboard() != null) {
					mCurKeyboard = mKeyboardSwitcher.getQwertyKeyboard();
				}
				mPredictionOn = false;
			}

			if ((attribute.inputType & EditorInfo.TYPE_TEXT_FLAG_AUTO_COMPLETE) != 0) {
				mPredictionOn = false;
			}
			updateShiftKeyState(attribute);
			break;

		default:
			mPredictionOn = true;
			if (mCurKeyboard == null && mKeyboardSwitcher != null && mKeyboardSwitcher.getDefaultKeyboard() != null) {
				mCurKeyboard = mKeyboardSwitcher.getDefaultKeyboard();
				mKeyboardSwitcher.setDefaultKeyboardIndex();
			}
			updateShiftKeyState(attribute);
			break;
		}

		if (attribute.privateImeOptions != null) {
			attribute.privateImeOptions.trim();

            if (attribute.privateImeOptions.contains("defaultInputmode=english") == true
					&& mQwertyKeyboard != null) {
				mCurKeyboard = mQwertyKeyboard;
			} else if (attribute.privateImeOptions.contains("defaultInputmode=numeric") == true
					&& mSymbolsKeyboard != null) {
				mCurKeyboard = mSymbolsKeyboard;
			} else if (attribute.privateImeOptions.contains("imeOptions=Search")) {
				mCurKeyboard.setImeOptions(getResources(), LatinKeyboard.KEYCODE_ENTER_SEARCH_KEY);
				return;
			}
		}

		if (mCurKeyboard != null)
			mCurKeyboard.setImeOptions(getResources(), attribute.imeOptions);
	}

	@Override
	public void onStartInputView(EditorInfo attribute, boolean restarting) {
		super.onStartInputView(attribute, restarting);
		// Apply the selected keyboard to the input view.
		if (DEBUG_LOG) {
			Log.d(TAG, "onStartInputView restarting : " + restarting);
		}

		if (mInputView != null) {
			if (mCurKeyboard == mJpnKeyboard
					&& mJpnManager.getKeyboardType() == JapaneseManager.KEYBOARD_MODE_JP_HIRA) {

				if (mCandidateView != null)
					mCandidateView.setMode(CandidateView.JapanMode);

			} else if (mCurKeyboard == mSymbolsKeyboard && mQwertyKeyboard != null) {
				mBeforeKeyboard = mQwertyKeyboard;
				SymbolMode_GlobalKeyTextChange();
			} else {
				mBeforeKeyboard = mCurKeyboard;
			}

			// candi hide
			if (mCandidateView != null) {
				hideCandidate();

				// reset
				mJpnManager.reset();
				mPinyinManager.reset();
			}

			if (mInputView.isRomajiPopupKeyboardShow()) {
				mInputView.dismissRomajiPopupKeyboard();
			}

			// qwety shift lock init..
			handleQwertyShiftInit();

			// Changjie shift init..
			setChangjieShiftState(false);

			mInputView.setKeyboard(mCurKeyboard);
			mInputView.closing();

			mInputView.KeyInit();
			mInputView.setShifted(false);

			sendVisibleBroadcast(true);
		}
	}

	@Override
	public View onCreateCandidatesView() {
		if (DEBUG_LOG)
			Log.d(TAG, "onCreateCandidatesView");

		View innerView = getLayoutInflater().inflate(R.layout.candidateview, null);
		mCandidateView = new CandidateView(this, innerView);
		mCandidateView.setOnBtnNotificationListener(mCandidateListener);

		return mCandidateView;
	}

	/**
	 * Setting Country language keyboard yunsuk 2013.12.26 2. 2014.01.07 :
	 * LanguageManager 3. 2014.10.21 : ADD 500N
	 */
	private void setLangKeyboard_JP() {
		mJpnManager.setKeyboardType(JapaneseManager.KEYBOARD_MODE_JP_HIRA);

		mJpnHiraKeyboard = new LatinKeyboard(this, R.xml.japan_12key_hira);
		mJpnHiraFromQwertyKeyboard = new LatinKeyboard(this, R.xml.japan_qwerty);
		mJpnEngKeyboard = new LatinKeyboard(this, R.xml.japan_12key_s_qwerty);
		mJpnNumberKeyboard = new LatinKeyboard(this, R.xml.japan_12key_number);
		mJpnSymbolKeyboard = new LatinKeyboard(this, R.xml.japan_12key_symbol);

		mJpnKeyboard = mJpnHiraKeyboard;

		mKeyboardSwitcher.addKeyboard(mJpnHiraKeyboard, KeyboardSwitcher.KEYBOARD_TYPE_JPN);
		mKeyboardSwitcher.addKeyboard(mJpnHiraFromQwertyKeyboard, KeyboardSwitcher.KEYBOARD_TYPE_JP_HIRA_QERTY);
	}

	/**
	 * @author yunsuk
	 * @see The keyboard is the default keyboard for the first time you will
	 *      save.
	 * @see The keyboard is added by the set value.
	 * @Since 2015.01.05
	 */
	private void setLangKeyboard(boolean isInitialize) {
		if (mKeyboardSwitcher != null) {

			String defaultLanguage = Locale.getDefault().toString();
			String language = defaultLanguage.substring(0, 2);
			String country = defaultLanguage.substring(3, 5);

			if (DEBUG_LOG) {
				Log.i(TAG, "## KEYBOARD SET :: LANGUAGE : " + language + ", COUNTRY : " + country);
			}

			mBKeyboardLanguage = mKeyboardLanguage;

			mKeyboardLanguage = LanguageManager.getLocalLanguage(language, country);

			mKeyboardSwitcher.clear();

			switch (mKeyboardLanguage) {
			case LanguageManager.KEYBOARD_LANGUAGE_JP:

				setLangKeyboard_JP();
				break;
			case LanguageManager.KEYBOARD_LANGUAGE_CN:
				if (mPinyinKeyboard == null || isInitialize)
					mPinyinKeyboard = new LatinKeyboard(this, R.xml.pinyin_qwerty);
				mKeyboardSwitcher.addKeyboard(mPinyinKeyboard, KeyboardSwitcher.KEYBOARD_TYPE_CHS);
				break;
			case LanguageManager.KEYBOARD_LANGUAGE_TW:
				if (mChangjieKeyboard == null || isInitialize)
					mChangjieKeyboard = new LatinKeyboard(this, R.xml.changjie);
				mKeyboardSwitcher.addKeyboard(mChangjieKeyboard, KeyboardSwitcher.KEYBOARD_TYPE_CHT);
				break;
			case LanguageManager.KEYBOARD_LANGUAGE_RU:
				if (mRusKeyboard == null || isInitialize)
					mRusKeyboard = new LatinKeyboard(this, R.xml.ru_rus);
				mKeyboardSwitcher.addKeyboard(mRusKeyboard, KeyboardSwitcher.KEYBOARD_TYPE_RUS);
				break;
			default:
				mKeyboardLanguage = LanguageManager.KEYBOARD_LANGUAGE_EN;
				if (mQwertyKeyboard == null || isInitialize)
					mQwertyKeyboard = new LatinKeyboard(this,
							R.xml.qwerty/* R.xml.us_qwerty */);
				break;
			}

			// Qwerty, Symbol, SymbolShift
			if (mKeyboardLanguage != LanguageManager.KEYBOARD_LANGUAGE_EN) {
				mQwertyKeyboard = new LatinKeyboard(this, R.xml.qwerty);
			}
			mKeyboardSwitcher.addKeyboard(mQwertyKeyboard, KeyboardSwitcher.KEYBOARD_TYPE_ENG);

			if (mSymbolsKeyboard == null || isInitialize) {
				mSymbolsKeyboard = new LatinKeyboard(this, R.xml.symbols);
				mSymbolsShiftedKeyboard = new LatinKeyboard(this, R.xml.symbols_shift);
			}

			// yunsuk 2014.02.19 ShiftLock
			if (mShiftUpIcon == null) {
				mShiftUpIcon = getResources().getDrawable(R.drawable.key_caps_on);
			}

			if (mShiftLockIcon == null) {
				mShiftLockIcon = getResources().getDrawable(R.drawable.key_caps_pressd);
			}

			if (mShiftNormalIcon == null) {
				mShiftNormalIcon = getResources().getDrawable(R.drawable.key_caps_off);
			}

			mCurKeyboard = mKeyboardSwitcher.getDefaultKeyboard();
			mKeyboardSwitcher.setDefaultKeyboardIndex();
		}

		addKeyboardBySettings(isInitialize);

	}

	private void addKeyboardBySettings(boolean isInitialize) {
		int keyboardNum = android.provider.Settings.System.getInt(getContentResolver(),
				android.provider.Settings.System.KEYBOARD_LANGUAGE, android.provider.Settings.System.KEYBOARD_LANG_ENG);

		Log.i(TAG, "############ addKeyboardBySettings : " + keyboardNum);

		if ((keyboardNum
				& android.provider.Settings.System.KEYBOARD_LANG_CHS) == android.provider.Settings.System.KEYBOARD_LANG_CHS
				&& mKeyboardLanguage != LanguageManager.KEYBOARD_LANGUAGE_CN) {
			if (mPinyinKeyboard == null || isInitialize)
				mPinyinKeyboard = new LatinKeyboard(this, R.xml.pinyin_qwerty);
			mKeyboardSwitcher.addKeyboard(mPinyinKeyboard, KeyboardSwitcher.KEYBOARD_TYPE_CHS);

			if (DEBUG_LOG)
				Log.i(TAG, "## Settings Add Keyboard Chs ##");
		}
		if ((keyboardNum
				& android.provider.Settings.System.KEYBOARD_LANG_CHT) == android.provider.Settings.System.KEYBOARD_LANG_CHT
				&& mKeyboardLanguage != LanguageManager.KEYBOARD_LANGUAGE_TW) {
			if (mChangjieKeyboard == null || isInitialize)
				mChangjieKeyboard = new LatinKeyboard(this, R.xml.changjie);
			mKeyboardSwitcher.addKeyboard(mChangjieKeyboard, KeyboardSwitcher.KEYBOARD_TYPE_CHT);

			if (DEBUG_LOG)
				Log.i(TAG, "## Settings Add Keyboard Cht ##");
		}
		if ((keyboardNum
				& android.provider.Settings.System.KEYBOARD_LANG_JPN) == android.provider.Settings.System.KEYBOARD_LANG_JPN
				&& mKeyboardLanguage != LanguageManager.KEYBOARD_LANGUAGE_JP) {

			setLangKeyboard_JP();

			if (DEBUG_LOG)
				Log.i(TAG, "## Settings Add Keyboard Jpn ##");
		}
		if ((keyboardNum
				& android.provider.Settings.System.KEYBOARD_LANG_RUS) == android.provider.Settings.System.KEYBOARD_LANG_RUS
				&& mKeyboardLanguage != LanguageManager.KEYBOARD_LANGUAGE_RU) {
			if (mRusKeyboard == null || isInitialize)
				mRusKeyboard = new LatinKeyboard(this, R.xml.ru_rus);
			mKeyboardSwitcher.addKeyboard(mRusKeyboard, KeyboardSwitcher.KEYBOARD_TYPE_RUS);

			if (DEBUG_LOG)
				Log.i(TAG, "## Settings Add Keyboard Rus ##");
		}
	}

	private void initKeyboardState() {
		if (mInputView != null) {
			LatinKeyboard curKeyboard = mInputView.getKeyboard();

            if (isInputJapanese(curKeyboard) && mJpnManager != null) {
				if (getCurrentInputConnection() != null)
					getCurrentInputConnection().commitText(mJpnManager.getEditString(), 1);
				mJpnManager.reset();
				mCandidateView.hideView();
			} else if (isInputPinyin(curKeyboard)) {
				if (getCurrentInputConnection() != null)
					getCurrentInputConnection().finishComposingText();
				mPinyinManager.reset();
				mCandidateView.hideView();
			} else if (isInputChangjie(curKeyboard)) {
				if (getCurrentInputConnection() != null)
					getCurrentInputConnection().finishComposingText();
				mCandidateView.hideView();
				resetChangjieStroke();
			}

			handleQwertyShiftInit();
			setChangjieShiftState(false);

			if (mInputView.isRomajiPopupKeyboardShow())
				mInputView.dismissRomajiPopupKeyboard();

			hideCandidate();

			mInputView.closing();
		}
	}

	private void handleClose() {

		onFinishInput();

		sendDownUpKeyEvents(Keyboard.KEYCODE_CANCEL);

		requestHideSelf(0);
	}

	public void onKey(int primaryCode, int[] keyCodes) {
		if (isInputViewKeyboard() || getCurrentInputConnection() != null) {
			Log.d(TAG, "### onKey ###   primaryCode : " + primaryCode);
			
			switch (primaryCode) {
			
			// Change, 180227 yunsuk [Original Delete Key -> Custom Key]
//			case Keyboard.KEYCODE_DELETE:
			case LatinKeyboard.KEYCODE_BACKSPACE:
				handleBackspace(primaryCode);
				break;
			case Keyboard.KEYCODE_SHIFT:
				handleShift();
				break;
			case Keyboard.KEYCODE_MODE_CHANGE:
				handleSymbolModeChange();
				break;
			case LatinKeyboard.KEYCODE_GLOBAL:
				handleGlobalChange();
				break;
			case LatinKeyboard.KEYCODE_SPACE:
				handleSpace(primaryCode);
				break;
			case LatinKeyboard.KEYCODE_ENTER:
				handleEnter(primaryCode);
				break;
			case LatinKeyboard.KEYCODE_CLOSE:
				keyDownUp(LatinKeyboard.KEYCODE_CLOSE);
				handleClose();
				break;
			case LatinKeyboard.KEYCODE_EMPTY_BLANK:
				break;
			case JapaneseManager.KEYCODE_JPN_GLOBAL:
				handleJapanGlobal(primaryCode, keyCodes);
				break;
			default:
				handleLanguage(primaryCode, keyCodes);
				break;
			}
		}
	}

	// //////////////////////////////////////////////////////////////////////////////////
	/*
	 * handle Key
	 */
	// //////////////////////////////////////////////////////////////////////////////////

	// /////////////////////////// [[ Backspace ]]
	// //////////////////////////////////
	private void handleBackspace(int primaryCode) {
		Log.d(TAG, "### handleBackspace ###");
		final int length = mComposing.length();

        if (isInputJapanese(mInputView.getKeyboard())) {
			handleBackspace_Japan();
		} else if (isInputPinyin(mInputView.getKeyboard())) {
			handleBackspace_Pinyin();
		} else if (isInputChangjie(mInputView.getKeyboard())) {
			handleBackspace_Changjie();
		} else {
			if (length > 1) {
				mComposing.delete(length - 1, length);
				getCurrentInputConnection().setComposingText(mComposing, 1);
			} else if (length > 0) {
				mComposing.setLength(0);
				getCurrentInputConnection().commitText("", 0);
			} else {
				keyDownUp(KeyEvent.KEYCODE_DEL);
			}
		}
	}

	private void handleBackspace_Japan() {
//		Log.d(TAG, "### handleBackspace_Japan ###");

		InputConnection ic = getCurrentInputConnection();

		if (mInputView.getKeyboard() == mJpnHiraKeyboard) {
			if (mJpnManager.getEisukana()) {

				mJpnManager.setIsEisukana(false);

				ic.setComposingText(mJpnManager.getComposingText(), 1);

				if (mJpnManager.getEditString().length() > 0) {
					mJpnManager.SearchWnnEngineJAJP();

					mCandidateView.backSpace(getLayoutInflater(), mJpnManager.getJpnHanjaString());
				}

			} else {
				if (mJpnManager.getEditString().length() > 0) {

					mJpnManager.OpenWnnEngineBackSpace();

					ic.setComposingText(mJpnManager.getEditString(), 1);

					mCandidateView.backSpace(getLayoutInflater(), mJpnManager.getJpnHanjaString());
				} else {
					keyDownUp(KeyEvent.KEYCODE_DEL);
				}
			}
		} else if (mInputView.getKeyboard() == mJpnHiraFromQwertyKeyboard) {

			if (mJpnManager.getEditString().length() > 0) {

				mJpnManager.OpenWnnEngineBackSpace();

				ic.setComposingText(mJpnManager.getEditString(), 1);

				mCandidateView.backSpace(getLayoutInflater(), mJpnManager.getJpnHanjaString());
			} else {
				keyDownUp(KeyEvent.KEYCODE_DEL);
			}
		} else {
			final int length = mComposing.length();

			if (length > 1) {
				mComposing.delete(length - 1, length);
				getCurrentInputConnection().setComposingText(mComposing, 1);
			} else if (length > 0) {
				mComposing.setLength(0);
				getCurrentInputConnection().commitText("", 0);
			} else {
				keyDownUp(KeyEvent.KEYCODE_DEL);
			}
		}
	}

	private void handleBackspace_Pinyin() {
//		Log.d(TAG, "### handleBackspace_Pinyin ###");
		
		InputConnection ic = getCurrentInputConnection();

		// Composing Back
		final int length = mComposing.length();
		if (length > 1) {
			mComposing.delete(length - 1, length);
			ic.setComposingText(mComposing, 1);
		} else if (length > 0) {
			mComposing.setLength(0);
			ic.commitText("", 0);
		} else {
			keyDownUp(KeyEvent.KEYCODE_DEL);
		}

		// PinyinEngine Back
		/*
		 * String[] data = null; if (mPinyinManager.backspace(mComposing)) {
		 * 
		 * if (mKeyboardLanguage == LanguageManager.KEYBOARD_LANGUAGE_CN) data =
		 * mPinyinManager.getGanData(); else if (mKeyboardLanguage ==
		 * LanguageManager.KEYBOARD_LANGUAGE_TW) data =
		 * mPinyinManager.getBunData();
		 * 
		 * Log.i(TAG, "## handleBackspace_Pinyin ##"); Log.i(TAG,
		 * mComposing.toString()); if(data != null) { String temp = ""; for(int
		 * i=0;i<data.length;i++){ temp += data[i] + ", "; } Log.i(TAG, temp); }
		 * 
		 * showCandidate(data); }
		 * 
		 * // Candidate Back mCandidateView.backSpace(getLayoutInflater(),
		 * data);
		 */

		// Modified, 2015.11.06 yunsuk
		boolean ismake = mPinyinManager.makePinyin(mComposing);

		if (ismake) {
			showCandidate(mPinyinManager.getGanData());
		}
	}

	private void handleBackspace_Changjie() {
//		Log.d(TAG, "### handleBackspace_Changjie ###");

		if (strokecount > 1 && wordProcessor != null) {
			strokecount -= 1;

			if (mComposing.length() > 1)
				mComposing.delete(strokecount, strokecount + 1);

			getCurrentInputConnection().setComposingText(mComposing, 1);

			ArrayList<String> words = wordProcessor
					.getChineseWordDictArrayList(new String(this.charbuffer, 0, this.strokecount), false);

			if (words.size() > 0) {
				String[] data = new String[words.size()];
				for (int i = 0; i < words.size(); i++) {
					data[i] = words.get(i);
				}
				showCandidate(data);
			}
		} else if (this.strokecount > 0) {
			mComposing.setLength(0);
			getCurrentInputConnection().commitText("", 0);
			this.resetChangjieStroke();
			hideCandidate();
		} else {
			keyDownUp(KeyEvent.KEYCODE_DEL);
			this.resetChangjieStroke();
			hideCandidate();
		}
	}

	private void resetChangjieStroke() {
		this.charbuffer = new char[5];
		this.strokecount = 0;
	}

	//////////////////////////////////////////////////////////////////////
	// [[ Shift ]]

	// Yunsuk 2014.02.19 [ Shift Lock ]
	private boolean isShiftLockEnablaed = false;
	private Drawable mShiftLockIcon = null;
	private Drawable mShiftUpIcon = null;
	private Drawable mShiftNormalIcon = null;

	private void handleQwertyShift() {

		// mInputView.setShifted(mCapsLock || !mInputView.isShifted());
		if (isShiftLockEnablaed) {
			isShiftLockEnablaed = false;
			((LatinKeyboard) mInputView.getKeyboard()).setShiftIcon(false, mShiftNormalIcon);
			mInputView.setShifted(false);
		} else {
			if (!mInputView.isShifted()) {
				mInputView.setShifted(true);
				((LatinKeyboard) mInputView.getKeyboard()).setShiftIcon(true, mShiftUpIcon);
			} else {
				isShiftLockEnablaed = true;
				((LatinKeyboard) mInputView.getKeyboard()).setShiftIcon(true, mShiftLockIcon);
			}
		}
	}

	private void setChangjieShiftState(boolean isState) {
		// Add, 170524 yunsuk [Changjie Shift Init]
		if (mChangjieKeyboard != null) {
			mInputView.setShifted(isState);
			mChangjieKeyboard.setShifted(isState);

			int iconId = isState ? R.drawable.key_caps_ch_on : R.drawable.key_caps_ch_off;
			Drawable icon = getResources().getDrawable(iconId);
			mChangjieKeyboard.setShiftIcon(isState, icon);
		}
	}

	private void handleQwertyShiftInit() {
		isShiftLockEnablaed = false;

		if (mQwertyKeyboard != null && mShiftNormalIcon != null)
			mQwertyKeyboard.setShiftIcon(false, mShiftNormalIcon);
		if (mJpnEngKeyboard != null && mShiftNormalIcon != null)
			mJpnEngKeyboard.setShiftIcon(false, mShiftNormalIcon);
		if (mRusKeyboard != null && mShiftNormalIcon != null)
			mRusKeyboard.setShiftIcon(false, mShiftNormalIcon);

		mInputView.setShifted(false);
	}

	private void handleShift() {
		Log.d(TAG, "### handleShift ###");

		Keyboard currentKeyboard = mInputView.getKeyboard();

		int options = ((LatinKeyboard) currentKeyboard).getImeOptions();

		if (isInputQwerty(currentKeyboard)) {
			handleQwertyShift();
		} else if (isInputSymbol(currentKeyboard)) {
			mSymbolsKeyboard.setShifted(true);
			mSymbolsShiftedKeyboard.setShifted(true);
			mInputView.setKeyboard(mSymbolsShiftedKeyboard);
		} else if (isInputSymbolShift(currentKeyboard)) {
			mSymbolsShiftedKeyboard.setShifted(false);
			mSymbolsKeyboard.setShifted(false);
			mInputView.setKeyboard(mSymbolsKeyboard);
		} else if (isInputChangjie(currentKeyboard)) {
			mChangjieKeyboard.setShifted(!mChangjieKeyboard.isShifted());

			/////////////////////////////////////////////////
			setChangjieShiftState(mChangjieKeyboard.isShifted());
			/////////////////////////////////////////////////

			mComposing.setLength(0);
			getCurrentInputConnection().commitText(mComposing, 1);
			hideCandidate();
			resetChangjieStroke();

		} else if (isInputJapanese(currentKeyboard) || isInputRus(currentKeyboard)) {
			handleQwertyShift();
		}

		((LatinKeyboard) mInputView.getKeyboard()).setImeOptions(getResources(), options);
	}

	///////////////////////////////////////////////////////////////////////
	// [[ Symbol ]]
	public void handleSymbolModeChange() {
		if (mSymbolsKeyboard != null) {
			LatinKeyboard current = mInputView.getKeyboard();
			int options = current.getImeOptions();
			String globalText = current.getGlobalText();

			// Global KeyText Change ( 가 / A / ....)
			SymbolMode_GlobalKeyTextChange();

			// keyboard change
			if (isInputSymbol(current) || isInputSymbolShift(current)) {
				// current = mBeforeKeyboard;
				current = mSymbolToBeforeKeyboard;
			} else {
				if (isInputJapanese(current)) {
					if (getCurrentInputConnection() != null) {
						getCurrentInputConnection().commitText(mJpnManager.getEditString(), 1);
					}
					mJpnManager.reset();
					mCandidateView.hideView();
				} else if (isInputPinyin(current)) {
					if (getCurrentInputConnection() != null) {
						getCurrentInputConnection().finishComposingText();
					}
					mPinyinManager.reset();
					mCandidateView.hideView();
				} else if (isInputQwerty(current)) {
					handleQwertyShiftInit();
				} else if (isInputChangjie(current)) {
					// Add, 170525 yunsuk [Changjie State Reset]
					if (getCurrentInputConnection() != null)
						getCurrentInputConnection().finishComposingText();
					mCandidateView.hideView();
					resetChangjieStroke();

					setChangjieShiftState(false);
				}

				// mBeforeKeyboard = (LatinKeyboard) current;
				mSymbolToBeforeKeyboard = (LatinKeyboard) current;
				mSymbolsKeyboard.setShifted(false);
				current = mSymbolsKeyboard;
			}

			// set imeOptions
			current.setImeOptions(getResources(), options);
			current.setGlobalText(globalText);

			// shift init
			mInputView.setKeyboard(current);
		}
	}

	///////////////////////////////////////////////////////////////////////
	// [[ Global ]]
	public void handleGlobalChange() {
		if (mCurKeyboard == null)
			return;

		Log.d(TAG, "### handleGlobalChange ###");
		
		InputConnection connection = getCurrentInputConnection();

		/** before set */
		Keyboard current = mInputView.getKeyboard();
		int imeOptions = mCurKeyboard.getImeOptions();

		if (isInputQwerty(current)) {
			handleQwertyShiftInit();
		} else if (isInputJapanese(current) && mJpnManager != null) {
			if (connection != null)
				connection.commitText(mJpnManager.getEditString(), 1);
			mJpnManager.reset();
			mCandidateView.hideView();
		} else if (isInputPinyin(current)) {
			if (connection != null)
				connection.finishComposingText();
			mPinyinManager.reset();
			mCandidateView.hideView();
		} else if (isInputChangjie(current)) {
			if (connection != null)
				connection.finishComposingText();
			mCandidateView.hideView();
			resetChangjieStroke();

			setChangjieShiftState(false);
		}

		/** after set */
		if (!isInputSymbol(current)) {

			current = mKeyboardSwitcher.next(((LatinKeyboard) current).getXmlResId());

			if (isInputJapanese(current)) {
				mJpnKeyboard = (LatinKeyboard) current;
				mCandidateView.setMode(CandidateView.JapanMode);

				// Add, 150504 yunsuk
				if (current == mJpnHiraFromQwertyKeyboard) {
					mJpnManager.setKeyboardType(JapaneseManager.KEYBOARD_QWERTY);

					mCurKeyboard = mJpnKeyboard = mJpnHiraFromQwertyKeyboard;
				}
			} else if (isInputPinyin(current)) {
				mCandidateView.setMode(CandidateView.PinyinMode);
			} else if (isInputChangjie(current)) {
				mCandidateView.setMode(CandidateView.ChangjieMode);
			}

			mBeforeKeyboard = (LatinKeyboard) current;
		} else {
			if (mSymbolToBeforeKeyboard == null) {
				mSymbolToBeforeKeyboard = mKeyboardSwitcher.getDefaultKeyboard();
			}
			current = mSymbolToBeforeKeyboard;
		}

		// Reset Composing
		mComposing.setLength(0);

		mInputView.setKeyboard(current);
		mInputView.setShifted(false);
		mCurKeyboard = (LatinKeyboard) current;
		mCurKeyboard.setImeOptions(getResources(), imeOptions);

		mInputView.KeyInit(); // Key Init

		// Add, 170511 yunsuk
		sendVisibleBroadcast(true);
	}

	public void handleLanguage(int primaryCode, int[] keyCodes) {

		if (isInputViewKeyboard() && isInputViewShown()) {

			Keyboard current = mInputView.getKeyboard();

            if (isInputJapanese(current)) {
				handleJapan(primaryCode, keyCodes);
			} else if (isInputPinyin(current)) {
				handlePinyinCN(primaryCode, keyCodes);
			} else if (isInputChangjie(current)) {
				handleChangjie(primaryCode, keyCodes);
				// }else if(isInputRus(current)) {
			} else {
				handleCharacter(primaryCode, keyCodes);
			}
		} else {
			// Log.i(getClass().getSimpleName(), "not input");
		}
	}

	private void handleRus(int primaryCode, int[] keyCodes) {
		if (primaryCode >= LatinKeyboard.KEYCODE_SYMBOL_01 && primaryCode <= LatinKeyboard.KEYCODE_SYMBOL_15) {

		}
	}

	private void handleJapanGlobal(int primaryCode, int[] keyCodes) {

		/** before set */
		Keyboard current = mInputView.getKeyboard();
		int imeOptions = mCurKeyboard.getImeOptions();

		if (isInputJapanese(current) && mJpnManager != null) {
			getCurrentInputConnection().commitText(mJpnManager.getEditString(), 1);
			mJpnManager.reset();
			mCandidateView.hideView();
		}

		if (mInputView.getKeyboard() == mJpnHiraKeyboard) {
			current = mJpnEngKeyboard;
		} else if (mInputView.getKeyboard() == mJpnEngKeyboard) {
			current = mJpnNumberKeyboard;
		} else if (mInputView.getKeyboard() == mJpnNumberKeyboard) {
			current = mJpnSymbolKeyboard;
		} else if (mInputView.getKeyboard() == mJpnHiraFromQwertyKeyboard) {
			// Add, 150504 yunsuk
			current = mKeyboardSwitcher.prev();
		} else {
			current = mJpnHiraKeyboard;
		}

		mJpnKeyboard = (LatinKeyboard) current;
		mBeforeKeyboard = (LatinKeyboard) current;
		mCandidateView.setMode(CandidateView.JapanMode);

		mInputView.setKeyboard(current);
		mInputView.setShifted(false);
		mCurKeyboard = (LatinKeyboard) current;
		mCurKeyboard.setImeOptions(getResources(), imeOptions);

		mInputView.KeyInit(); // Key Init
	}

	private void handleJapan(int primaryCode, int[] keyCodes) {
		// Add, 180305 yunsuk
		if(primaryCode == Keyboard.KEYCODE_DELETE) {
			Log.e(TAG, "### Return handleJapan primaryCode : " + primaryCode);
			return;
		}
		
		
	
		// Add, 171120 yunsuk [EditText -> Clear Text]
		InputConnection ic = getCurrentInputConnection();
		if (ic.getTextBeforeCursor(10000, 0) != null && ic.getTextBeforeCursor(10000, 0).toString().length() == 0) {
			mComposing.setLength(0);
		}

		if (mInputView.getKeyboard() == mJpnHiraKeyboard) {
			handleJapanKata(primaryCode, keyCodes);
		} else if (mInputView.getKeyboard() == mJpnHiraFromQwertyKeyboard) {
			handleJapanQwerty(primaryCode, keyCodes);
		} else {
			handleJapanCharactor(primaryCode, keyCodes);
		}
	}

	// Modified, 170711 yunsuk [Change to Japanese character input Composing]
	private void handleJapanCharactor(int primaryCode, int[] keyCodes) {
		Log.d(TAG, "### handleJapanCharactor primaryCode : " + primaryCode);
		
		int length = mComposing.length();

		// reset value
		if (primaryCode >= LatinKeyboard.KEYCODE_SYMBOL_01
				&& primaryCode <= LatinKeyboard.KEYCODE_SYMBOL_15) {

		} else if (primaryCode == JapaneseManager.KEYCODE_TURN) {
			
		} else {
			mJpnCharBeforeIndex = -1;
		}

		InputConnection ic = getCurrentInputConnection();
		
		ic.beginBatchEdit(); // Add, 180227 yunsuk
		
		////////////////////////////////////////////////////////////////////////
		//ExtractedText et = ic.getExtractedText(new ExtractedTextRequest(), 0);
		//int selStart = et.selectionStart;
		//int selEnd = et.selectionEnd;
		//Log.d(TAG, "handleJapanCharactor selStart : " + selStart + ", selEnd : " + selEnd);
		/////////////////////////////////////////////////////////////////////
			
		
		// special symbol
		if (primaryCode >= LatinKeyboard.KEYCODE_SYMBOL_01 && primaryCode <= LatinKeyboard.KEYCODE_SYMBOL_15) {

			//////////////////////////////////////////////////////////////////////
			// Add, 171117 yunsuk [// 1.etc + SpecialSymbol = commit]
			length = mComposing.length();
			if(length > 0) {
			String beforeWord = mComposing.substring(length - 1);
			boolean isBeforeWordSymbol = LanguageMap.isSpecialSymbol(beforeWord);
				if (isBeforeWordSymbol == false) {
				Log.e(TAG, "### handleJapanCharactor Current[Symbol], Before[etc] ###");
					ic.commitText(mComposing, 1);
					mComposing.setLength(0);
				}
			}
		
			///////////////////////////////////////////////////////////////////////
			String beforeWord = "";

			if (mJpnCharBeforeIndex != (primaryCode - LatinKeyboard.KEYCODE_SYMBOL_01)) {
				mJpnCharBeforeIndex = (primaryCode - LatinKeyboard.KEYCODE_SYMBOL_01);
				
				// Add, 171116 yunsuk [e.SpecialSymbol Different]
				Log.e(TAG, "### Current[Symbol], Current[Other Symbol] Commit Text : " 
								+ mComposing.toString() + " ###");
				
				ic.commitText(mComposing, 1);
				mComposing.setLength(0);
			} else {
				if (length > 0) {
					beforeWord = mComposing.substring(length - 1);
					mComposing.delete(length - 1, length);
				}
			}

			String symbol = LanguageMap.getSpecialSymbol(beforeWord, primaryCode - LatinKeyboard.KEYCODE_SYMBOL_01);
			if (symbol != null) {
				mComposing.append(symbol);
			}

			ic.setComposingText(mComposing, 1);

		} else if (primaryCode >= JapaneseManager.KEYCODE_NUMBER_0 && primaryCode <= JapaneseManager.KEYCODE_NUMBER_9) {
			mComposing.append((char) primaryCode);
			ic.commitText(mComposing, 1);
			mComposing.setLength(0);
		} else if (primaryCode == JapaneseManager.KEYCODE_TURN) {

			if (length > 0) {
				String beforeWord = mComposing.substring(length - 1);

				if (beforeWord != null && Character.isLetter(beforeWord.charAt(0))) {
					// Add, 170711 yunsuk [Add all lower case letters and upper case letters]
					boolean isUpperCase = Character.isUpperCase(beforeWord.charAt(0));
					beforeWord = beforeWord.toLowerCase();

					String afterWord = LanguageMap.getEngCycleTableWord(beforeWord);

					if (afterWord != null && afterWord.length() > 0) {
						
						if(isUpperCase) {
							afterWord = afterWord.toUpperCase();
						}

						mComposing.delete(length - 1, length);
						mComposing.append(afterWord);

						ic.setComposingText(mComposing, 1);
					}
				} else {
					String symbol = LanguageMap.getSpecialSymbol(beforeWord, mJpnCharBeforeIndex);
					if (symbol != null && symbol.length() > 0) {
						mComposing.delete(length - 1, length);
						mComposing.append(symbol);
						ic.setComposingText(mComposing, 1);
					}
				}
			}

		} else if (primaryCode == JapaneseManager.KEYCODE_REPLACE_L_U) { // Lower
			
																			// <->Uppe
			if (length > 0) {
				String beforeWord = mComposing.substring(length - 1);

				if (Character.isLetter(beforeWord.charAt(0))) {
					if (!beforeWord.toUpperCase().equals(beforeWord)) {
						beforeWord = beforeWord.toUpperCase();
					} else {
						beforeWord = beforeWord.toLowerCase();
					}
				}
				mComposing.delete(length - 1, length);
				mComposing.append(beforeWord);
				ic.setComposingText(mComposing, 1);
			}
		} else if (primaryCode == JapaneseManager.KEYCODE_SPACE) {
			mComposing.append(" ");
			ic.commitText(mComposing, 1);
			mComposing.setLength(0);

		} else {
			if (isAlphabet(primaryCode)) {
				
				////////////////////////////////////////////////////////////////////////////
				// Add, 171117 yunsuk [// 2.SpecialSymbol + etc = commit]
				length = mComposing.length();
				if (length > 0) {
				String beforeWord = mComposing.substring(length - 1);
					if (LanguageMap
							.isSpecialSymbol(beforeWord) == true) {
						Log.e(TAG,
								"### Current[Alphabet], Before[Symbol] CommitText : "
										+ mComposing.toString() + " ###");
				
						ic.finishComposingText();
					mComposing.setLength(0);
					} else {
						// Add, 180305 yunsuk
						int charCode = (int)beforeWord.charAt(0);
						for(int p : keyCodes) {
							if(charCode == p) {
								mComposing.delete(length - 1, length);
								break;
							}
						}
				}
				}
				////////////////////////////////////////////////////////////////////////////
				
				mComposing.append((char) primaryCode);
			} else {
				ic.commitText(mComposing, 1);
				mComposing.setLength(0);
				mComposing.append(String.valueOf((char) primaryCode));
			}
			
			ic.setComposingText(mComposing, 1);
		}
		
		ic.endBatchEdit(); // Add, 180227 yunsuk
	}

	private void handleJapanKata(int primaryCode, int[] keyCodes) {
//		Log.i(TAG, "### handleJapanKata primaryCode : " + primaryCode + ", mComposing : " + mComposing.toString());

		int length = mComposing.length();
		InputConnection ic = getCurrentInputConnection();
		ic.beginBatchEdit();

		if (primaryCode >= JapaneseManager.KEYCODE_HIRA_START && primaryCode <= JapaneseManager.KEYCODE_HIRA_END) {

			if (mJpnManager.getEisukana()) {
				mJpnManager.setIsEisukana(false);
			}

			String hira = LanguageMap.getSimeji(primaryCode - JapaneseManager.KEYCODE_HIRA_START);

			////////////////////////////////////////////////////////////////////////////////////
			// Add, 180227 yunsuk [Hiragana Same Word Click]
			if (length > 0) {
				// Search SpecialSymbol And Commit
				String beforeWord = mComposing.substring(length - 1);
				if (LanguageMap.isHiraganaSymbol(beforeWord) == true) {
					ic.finishComposingText();
					mComposing.setLength(0);
				} else {
					// Search Same Word And Del
					long delayTime = System.currentTimeMillis() - mJapanKataInputTime;
					
					if(mJapanKataInputTime == 0 || delayTime > 1000) {
						Log.d(TAG, "handle Japan Kata Input Delay Time And Next Word : " + delayTime);
					} else {
						// Modified, 180709 Original
				boolean isCycle = LanguageMap.isHiraganaCycle(beforeWord, hira);
				if(isCycle == true) {
					mComposing.delete(length - 1, length);
				}
			}
			}
			}

			// Add, 180709 yunsuk [Jp Kata Input Recycle Time]
			mJapanKataInputTime = System.currentTimeMillis();
			////////////////////////////////////////////////////////////////////////////////////

			mComposing.append(hira);
			ic.setComposingText(mComposing, 1);

			// // Thread
			makeCandidateJpn();

		} else if (primaryCode == JapaneseManager.KEYCODE_TURN) { // turn
			if (length > 0) {
				String beforeWord = mComposing.substring(length - 1);
				mComposing.delete(length - 1, length);
				String afterWord = LanguageMap.getHiraganaCycleTableWord(beforeWord);
				mComposing.append(afterWord);

				ic.setComposingText(mComposing, 1);

				// Thread
				makeCandidateJpn();
			}
		} else if (primaryCode == JapaneseManager.KEYCODE_CHANGE_KEYBOARD) {

			ic.finishComposingText();

			mJpnManager.reset();
			mJpnManager.setKeyboardType(JapaneseManager.KEYBOARD_QWERTY);

			mCurKeyboard = mJpnKeyboard = mJpnHiraFromQwertyKeyboard;

			mKeyboardSwitcher.setDefaultKeyboard(mJpnKeyboard);

			mInputView.setKeyboard(mCurKeyboard);
			mInputView.KeyInit();
			mCandidateView.hideView();

		} else if (primaryCode == JapaneseManager.KEYCODE_EISUKANA) {

			mJpnManager.clickEisukana(ic);

			handleInputCandidate(mJpnManager.getJpnHanjaString());

		} else if (primaryCode == JapaneseManager.KEYCODE_SPACE) { // space
			String commitText = mJpnManager.getComposingText();

			if (commitText.length() > 0) {
				mJpnManager.reset();
			}
			getCurrentInputConnection().finishComposingText();
			getCurrentInputConnection().commitText("\u0020", 1);
			mComposing.setLength(0);

			mCandidateView.hideView();

		} else if (primaryCode == JapaneseManager.KEYCODE_REPLACE_TAK) {
			if (length > 0) {
				String hira = mComposing.substring(length - 1);

				String replaceString = LanguageMap.getTak(hira);

				if (replaceString != null && replaceString.length() > 0) {
					mComposing.replace(length - 1, length, replaceString);
					ic.setComposingText(mComposing, 1);

					// Update
					makeCandidateJpn();
				}
			}
		} else {
			if (primaryCode >= JapaneseManager.KEYCODE_SYMBOL_START
					&& primaryCode <= JapaneseManager.KEYCODE_SYMBOL_END) {
				
				String symbolWord = LanguageMap.getHiraganaSymbol(primaryCode - JapaneseManager.KEYCODE_SYMBOL_START);
			
				
				///////////////////////////////////////////////////////////////////////////
				// Add, 180227 yunsuk
				if(length > 0) {
					String beforeWord = mComposing.substring(length - 1);
					
					if(LanguageMap.isHiraganaSymbol(beforeWord) == true) {
						mComposing.delete(length - 1, length);
					} else {
						ic.finishComposingText();
						mComposing.setLength(0);
					}
				}
				///////////////////////////////////////////////////////////////////////////
				
				mComposing.append(symbolWord);
				ic.setComposingText(mComposing, 1);
			}
		}
		ic.endBatchEdit();
	}

	public void handleJapanQwerty(int primaryCode, int[] keyCodes) {

		if (primaryCode == JapaneseManager.KEYCODE_CHANGE_KEYBOARD) {

			if (mJpnManager.getEditString().length() > 0)
				getCurrentInputConnection().commitText(mJpnManager.getEditString(), 1);
			mJpnManager.reset();
			mJpnManager.setKeyboardType(JapaneseManager.KEYBOARD_MODE_JP_HIRA);

			mCurKeyboard = mJpnKeyboard = mJpnHiraKeyboard;

			mKeyboardSwitcher.setDefaultKeyboard(mJpnKeyboard);
			mInputView.setKeyboard(mCurKeyboard);

			mCandidateView.hideView();

		} else if (primaryCode == JapaneseManager.KEYCODE_SPACE) { // space
			String commitText = mJpnManager.getEditString();

			if (commitText.length() > 0) {
				mJpnManager.reset();
			}
			commitText += " ";
			getCurrentInputConnection().commitText(commitText, 1);
			mCandidateView.hideView();

		} else {
			// 4 words
			/* English is a four-letter word that is not a combination. */
			if (mJpnManager.getComposingLength() < LanguageMap.MAX_LENGTH) {
				if (isAlphabet(primaryCode) || primaryCode == JapaneseManager.KEYCODE_HIRA_HYPEN) {
					String curEng = String.valueOf((char) primaryCode);

					//////////////////////////////////////////////////////////////////////////////////////////////////////////////
					// Add, 151012 yunsuk
					if (primaryCode == JapaneseManager.KEYCODE_HIRA_HYPEN) {
						curEng = LanguageMap
								.getSimeji(JapaneseManager.KEYCODE_HIRA_HYPEN - JapaneseManager.KEYCODE_HIRA_START);

						// Add, 151104 yunsuk
						mComposing.append(curEng);
						getCurrentInputConnection().commitText(mJpnManager.getEditString(), 1);
						mJpnManager.reset();
						mCandidateView.hideView();
						return;
					}
					//////////////////////////////////////////////////////////////////////////////////////////////////////////////

					mComposing.append(curEng);

					String toEng = mJpnManager.getComposingText();

					String[] hira = LanguageMap.convertHiragana(toEng);

					if (hira != null) {
						String[] kata = LanguageMap.convertKatakana(toEng);

						mJpnManager.commitText(toEng, hira[0], kata[0]);

						if (hira[1].length() > 0) {
							mComposing.append(hira[1]);
						}
					}

					getCurrentInputConnection().setComposingText(mJpnManager.getEditString(), 1);

					// ## candidate thread
					makeCandidateJpn();

				} else {
					mJpnManager.reset();
				}
			}
		}
	}

	// Modified, 151109 yunsuk
	public void handlePinyinCN(int primaryCode, int[] keyCodes) {
		if (mComposing.length() < LIMIT_PINYIN) {
			if (isAlphabet(primaryCode)) {
				mComposing.append(String.valueOf((char) primaryCode));

				getCurrentInputConnection().setComposingText(mComposing, 1);

				boolean ismake = mPinyinManager.makePinyin(mComposing);

				if (ismake) {
					showCandidate(mPinyinManager.getGanData());
					// showCandidate(mPinyinManager.getBunData());
				}
			} else {
				mComposing.append(String.valueOf((char) primaryCode));
				getCurrentInputConnection().commitText(mComposing, 1);
				mComposing.setLength(0);
				mCandidateView.hideView();
			}
		}
	}

	private void handleCharacter(int primaryCode, int[] keyCodes) {
		if (isInputViewShown()) {
			if (mInputView.isShifted()) {
				primaryCode = Character.toUpperCase(primaryCode);
			}
		}

		if (isAlphabet(primaryCode)) {
			mComposing.append((char) primaryCode);
		} else {
			mComposing.append(String.valueOf((char) primaryCode));
		}
		getCurrentInputConnection().commitText(mComposing, 1);
		updateShiftKeyState(getCurrentInputEditorInfo());
		mComposing.setLength(0);
	}

	// changjie
	// Search word have a select Word
	public boolean onChangjieChooseWord(String word) {
		this.resetChangjieStroke();
		if (this.wordProcessor.getChinesePhraseDictLinkedHashMap(word) != null) {
			ArrayList<String> words = new ArrayList<String>(this.wordProcessor.getChinesePhraseDictLinkedHashMap(word));

			int size = words.size();
			if (size > 0) {
				String[] data = new String[size];
				for (int i = 0; i < size; i++) {
					data[i] = words.get(i);
				}
				showCandidate(data);

				return true;
			}
		}
		return false;
	}

	/** Input Chanjie */
	// yunsuk 2013.12.30
	public void handleChangjie(int primaryCode, int[] keyCodes) {

		if (isAlphabet(primaryCode)) {
			if (wordProcessor == null) {
				wordProcessor = new WordProcessor(this);
				wordProcessor.init();
			}

			int maxKeyNum = mChangjieKeyboard.isShifted() ? 2 : 5;

			if (strokecount < maxKeyNum) {

				charbuffer[strokecount++] = (char) primaryCode;

				String transChanjie = wordProcessor.translateToChangjieCode(String.valueOf((char) primaryCode));
				mComposing.append(transChanjie);

				getCurrentInputConnection().setComposingText(mComposing, 1);

				ArrayList<String> words = wordProcessor.getChineseWordDictArrayList(
						new String(this.charbuffer, 0, this.strokecount), mChangjieKeyboard.isShifted());

				int size = words.size();
				if (size > 0) {
					String[] data = new String[size];
					for (int i = 0; i < size; i++) {
						data[i] = words.get(i);
					}
					showCandidate(data);
				} else {
					hideCandidate();
				}
			}
		} else {

		}
	}

	public void handleLanguageReset() {
		Log.e(TAG, "### handleLanguageReset() ###");
		
		Keyboard current = mInputView.getKeyboard();

        if (isInputJapanese(current)) {
			getCurrentInputConnection().commitText(mComposing, 1);
			mJpnManager.reset();
		} else if (isInputPinyin(current)) {
			getCurrentInputConnection().commitText(mComposing, 1);
			mPinyinManager.reset();
		} else if (isInputChangjie(current)) {
			getCurrentInputConnection().commitText(mComposing, 1);
		}

		mComposing.setLength(0);

		mCandidateView.hideView();
	}

	// /////////////////////////// [[ Space ]]
	// //////////////////////////////////
	public void handleSpace(int primaryCode) {
		Log.d(TAG, "### handleSpace ###");

		mComposing.append("\u0020");

		getCurrentInputConnection().finishComposingText();
		getCurrentInputConnection().commitText("\u0020", 1);
		mComposing.setLength(0);
	}

	// /////////////// [[ Enter ]]
	public void handleEnter(int primaryCode) {
		handleLanguageReset();
		sendKeyChar((char) primaryCode);
	}

	// /////////////////// [[ Input Candidate ]]
	public void handleInputCandidate(String[] str) {
		if (str != null) {
			Keyboard current = mInputView.getKeyboard();
			// if(isInputJapanese(current))
			if (isInputJapanese(current) || isInputPinyin(current) || isInputChangjie(current)) {
				mCandidateView.insertData(getLayoutInflater(), str);
			}
		}
	}

	// ///////////////////////////////////////////////////////////////////////////////////
	public void SymbolMode_GlobalKeyTextChange() {

		String language = "";

		if (mBeforeKeyboard != null) {
			// language keytext save
            if (mBeforeKeyboard == mQwertyKeyboard) {
				language = getResources().getString(R.string.label_eng_key);
			} else if (mBeforeKeyboard == mJpnKeyboard) {
				language = getResources().getString(R.string.label_jp_key);
			} else if (mBeforeKeyboard == mPinyinKeyboard) {
				language = getResources().getString(R.string.label_pin_key);
			} else if (mBeforeKeyboard == mChangjieKeyboard) {
				language = getResources().getString(R.string.label_pin_key);
			} else if (mBeforeKeyboard == mRusKeyboard) {
				language = getResources().getString(R.string.label_ru_key);
			}
		}
		if (language.length() > 0) {
			// searchKey & key text change
			mSymbolsKeyboard.setChangeKeyText(language, LatinKeyboard.KEYCODE_GLOBAL);
			mSymbolsShiftedKeyboard.setChangeKeyText(language, LatinKeyboard.KEYCODE_GLOBAL);
		}
	}

	private String getWordSeparators() {
		return mWordSeparators;
	}

	public boolean isWordSeparator(int code) {
		String separators = getWordSeparators();
		return separators.contains(String.valueOf((char) code));
	}

	private boolean isAlphabet(int code) {
		if (Character.isLetter(code)) {
			return true;
		} else {
			return false;
		}
	}

	// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Default Method
	// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	private void keyDownUp(int keyEventCode) {
		getCurrentInputConnection().sendKeyEvent(new KeyEvent(KeyEvent.ACTION_DOWN, keyEventCode));
		getCurrentInputConnection().sendKeyEvent(new KeyEvent(KeyEvent.ACTION_UP, keyEventCode));
	}

	public void onText(CharSequence text) {
		InputConnection ic = getCurrentInputConnection();
		if (ic == null)
			return;
		ic.beginBatchEdit();
		if (mComposing.length() > 0) {
			commitTyped(ic);
		}
		ic.commitText(text, 0);
		ic.endBatchEdit();
		updateShiftKeyState(getCurrentInputEditorInfo());
	}

	@Override
	public void onUpdateSelection(int oldSelStart, int oldSelEnd, int newSelStart, int newSelEnd, int candidatesStart,
			int candidatesEnd) {

		InputConnection ic = getCurrentInputConnection();
		if (ic == null || mInputView == null)
			return;

		Keyboard curKeyboard = mInputView.getKeyboard();

		// Log.e(TAG, "onUpdateSelection ::: 1:[" + oldSelStart + "], 2:[" +
		// oldSelEnd + "], 3:[" + newSelStart + "], 4:["
		// + newSelEnd + "], 5:[" + candidatesStart + "], 6:[" + candidatesEnd +
		// "]");
		 
		if ((newSelStart != candidatesEnd) || (newSelEnd != candidatesEnd)) {

			boolean isResetComposing = true;
            if (isInputPinyin(curKeyboard)) {
				mPinyinManager.reset();
			} else if (isInputChangjie(curKeyboard)) {
				resetChangjieStroke();
			} else if (isInputJapanese(curKeyboard)) {

//				Log.e(TAG, "## onUpdateSelection ## mJpnManager.reset()");
//				Log.i(TAG, "oldSelStart : " + newSelStart + ", oldSelEnd : " + newSelEnd);
//				Log.i(TAG, "newSelStart : " + newSelStart + ", newSelEnd : " + newSelEnd);
//				Log.i(TAG, "candidatesStart : " + candidatesStart + ", candidatesEnd : " + candidatesEnd);
				
				// Add, 151013 yunsuk
				mJpnManager.reset();
			}

			// Add, 170417 yunsuk [Hangul Composing Problem - Marshmallow]
			if (isResetComposing) {
				mComposing.setLength(0);
				ic.finishComposingText();
				inputLanguageWord(""); // Add, 170527 yunsuk [CommitText () call when changing selection position]
			}

			hideCandidate();
		}

		super.onUpdateSelection(oldSelStart, oldSelEnd, newSelStart, newSelEnd, candidatesStart, candidatesEnd);
	}

	private boolean translateKeyDown(int keyCode, KeyEvent event) {

		mMetaState = MetaKeyKeyListener.handleKeyDown(mMetaState, keyCode, event);
		int c = event.getUnicodeChar(MetaKeyKeyListener.getMetaState(mMetaState));
		mMetaState = MetaKeyKeyListener.adjustMetaAfterKeypress(mMetaState);
		InputConnection ic = getCurrentInputConnection();
		if (c == 0 || ic == null) {
			return false;
		}

		boolean dead = false;

		if ((c & KeyCharacterMap.COMBINING_ACCENT) != 0) {
			dead = true;
			c = c & KeyCharacterMap.COMBINING_ACCENT_MASK;
		}

		if (mComposing.length() > 0) {
			char accent = mComposing.charAt(mComposing.length() - 1);
			int composed = KeyEvent.getDeadChar(accent, c);

			if (composed != 0) {
				c = composed;
				mComposing.setLength(mComposing.length() - 1);
			}
		}

		onKey(c, null);

		return true;
	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		Log.d(TAG, "### onKeyDown ###  keyCode : " + keyCode);

		switch (keyCode) {
		case KeyEvent.KEYCODE_BACK:
			if (event.getRepeatCount() == 0 && mInputView != null) {
				if (mInputView.handleBack()) {
					return true;
				}
			}
			break;

/////////////////////////////////////////////////////////////////
// add, 200420 ian, #Physical & Virtual Keyboard
		case KeyEvent.KEYCODE_DEL:
			if(PHYSICAL_KEYBOARD_ENABLE == true) {
				onKey(LatinKeyboard.KEYCODE_BACKSPACE, null);
				return true;
			} else {
			if (mComposing.length() > 0) {
				onKey(Keyboard.KEYCODE_DELETE, null);
				return true;
			}
			}
/////////////////////////////////////////////////////////////////
			break;
		case KeyEvent.KEYCODE_ENTER:
			if(PHYSICAL_KEYBOARD_ENABLE == true) {
				onKey(LatinKeyboard.KEYCODE_ENTER, null);
			} else {
			onKey(KeyEvent.KEYCODE_ENTER, null);
			}
			return true;

///////////////////////////////////////////////////////////////////
// add, 200420 ian, #Physical & Virtual Keyboard
		case KeyEvent.KEYCODE_CAPS_LOCK:
			if(PHYSICAL_KEYBOARD_ENABLE == true) {
				Log.d(TAG, "### onKeyDown KEYCODE_CAPS_LOCK => LatinKeyboard.KEYCODE_SHIFT ###");
				onKey(LatinKeyboard.KEYCODE_SHIFT, null);
				
				return true;
			}
			break;
		case KeyEvent.KEYCODE_KANA:
			if(PHYSICAL_KEYBOARD_ENABLE == true) {
				Log.d(TAG, "### onKeyDown KEYCODE_KANA => KEYCODE_GLOBAL ###");
				onKey(LatinKeyboard.KEYCODE_GLOBAL, null);
				return true;
			}
			break;
//////////////////////////////////////////////////////////////////

		default:
			if (PROCESS_HARD_KEYS) {
				if (keyCode == KeyEvent.KEYCODE_SPACE && (event.getMetaState() & KeyEvent.META_ALT_ON) != 0) {

					InputConnection ic = getCurrentInputConnection();
					if (ic != null) {
						ic.clearMetaKeyStates(KeyEvent.META_ALT_ON);
						keyDownUp(KeyEvent.KEYCODE_A);
						keyDownUp(KeyEvent.KEYCODE_N);
						keyDownUp(KeyEvent.KEYCODE_D);
						keyDownUp(KeyEvent.KEYCODE_R);
						keyDownUp(KeyEvent.KEYCODE_O);
						keyDownUp(KeyEvent.KEYCODE_I);
						keyDownUp(KeyEvent.KEYCODE_D);
						return true;
					}
				}
				if (mPredictionOn && translateKeyDown(keyCode, event)) {
					return true;
				}
			}
		}
		return super.onKeyDown(keyCode, event);
	}

	@Override
	public boolean onKeyUp(int keyCode, KeyEvent event) {
		if (PROCESS_HARD_KEYS) {
			if (mPredictionOn) {
				mMetaState = MetaKeyKeyListener.handleKeyUp(mMetaState, keyCode, event);
			}
		}
		return super.onKeyUp(keyCode, event);
	}

	private void commitTyped(InputConnection inputConnection) {
		if (mComposing.length() > 0) {
			Log.d(TAG, "#### commitTyped ###");
			inputConnection.commitText(mComposing, mComposing.length());
			mComposing.setLength(0);
		}
	}

	private void updateShiftKeyState(EditorInfo attr) {

		if (attr != null && mInputView != null && (mQwertyKeyboard == mInputView.getKeyboard()
				|| mJpnEngKeyboard == mInputView.getKeyboard() || mRusKeyboard == mInputView.getKeyboard())) {
			int caps = 0;
			EditorInfo ei = getCurrentInputEditorInfo();
			if (ei != null && ei.inputType != EditorInfo.TYPE_NULL) {
				caps = getCurrentInputConnection().getCursorCapsMode(attr.inputType);
			}
			// yunsuk 2014.02.18
			if (!isShiftLockEnablaed) {
				mInputView.setShifted(false);

				// Add, 170526 yunsuk [Shift State Change]
				((LatinKeyboard) mInputView.getKeyboard()).setShiftIcon(false, mShiftNormalIcon);
			}
			// # Original Shift State Change
			// mInputView.setShifted(mCapsLock || caps != 0);
		}
	}

	private void sendKey(int keyCode) {
		switch (keyCode) {
		case '\n':
			keyDownUp(KeyEvent.KEYCODE_ENTER);
			break;
		default:
			if (keyCode >= '0' && keyCode <= '9') {
				keyDownUp(keyCode - '0' + KeyEvent.KEYCODE_0);
			} else {
				getCurrentInputConnection().commitText(String.valueOf((char) keyCode), 1);
			}
			break;
		}
	}

	@Override
	public void onBindInput() {
		super.onBindInput();
	}

	public void swipeRight() {
		// Drag
	}

	public void swipeLeft() {
	}

	public void swipeDown() {
	}

	public void swipeUp() {
	}

	public void onPress(int primaryCode) {
		// Real Press Key
		// Log.i(TAG, "onPress");
	}

	public void onRelease(int primaryCode) {
		// Real Release Key
		// Log.i(TAG, "onRelease");
		// mInputView.setPreviewEnabled(true);
	}

	@Override
	public void onConfigurationChanged(Configuration newConfig) {
		super.onConfigurationChanged(newConfig);
	}

	// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Listener
	// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	private onKeyboardViewListener mOnKeyboardViewListener = new onKeyboardViewListener() {

		@Override
		public void onBlankEvent() {
			mJpnManager.reset();
			mCandidateView.hideView();
			requestHideSelf(0);
		}

		@Override
		public void onSimejiInputKey(String keyText) {
			mComposing.append(keyText);
			getCurrentInputConnection().setComposingText(mComposing, 1);
		}

		@Override
		public void onLongPressKey(Key key) {
			if (isInputViewKeyboard()) {
				LatinKeyboard currentKeyboard = (LatinKeyboard) mInputView.getKeyboard();

				if (key != null && key.codes[0] == Keyboard.KEYCODE_SHIFT && ((mQwertyKeyboard == currentKeyboard)
						|| (mJpnEngKeyboard == currentKeyboard) || (mRusKeyboard == currentKeyboard))) {
					isShiftLockEnablaed = false;

					currentKeyboard.setShiftIcon(true, mShiftLockIcon);
					mInputView.setShifted(true);
				}
			}
		}

		@Override
		public void onTouchUp(int x, int y) {
		}

		@Override
		public void onCommitText(String txt) {
			getCurrentInputConnection().commitText(txt, 1);
		}
	};

	private CandidateView.onBtnNotificationListener mCandidateListener = new CandidateView.onBtnNotificationListener() {

		@Override
		public void onBtnCandidateListShow(boolean isShow) {
			if (isShow) {
				mCandidateView.setShowCandidateDataViewHeight(0);
				mInputView.setVisibility(View.VISIBLE);
			} else {
				mCandidateView.setShowCandidateDataViewHeight(mInputView.getHeight());
				mInputView.setVisibility(View.GONE);
			}
		}

		@Override
		public void onBtnText(String text, int index) {
			boolean isCandidateShow = false;
			LatinKeyboard current = mInputView.getKeyboard();

			if (isInputJapanese(current) && mJpnManager != null) {
				WnnWord ww = mJpnManager.getWord(index);
				mJpnManager.addWord(ww);
				mJpnManager.reset();
			} else if (isInputPinyin(current)) {
				mPinyinManager.reset();
			} else if (isInputChangjie(current)) {
				isCandidateShow = onChangjieChooseWord(text);
			}

			if (!isCandidateShow) {
				hideCandidate();
			}

			getCurrentInputConnection().commitText(text, 1);
			mComposing.setLength(0);
		}

		@Override
		public void onCandidateShow(boolean isShow) {
			setCandidatesViewShown(isShow);
		}
	};
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// candidate -> touch key -> input text
	public void inputLanguageWord(String word) {
		InputConnection ic = getCurrentInputConnection();
		ic.beginBatchEdit();
		ic.commitText(word, 1);
		ic.endBatchEdit();
	}

	private void hideCandidate() {
		if (mCandidateView != null) {
			mCandidateView.hideView();
			if (mInputView.getVisibility() == View.GONE) {
				mInputView.setVisibility(View.VISIBLE);
				mCandidateView.setShowCandidateDataViewHeight(0);
			}
		}
	}

	private boolean isInputViewKeyboard() {
		return (mInputView != null && mInputView.getKeyboard() != null);
	}

	private boolean isInputRus(Keyboard cKeyboard) {
		return (isInputViewKeyboard() && mRusKeyboard != null && cKeyboard == mRusKeyboard);
	}

	private boolean isInputSymbol(Keyboard cKeyboard) {
		return (isInputViewKeyboard() && mSymbolsKeyboard != null && cKeyboard == mSymbolsKeyboard);
	}

	private boolean isInputSymbolShift(Keyboard cKeyboard) {
		return (isInputViewKeyboard() && mSymbolsShiftedKeyboard != null && cKeyboard == mSymbolsShiftedKeyboard);
	}

	private boolean isInputQwerty(Keyboard cKeyboard) {
		return (isInputViewKeyboard() && mQwertyKeyboard != null && cKeyboard == mQwertyKeyboard);
	}

	private boolean isInputPinyin(Keyboard cKeyboard) {
		return (isInputViewKeyboard() && mPinyinKeyboard != null && cKeyboard == mPinyinKeyboard);
	}

	private boolean isInputChangjie(Keyboard cKeyboard) {
		return (isInputViewKeyboard() && mChangjieKeyboard != null && cKeyboard == mChangjieKeyboard);
	}

	private boolean isInputJapanese(Keyboard cKeyboard) {
		return (isInputViewKeyboard() && ((mJpnHiraKeyboard != null && cKeyboard == mJpnHiraKeyboard)
				|| (mJpnNumberKeyboard != null && cKeyboard == mJpnNumberKeyboard)
				|| (mJpnEngKeyboard != null && cKeyboard == mJpnEngKeyboard)
				|| (mJpnSymbolKeyboard != null && cKeyboard == mJpnSymbolKeyboard)
				|| (mJpnHiraFromQwertyKeyboard != null && cKeyboard == mJpnHiraFromQwertyKeyboard)));
	}

	// Add, 151106 yunsuk
	private void showCandidate(final String[] data) {
		mHandler.postDelayed(new Runnable() {
			@Override
			public void run() {
				if (data != null && data.length > 0) {
					handleInputCandidate(data);
					mCandidateView.setWidth(mInputView.getWidth());
					setCandidatesViewShown(true);
				}
			}
		}, 10);
	}

	// Add, 151106 yunsuk
	private void makeCandidateJpn() {
		if (mJpnManager != null) {
			Thread thread = new Thread(new Runnable() {

				@Override
				public void run() {
					try {
						Thread.sleep(10);
					} catch (InterruptedException e) {
						;
					}

					mJpnManager.SearchWnnEngineJAJP();

					showCandidate(mJpnManager.getJpnHanjaString());
				}
			});
			thread.run();
		}
	}
}
