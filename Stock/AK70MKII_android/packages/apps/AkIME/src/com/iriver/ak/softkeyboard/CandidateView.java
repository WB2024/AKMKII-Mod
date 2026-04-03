package com.iriver.ak.softkeyboard;

import com.iriver.ak.softkeyboard.R;

import android.content.Context;
import android.os.Handler;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

/**
 * CandidateView
 * 
 * @author yunsuk
 * 
 * @since 2013.10.28 mon
 * @see : Japan and Chinese Language Data PopupWindow
 * 
 */
public class CandidateView extends LinearLayout {

	// Keyboard Mode
	private int mLanguageMode = 1;
	public final static int JapanMode = 1;
	public final static int PinyinMode = 2;
	public final static int ChangjieMode = 3;

	// data cnt
	private int mDefCandiDataHalfCnt = 4;
	private final int DEFAULT_CANDIDATE_CNT = 8;

	// Layout
	private ScrollView mCandidateDataScrollView = null;
	private LinearLayout mCandidateDataListView = null;
	private LinearLayout mCandidateUpDataView = null;
	private LinearLayout mCandidateDownDataView = null;

	// Button
	private ImageView mCandidateShowBtn = null;

	// Add, 170525 yunsuk [Cneter Line]
	private View mCandidateCenterLine = null;
	
	//
	private boolean mIsInputViewShow = true;

	//
	private LinearLayout mCandidateTopLayout = null;

	public CandidateView(Context context, View v) {
		super(context);

		addView(v);

		// default candidate view
		mCandidateUpDataView = (LinearLayout) v.findViewById(R.id.candidateview_up_data_layout);
		mCandidateDownDataView = (LinearLayout) v.findViewById(R.id.candidateview_down_data_layout);

		// btn
		mCandidateShowBtn = (ImageView) v.findViewById(R.id.candidate_show_btn);
		mCandidateShowBtn.setOnClickListener(new View.OnClickListener() {

			@SuppressWarnings("deprecation")
			@Override
			public void onClick(View v) {
				// TODO Auto-generated method stub

				int cnt = mCandidateDataListView.getChildCount();

				if (cnt > 0) {
					if (mCandidateDataScrollView.getHeight() == 0) {
						mCandidateShowBtn.setImageResource(R.drawable.icon_list_arrow_up);
						mIsInputViewShow = false;
					} else {
						mCandidateShowBtn.setImageResource(R.drawable.icon_list_arrow_down);
						mIsInputViewShow = true;
					}
				} else {
					// mCandidateShowBtn
					// .setBackgroundDrawable(mCandidateBtnDisable);
					mIsInputViewShow = false;
				}

				mOnBtnListener.onBtnCandidateListShow(mIsInputViewShow);
			}
		});

		//
		mCandidateDataScrollView = (ScrollView) v.findViewById(R.id.under_data_scrollview_layout);
		mCandidateDataListView = (LinearLayout) v.findViewById(R.id.under_data_layout);

		//
		mCandidateTopLayout = (LinearLayout) v.findViewById(R.id.candidateview_top_layout);
		
		mCandidateCenterLine =  v.findViewById(R.id.candidateview_centerline);
	}

	public void setWidth(int width) {
		ViewGroup.LayoutParams vl = mCandidateTopLayout.getLayoutParams();
		vl.width = width;
		mCandidateTopLayout.setLayoutParams(vl);
	}

	public void backSpace(LayoutInflater inflater, String[] str) {
		switch (mLanguageMode) {
		case JapanMode:
			backSpace_Japan(inflater, str);
			break;
		case PinyinMode:
			backSpace_Pinyin(inflater, str);
			break;
		case ChangjieMode:
			backSpace_Changjie(inflater, str);
			break;
		}
	}

	public void backSpace_Changjie(LayoutInflater inflater, String[] data) {
		if (data != null) {
			addCandidateData(inflater, data);
		} else {
			hideView();
		}
	}

	public void backSpace_Japan(LayoutInflater inflater, String[] hanja) {
		if (hanja != null) {
			addCandidateData(inflater, hanja);
		} else {
			hideView();
		}
	}

