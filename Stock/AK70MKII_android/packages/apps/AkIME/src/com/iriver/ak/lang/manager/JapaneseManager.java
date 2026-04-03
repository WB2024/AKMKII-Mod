package com.iriver.ak.lang.manager;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import com.iriver.ak.openwnn.KanaConverter;
import com.iriver.ak.openwnn.OpenWnnClauseConverterJAJP;
import com.iriver.ak.openwnn.OpenWnnDictionaryImpl;
import com.iriver.ak.openwnn.WnnDictionary;
import com.iriver.ak.openwnn.WnnWord;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.style.BackgroundColorSpan;
import android.text.style.CharacterStyle;
import android.view.inputmethod.InputConnection;

public class JapaneseManager {
	// //////////////////////////////////////////////////////////////////////////
	/** Score(frequency value) of word in the learning dictionary */
	public static final int FREQ_LEARN = 600;
	/** Score(frequency value) of word in the user dictionary */
	public static final int FREQ_USER = 500;

	/** Keyboard type (Qwerty) */
	public static final int KEYBOARD_QWERTY = 2;

	/** Keyboard type (Simeji) */
	public static final int KEYBOARD_MODE_JP_HIRA = 1;

	public static final int KEYBOARD_MODE_JP_QWERTY = 3;
	public static final int KEYBOARD_MODE_JP_NUMBER = 4;
	public static final int KEYBOARD_MODE_JP_SYMBOL = 5;

	/** Maximum limit length of output */
	public static final int MAX_OUTPUT_LENGTH = 10;

	/** Limitation of predicted candidates */
	public static final int PREDICT_LIMIT = 100;

	/** */
	public static final int KEYCODE_TURN = 200;
	public static final int KEYCODE_CHANGE_KEYBOARD = 201;
	public static final int KEYCODE_EISUKANA = 202;
	public static final int KEYCODE_SPACE = 203;
	public static final int KEYCODE_REPLACE_TAK = 204;
	public static final int KEYCODE_REPLACE_L_U = 204;
	public static final int KEYCODE_SYMBOL_START = 205;
	public static final int KEYCODE_SYMBOL_END = 208;
	public static final int KEYCODE_JPN_GLOBAL = 209;
	
	public static final int KEYCODE_HIRA_START = 300;
//	public static final int KEYCODE_HIRA_END = 345;
	public static final int KEYCODE_HIRA_END = 346;
	public static final int KEYCODE_HIRA_HYPEN = 346; // Add, 151013 yunsuk
	
	public static final int KEYCODE_NUMBER_0 = 48;
	public static final int KEYCODE_NUMBER_9 = 57;

	/* Dictionary JP */
	private WnnDictionary mDictionaryJP = null;

	/* converters */
	private OpenWnnClauseConverterJAJP mClauseConverter = new OpenWnnClauseConverterJAJP();

	private ArrayList<WnnWord> mJpnHanjaString = new ArrayList<WnnWord>();

	private final String LIB_PATH = "/lib/libWnnJpnDic.so";
	private final String SYS_LIB_PATH = "/system/lib64/libWnnJpnDic.so";
	private final String DB_PATH = "/JpnDic/jpnDic.dic";

	private JpnTextClass mJpnTextManager = null;

	/** Kana converter (for EISU-KANA conversion) */
	private KanaConverter mKanaConverter;

	private int mKeyboardType = KEYBOARD_MODE_JP_HIRA;

	/** HashMap for checking duplicate word */
	private HashMap<String, WnnWord> mCandTable;

	/** Japanese Eisukana Selection */
	private boolean mIsEisukana = false;
	private SpannableStringBuilder mDisplayText = new SpannableStringBuilder();
	private final CharacterStyle SPAN_EXACT_BGCOLOR_HL = new BackgroundColorSpan(
			0xFF66CDAA);

	public JapaneseManager(Context context) {
		// TODO Auto-generated constructor stub
		CreateOpenWnnEngineJAJP(context);
		mJpnTextManager = new JpnTextClass();
		mKanaConverter = new KanaConverter();
		mCandTable = new HashMap<String, WnnWord>();
	}

	public void CreateOpenWnnEngineJAJP(Context context) {
		if (mDictionaryJP == null) {

			ApplicationInfo info = context.getApplicationInfo();
			String libPath = info.dataDir + LIB_PATH;
			String dbPath = info.dataDir + DB_PATH;

			/* load Japanese dictionary library */
			mDictionaryJP = new OpenWnnDictionaryImpl(context, libPath, dbPath);

			if (!mDictionaryJP.isActive()) {
				mDictionaryJP = new OpenWnnDictionaryImpl(context,
						SYS_LIB_PATH, dbPath);
			}

			/* clear dictionary settings */
			mDictionaryJP.clearDictionary();
			mDictionaryJP.clearApproxPattern();
			mDictionaryJP.setInUseState(false);

			/* WnnEngine's interface */
			mClauseConverter.setDictionary(mDictionaryJP);
		}
	}

	public String[] getJpnHanjaString() {

		int size = mJpnHanjaString.size();
		if (size > 0) {
			String[] str = new String[size];
			for (int i = 0; i < size; i++) {
				str[i] = mJpnHanjaString.get(i).candidate;
			}
			return str;
		}
		return null;
	}

