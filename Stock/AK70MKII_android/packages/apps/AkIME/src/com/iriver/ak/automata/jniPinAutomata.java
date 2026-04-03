package com.iriver.ak.automata;

public class jniPinAutomata {
	static jniPinAutomata thisClass;

	public jniPinAutomata() {
		thisClass = this;
	}

	static public String[] makePinyin(int input, String jBuf1) {
		return thisClass.jniPinyinAutomata(input, jBuf1);
	}

	public native String[] jniPinyinAutomata(int input, String jBuf1);

	static {
		System.loadLibrary("PinyinAutomata");
	}
}