	/**
	 * @call HanjaDataView [Show and Hide]
	 * @user yunsuk
	 * @method setShowCandidateDataViewHeight
	 * @param height
	 */
	public void setShowCandidateDataViewHeight(int height) {
		LinearLayout.LayoutParams lcdsv = (LinearLayout.LayoutParams) mCandidateDataScrollView.getLayoutParams();
		lcdsv.height = height;
		mCandidateDataScrollView.setLayoutParams(lcdsv);

		if (height > 0) {
			Animation anim = AnimationUtils.loadAnimation(getContext(), R.anim.up_trans);
			mCandidateDataScrollView.startAnimation(anim);
		}
	}

	/**
	 * @call Candidate ChildView Remove AllView
	 * @user yunsuk
	 * @method removeCandidateDataView()
	 */
	public void removeCandidateDataView() {
		if (mCandidateDataListView != null && mCandidateDataListView.getChildCount() > 0)
			mCandidateDataListView.removeAllViews();
		if (mCandidateUpDataView != null && mCandidateUpDataView.getChildCount() > 0)
			mCandidateUpDataView.removeAllViews();
		if (mCandidateDownDataView != null && mCandidateDownDataView.getChildCount() > 0)
			mCandidateDownDataView.removeAllViews();
	}


	public void insertData(LayoutInflater inflater, String[] data) {
		addCandidateData(inflater, data);
	}

	
	/**
	 * @user yunsuk
	 * @param LayoutInflater
	 * @param String[]
	 */
	public void addCandidateData(LayoutInflater inflater, String[] str) {

		removeCandidateDataView();

		if (mCandidateDataListView != null && str != null && str.length > 0) {
//			mDefCandiDataHalfCnt = 4;

			// kanji
			int size = str.length;
			int min = size > DEFAULT_CANDIDATE_CNT ? DEFAULT_CANDIDATE_CNT : 0;

			if(size <= DEFAULT_CANDIDATE_CNT) {
//				mDefCandiDataHalfCnt = 5;
				mCandidateShowBtn.setVisibility(View.GONE);
			} else {
				mCandidateShowBtn.setVisibility(View.VISIBLE);
			}
			
			// def
			createCandidateDefaultData(inflater, str);

			if (min >= DEFAULT_CANDIDATE_CNT) {

				mCandidateShowBtn.setImageResource(R.drawable.icon_list_arrow_down);
				mCandidateShowBtn.setEnabled(true);

				AddCandidateThread candiListThread = new AddCandidateThread(inflater, str, min, size);
				candiListThread.setDaemon(true);
				candiListThread.start();

			} else {
				// Disable Arrow
				// mCandidateShowBtn.setBackgroundDrawable(mCandidateBtnDisable);
				mCandidateShowBtn.setEnabled(false);
			}
		}
	}

	private int addCandidateTextView(LayoutInflater inflater, LinearLayout parentView, String[] str, int start, int end, boolean isTopData) {
		int sumCnt = 0;
		if (parentView != null) {
			parentView.setVisibility(View.VISIBLE);

		float textWidth = getResources().getDimension(R.dimen.jp_popup_text_width);
		float textHeight = getResources().getDimension(R.dimen.jp_popup_text_height);
			if(isTopData == false) {
				textWidth = getResources().getDimensionPixelOffset(R.dimen.jp_popup_text_width_expanded);
				textHeight = getResources().getDimensionPixelOffset(R.dimen.jp_popup_text_height_expanded);
			}

//			LinearLayout.LayoutParams textLp = new LinearLayout.LayoutParams((int) textWidth, (int) textHeight);
//			textLp.weight = 1;
			LinearLayout.LayoutParams textLp = new LinearLayout.LayoutParams(LayoutParams.WRAP_CONTENT,
					LayoutParams.MATCH_PARENT, 1);

			String sumStr = "";

			for (int i = start; i < end; i++) {
//				if (false) {
//					String s = str[i];
//					if ((i < end - 1) && (s.length() > 4)) {
//						end--;
//						sumCnt++;
//						textLp.width = (int) (textWidth * 2);
//					}
//				}

				TextView tv = (TextView) inflater.inflate(R.layout.candidate_textview, null);
				tv.setText(str[i]);
				tv.setTag(i);
				tv.setLayoutParams(textLp);
				tv.setOnClickListener(onClick);
				if (i == end - 1) {
					tv.setCompoundDrawables(null, null, null, null);
				}
				parentView.addView(tv);
			}
		}
		
		return sumCnt;
			}
 