	private boolean addCandidate(WnnWord word) {
		if (word.candidate == null || mCandTable.containsKey(word.candidate)
				|| word.candidate.length() > MAX_OUTPUT_LENGTH) {
			return false;
		}
		mCandTable.put(word.candidate, word);
		mJpnHanjaString.add(word);
		return true;
	}

	private void addArrayJpnString(WnnDictionary wnnDic, String hira) {
		WnnWord word = null;
		String hiraWord = "";
		boolean isSame = false;

		mCandTable.clear();
		mJpnHanjaString.clear();

		while ((word = wnnDic.getNextWord()) != null) {

			if (word.frequency == 0) { // 1 word
				if (hira.equals(word.stroke) && word.candidate.length() > 0) {
					addCandidate(word);
				}
			} else {
				isSame = false;

				if (!hiraWord.equals(word.stroke)) {
					for (int i = mJpnHanjaString.size() - 1; i >= 0; i--) {
						// same word not add

						if (mJpnHanjaString.get(i).equals(word.candidate)
								|| word.candidate.length() <= 0) {
							isSame = true;
							break;
						}
					}
					if (!isSame) {
						addCandidate(word);
					}
				}
			}
		}

		/* get candidates by single clause conversion */
		Iterator<?> convResult = mClauseConverter.convert(hira);
		if (convResult != null) {

			while (convResult.hasNext()) {
				WnnWord ww = (WnnWord) convResult.next();

				addCandidate(ww);
			}
		}

		/* get candidates from Kana converter */
		if (mKanaConverter != null) {
			String en = mJpnTextManager.getEng()
					+ mJpnTextManager.getComposingText();// mJpnTextManager.getCuEn();

			List<WnnWord> addCandidateList = mKanaConverter
					.createPseudoCandidateList(hira, en, mKeyboardType);

			Iterator<WnnWord> it = addCandidateList.iterator();

			while (it.hasNext()) {
				addCandidate(it.next());
			}
		}
	}

	public WnnWord getWord(int index) {

		if (mJpnHanjaString.size() >= index) {
			return mJpnHanjaString.get(index);
		}

		return null;
	}

	/** @see add User Dictionary Word */
	public int addWord(WnnWord word) {
		mDictionaryJP.setInUseState(true);
		if (word.partOfSpeech == null) {
			word.partOfSpeech = mDictionaryJP
					.getPOS(WnnDictionary.POS_TYPE_MEISI);
		}
		mDictionaryJP.addWordToUserDictionary(word);
		mDictionaryJP.setInUseState(false);
		return 0;
	}

	/** @see Search Hira Maching Kata Word */
	public void SearchKaTa() {
		if (mKeyboardType == KEYBOARD_MODE_JP_HIRA) {
			mCandTable.clear();
			mJpnHanjaString.clear();

			String hira = mJpnTextManager.getComposingText();

			/* get candidates from Kana converter */
			if (mKanaConverter != null) {

				List<WnnWord> addCandidateList = mKanaConverter
						.createPseudoCandidateList(hira, "", mKeyboardType);

				Iterator<WnnWord> it = addCandidateList.iterator();

				while (it.hasNext()) {
					addCandidate(it.next());
				}
			}
		}
	}

	/** @see Search Match Candidate Word */
	public void SearchWnnEngineJAJP() {

		if (mDictionaryJP != null) {

			int candidates = 0;

			String hira = "";
			if (mKeyboardType == KEYBOARD_QWERTY)
				hira = mJpnTextManager.getEditString();
			if (mKeyboardType == KEYBOARD_MODE_JP_HIRA)
				hira = mJpnTextManager.getComposingText();

			int hiraSize = hira.length();

			// 2015.01.16 yunsuk
			// Backspace Sync
			if (hiraSize <= MAX_OUTPUT_LENGTH) {
				setDictionaryForPrediction(hiraSize);

				/* search dictionaries */
				mDictionaryJP.setInUseState(true);

				if (hiraSize == 0) { // backspace
					/* search by previously selected word */
					candidates = mDictionaryJP.searchWord(
							WnnDictionary.SEARCH_LINK,
							WnnDictionary.ORDER_BY_FREQUENCY, hira, null);
				} else {

					if (false) {
						/* exact matching */
						candidates = mDictionaryJP.searchWord(
								WnnDictionary.SEARCH_EXACT,
								WnnDictionary.ORDER_BY_FREQUENCY, hira);
					} else {
						/* prefix matching */
						candidates = mDictionaryJP.searchWord(
								WnnDictionary.SEARCH_PREFIX,
								WnnDictionary.ORDER_BY_FREQUENCY, hira);
					}
				}
				addArrayJpnString(mDictionaryJP, hira);
			}
		}
	}

	/**
	 * Set dictionary for prediction.
	 * 
	 * @param strlen
	 *            Length of input string
	 */

