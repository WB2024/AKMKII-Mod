package com.iriver.ak.popup;

import com.iriver.ak.softkeyboard.R;

import android.content.Context;
import android.graphics.drawable.ColorDrawable;
import android.inputmethodservice.Keyboard.Key;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.MeasureSpec;
import android.view.View.OnClickListener;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.TextView;

public class PopupKeyboardEn extends PopupWindow{

	private View mView = null;
	private Context mContext = null;
	
	private Key mKey = null;
	private ImageButton mCloseBtn = null;
	
	private LayoutInflater mLayoutInflater;
	private LinearLayout mCandidateTop = null;
	private LinearLayout mCandidateBottom = null;
	private View mCenterLine = null; // Add, 170525 yunsuk [Line Visible]
	
	private final int LINE_TOP_CNT = 6;
	private final int LINE_BOTTOM_CNT = 7;
	
	public PopupKeyboardEn(Context context) {
		super(context);
		mContext = context;

		mLayoutInflater = (LayoutInflater) context.getSystemService(
				Context.LAYOUT_INFLATER_SERVICE);
		mView = mLayoutInflater.inflate(R.layout.popupkey_en, null);
		
		mCandidateTop = (LinearLayout)mView.findViewById(R.id.popupkeyboard_top);
		mCandidateBottom = (LinearLayout)mView.findViewById(R.id.popupkeyboard_bottom);
		
		mCloseBtn = (ImageButton)mView.findViewById(R.id.popupkeyboard_close);
		mCloseBtn.setOnClickListener(new View.OnClickListener() {
			
			@Override
			public void onClick(View v) {
				dismiss();
			}
		});
		
		mCenterLine = mView.findViewById(R.id.popupkeyboard_centerline);
		
		setOutsideTouchable(true);
		
		setContentView(mView);
		
		setBackgroundDrawable(new ColorDrawable(android.R.color.transparent));
	}
	
	public void init(Context context, Key key, int width, int height, boolean isShift){
		mKey = key;		
	
		String popupChar = "";
		popupChar += key.popupCharacters.toString();

		if(isShift){
			popupChar=popupChar.toUpperCase();
		}
		
		createPopupText(context, popupChar);

		mView.measure(
				MeasureSpec.makeMeasureSpec(width, MeasureSpec.AT_MOST),
				MeasureSpec.makeMeasureSpec(height, MeasureSpec.AT_MOST));
		setWidth(width);
		setHeight(mView.getMeasuredHeight());
	}
	
	public void createPopupText(Context context, String popupString) {
		int charLength = popupString.length();
		
		if(mCandidateTop.getChildCount() > 0) {
			mCandidateTop.removeAllViews();
		}
		if(mCandidateBottom.getChildCount() > 0) {
			mCandidateBottom.removeAllViews();
		}
		
		boolean isTowLine = charLength > LINE_TOP_CNT;
		mCandidateBottom.setVisibility(isTowLine ? View.VISIBLE : View.GONE);
		mCenterLine.setVisibility(isTowLine ? View.VISIBLE : View.GONE);
		
		float textWidth = context.getResources().getDimension(R.dimen.en_popup_text_width);
		float textHeight = context.getResources().getDimension(R.dimen.en_popup_height);
		
		LinearLayout.LayoutParams textLp = new LinearLayout.LayoutParams((int) textWidth, (int) textHeight);

		for (int i = 0; i < charLength; i++) {
			char c = popupString.charAt(i);

			TextView tv = (TextView) mLayoutInflater.inflate(R.layout.candidate_textview, null);
			tv.setText(String.valueOf(c));
			tv.setTag(i);
			tv.setLayoutParams(textLp);
			tv.setOnClickListener(onClick);

			if (i < LINE_TOP_CNT) {
				if (i == LINE_TOP_CNT - 1) {
					tv.setCompoundDrawables(null, null, null, null);
				}
	
				mCandidateTop.addView(tv);
			} else {
				if (i == charLength - 1) {
					tv.setCompoundDrawables(null, null, null, null);
				}

				mCandidateBottom.addView(tv);
			}
		}
	}
	
	View.OnClickListener onClick = new OnClickListener() {
		
		@Override
		public void onClick(View view) {
			// TODO Auto-generated method stub
			String txt = ((TextView)view).getText().toString();
			mPopupKeyboardListener.onText(txt);
			dismiss();
		}
	};
	
	// //////////////////////////////////////////////////////////////////////////////////////
	// Listener
	// //////////////////////////////////////////////////////////////////////////////////////
	private popupKeyboardListener mPopupKeyboardListener;
	
	public interface popupKeyboardListener{
		public void onText(String txt);
	}
	public void setOnPopupKeyboardListener(popupKeyboardListener listener){
		mPopupKeyboardListener = listener;
	}
}