	private void createCandidateDefaultData(LayoutInflater inflater, String[] str) {
		int strSize = str.length;
		int size = strSize > DEFAULT_CANDIDATE_CNT ? DEFAULT_CANDIDATE_CNT : strSize;
		
		if (size >= mDefCandiDataHalfCnt) {

			if(true) {
				addCandidateTextView(inflater, mCandidateUpDataView, str, 0, mDefCandiDataHalfCnt, true);
				addCandidateTextView(inflater, mCandidateDownDataView, str, mDefCandiDataHalfCnt, size, true);
				mCandidateCenterLine.setVisibility(View.VISIBLE);
			} else {
				// Calc start, End
//				int topStart = 0;
//				int topEnd = mDefCandiDataHalfCnt;
//
//				int nextNum = addCandidateTextView(inflater, mCandidateUpDataView, str, topStart, topEnd, true);
//				int nextStart = topEnd - nextNum;
//				int nextEnd = size - nextNum;
//				addCandidateTextView(inflater, mCandidateDownDataView, str, nextStart, nextEnd, true);
			}
		} else {
			addCandidateTextView(inflater, mCandidateUpDataView, str, 0, size, true);

			mCandidateDownDataView.setVisibility(View.GONE);
			mCandidateCenterLine.setVisibility(View.GONE);
		}
	}

	/* Thread */
	class AddCandidateThread extends Thread {
		private LayoutInflater mInflater;
		private String[] mString;
		private int mStart;
		private int mEnd;

		AddCandidateThread(LayoutInflater inflater, String[] str, int min, int size) {
			mInflater = inflater;
			mString = str;
			mStart = min;
			mEnd = size;
		}

		public void run() {

			try {
				Thread.sleep(10);
			} catch (InterruptedException e) {
				;
			}

			mHandler.post(new Runnable() {

				@Override
				public void run() {

					// LayoutParams
					ViewGroup.LayoutParams rowLp = new LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT,
							LayoutParams.WRAP_CONTENT);

					int max = 0;
					for (int i = mStart; i < mEnd; i++) {
						if (i % mDefCandiDataHalfCnt == 0) {
							LinearLayout parentLinear = new LinearLayout(getContext());
							parentLinear.setOrientation(LinearLayout.HORIZONTAL);
							parentLinear.setLayoutParams(rowLp);
							mCandidateDataListView.addView(parentLinear);

							max = (i + mDefCandiDataHalfCnt < mString.length) ? (i + mDefCandiDataHalfCnt)
									: (mString.length);

							addCandidateTextView(mInflater, parentLinear, mString, i, max, false);

							// # Line
							View underLine = new View(mContext);
							underLine.setMinimumHeight(getResources().getDimensionPixelOffset(R.dimen.candidate_under_line_height));
							underLine.setBackgroundColor(getResources().getColor(R.color.candidate_popup_under_line));
							mCandidateDataListView.addView(underLine);
						}
					}
				}
			});
		}

		Handler mHandler = new Handler();
	}

	View.OnClickListener onClick = new View.OnClickListener() {

		@SuppressWarnings("deprecation")
		@Override
		public void onClick(View v) {
			String text = ((TextView) v).getText().toString();
			int index = (Integer) v.getTag();
			mOnBtnListener.onBtnText(text, index);
			mCandidateShowBtn.setImageResource(R.drawable.icon_list_arrow_down);
		}
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/**
	 * @call Keyboard LanguageMode
	 * @param mode
	 */
	public void setMode(int mode) {
		mLanguageMode = mode;
	}

	public boolean isBackspace() {
		return true;
	}

	public boolean isShow() {
		return false;
	}

	public void hideView() {
		removeCandidateDataView();
		mOnBtnListener.onCandidateShow(false);
	}

	onBtnNotificationListener mOnBtnListener = null;

	public interface onBtnNotificationListener {
		public void onBtnCandidateListShow(boolean isShow);

		public void onBtnText(String text, int index);

		public void onCandidateShow(boolean isShow);
	}

	public void setOnBtnNotificationListener(onBtnNotificationListener l) {
		mOnBtnListener = l;
	}

	public void backSpace_Pinyin(LayoutInflater inflater, String[] hanja) {
		if (hanja != null) {
			addCandidateData(inflater, hanja);
		} else {
			hideView();
		}
	}
}
