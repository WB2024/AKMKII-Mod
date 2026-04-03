package com.iriver.ak.lang.manager;

import java.util.ArrayList;

import com.iriver.ak.automata.jniPinAutomata;

import android.content.Context;

public class PinyinManager {

	private jniPinAutomata mJniPinyinAutomata = null;
	private PinyinTextClass mPinyinTextClass = null;
	
	public PinyinManager(Context context) {
		// TODO Auto-generated constructor stub
		mJniPinyinAutomata = new jniPinAutomata();
		mPinyinTextClass = new PinyinTextClass();
	}
	
	public boolean makePinyin(StringBuilder composing) {

		String engPinyin = composing.toString();
		
		if(engPinyin.length() > 0){

			String[] aa = jniPinAutomata.makePinyin(0, engPinyin);

			mPinyinTextClass.addText(composing);

			if(aa[1].length() > 0){
				mPinyinTextClass.commitText(engPinyin, aa[1], aa[2]);
				return true;
			}else{
				mPinyinTextClass.commitText(engPinyin, "", "");
			}
		}
		return false;
	}


	public String getCuEn(){
		return mPinyinTextClass.getCuEn();
	}
	
	public String getEditString(){
		return mPinyinTextClass.getEng();
	}
	
	public void reset(){
		mPinyinTextClass.reset();
	}
	
	
	public boolean backspace(StringBuilder composing){

		mPinyinTextClass.addText(composing);
		
		return makePinyin(composing);
	}
	
	public String[] getGanData(){
		int len = mPinyinTextClass.getGan().length();
		String[] abc = new String[len];
		for(int i=0;i<getGan().length();i++){
			abc[i] = String.valueOf(getGan().charAt(i));
		}
		return abc;
	}
	
	public String getGan(){
		return mPinyinTextClass.getGan();
	}
	
	public String[] getBunData(){
		int len = mPinyinTextClass.getBun().length();
		String[] abc = new String[len];
		for(int i=0;i<getBun().length();i++){
			abc[i] = String.valueOf(getBun().charAt(i));
		}
		return abc;
	}
	
	public String getBun(){
		return mPinyinTextClass.getBun();
	}

	/**
	 * @class JpnTextClass
	 * @author yunsuk
	 *
	 */
	class PinyinTextClass{
		private ArrayList<String> mEn= new ArrayList<String>();
		private String mBun = "";
		private String mGan = "";
		private StringBuilder mComposing = null;
		
		public PinyinTextClass(){
		}
	
		public void addText(StringBuilder composing){
			mComposing = composing;
		}

		public void commitText(String en){
			mGan = "";
			mBun = "";
			mEn.add(en);
		}
		public void commitText(String en, String gan, String bun){
			mGan = gan;
			mBun = bun;
			mEn.add(en);
		}
	
		public void reset(){
			if(mEn != null)
				mEn.clear();
			mBun = "";
			mGan = "";
		}
		
		public String getCuEn(){
			return mComposing.toString();
		}
		
		public String getEng(){
			String temp = "";
			for(int i=0;i<mEn.size();i++)
				temp += mEn.get(i);
			return temp;
		}
		
		public String getBun(){
			return mBun;
		}
		public String getGan(){
			return mGan;
		}
	}
}