	private void setDictionaryForPrediction(int paramInt) {
		WnnDictionary localWnnDictionary = this.mDictionaryJP;
		localWnnDictionary.clearDictionary();
		// if (this.mDictType != 4)
		{
			localWnnDictionary.clearApproxPattern();
			if (paramInt != 0) {
				localWnnDictionary.setDictionary(0, 100, 400);
				if (paramInt > 1)
					localWnnDictionary.setDictionary(1, 100, 400);
				localWnnDictionary.setDictionary(2, 245, 245);
				localWnnDictionary.setDictionary(3, 100, 244);
				localWnnDictionary.setDictionary(-1, 500, 500);
				localWnnDictionary.setDictionary(-2, 600, 600);

				if (this.mKeyboardType != KEYBOARD_QWERTY)
					localWnnDictionary
							.setApproxPattern(WnnDictionary.APPROX_PATTERN_JAJP_12KEY_NORMAL);
			} else {
				localWnnDictionary.setDictionary(2, 245, 245);
				localWnnDictionary.setDictionary(3, 100, 244);
				localWnnDictionary.setDictionary(-2, 600, 600);
			}
		}
	}

	/** @see Click Eisukana Selection Word */
	public void clickEisukana(InputConnection ic) {
		mIsEisukana = !mIsEisukana;

		String composing = mJpnTextManager.getComposingText();

		if (composing.length() > 0) {
			if (mIsEisukana) {
				mDisplayText.clear();
				mDisplayText.insert(0, composing);
				mDisplayText.setSpan(SPAN_EXACT_BGCOLOR_HL, 0,
						composing.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
				ic.setComposingText(mDisplayText, 1);

				SearchKaTa();
			} else {
				ic.setComposingText(mJpnTextManager.getComposingText(), 1);

				SearchWnnEngineJAJP();
			}
		}
	}

	public void setIsEisukana(boolean isEisukana) {
		mIsEisukana = isEisukana;
	}

	public boolean getEisukana() {
		return mIsEisukana;
	}

	public void setKeyboardType(int type) {
		mKeyboardType = type;
	}

	public int getKeyboardType() {
		return mKeyboardType;
	}

	public void reset() {
		// 2015.01.16 yunsuk
		// Space Clear
		mJpnHanjaString.clear();
		mJpnTextManager.reset();
		setIsEisukana(false);
	}

	public void OpenWnnTextManagerBackSpace() {
		mJpnTextManager.backspace();
	}

	public void OpenWnnEngineBackSpace() {
		mJpnTextManager.backspace();

		SearchWnnEngineJAJP();
	}

	public String getHira() {
		return mJpnTextManager.getHira();
	}

	public String getEn() {
		return mJpnTextManager.getEng();
	}

	public String getKata() {
		return mJpnTextManager.getKata();
	}

	public String getEditString() {
		if (mKeyboardType == KEYBOARD_QWERTY)
			return mJpnTextManager.getEditString();
		return mJpnTextManager.getComposingText();
	}

	public void commitText(String en, String hira, String kata) {
		mJpnTextManager.commitText(en, hira, kata);
	}

	public void setComposing(StringBuilder composing) {
		mJpnTextManager.setComposing(composing);
	}

	public StringBuilder getComposing() {
		return mJpnTextManager.getComposing();
	}

	public String getComposingText() {
		return mJpnTextManager.getComposingText();
	}

	public int getComposingLength() {
		return mJpnTextManager.getComposingText().length();
	}

	/**
	 * @class JpnTextClass
	 * @author yunsuk
	 *
	 */
	class JpnTextClass {
		private ArrayList<String> mEn = new ArrayList<String>();
		// private String mComposingText = "";
		private String mHira = "";
		private String mKata = "";
		private StringBuilder mComposing = new StringBuilder();

		public JpnTextClass() {

		}

		public void setComposing(StringBuilder composing) {
			mComposing = composing;
		}

		public void commitText(String en, String hira, String kata) {

			mHira += hira;
			mKata += kata;

			mEn.add(en);

			if (mKeyboardType == KEYBOARD_QWERTY)
				mComposing.setLength(0);
		}

		public void backspace() {

			final int length = mComposing.length();

			if (length > 1) {
				mComposing.delete(length - 1, length);
			} else if (length > 0) {
				mComposing.setLength(0);
			} else {
				if (mEn.size() > 1) {
					mEn.remove(mEn.size() - 1);
				} else {
					mEn.clear();
				}

				if (mHira.length() > 1) {
					mHira = mHira.substring(0, mHira.length() - 1);
				} else {
					mHira = "";
				}

				if (mKata.length() > 1) {
					mKata = mKata.substring(0, mKata.length() - 1);
				} else {
					mKata = "";
				}
			}
		}

		public void reset() {
			mEn.clear();
			mHira = "";
			mKata = "";
			mComposing.setLength(0);
		}

		public StringBuilder getComposing() {
			return mComposing;
		}

		public String getComposingText() {
			return mComposing.toString();
		}

		public String getEng() {
			String temp = "";
			for (int i = 0; i < mEn.size(); i++)
				temp += mEn.get(i);
			return temp;
		}

		public String getHira() {
			return mHira;
		}

		public String getKata() {
			return mKata;
		}

		public String getEditString() {
			return getHira() + mComposing.toString();
		}
	}
}
