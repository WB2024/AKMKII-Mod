package com.iriver.ak.lang.map;

import java.util.HashMap;


public class LanguageMap {
		
	public static final String[] JP_HIRAGANA_SYMBOL_CYCLE_TABLE = {
		"。", "?", "!", "、"
	};
	
	// Add, 180227 yunsuk
	public static boolean isHiraganaSymbol(String word) {
		for(String s : LanguageMap.JP_HIRAGANA_SYMBOL_CYCLE_TABLE) {
			if(word.equals(s)) {
				return true;
			}
		}
		return false;
	}
	
	public static String getHiraganaSymbol(int index){
		return JP_HIRAGANA_SYMBOL_CYCLE_TABLE[index];
	}
	
	// Add, 180227 yunsuk
	public static boolean isHiraganaCycle(String beforeWord, String afterWord) {
		boolean isCycle = false;
		for (int i = 0; i < JP_FULL_HIRAGANA_CYCLE_TABLE.length; i++) {
			int cntSame = 0;
			for (int j = 0; j < JP_FULL_HIRAGANA_CYCLE_TABLE[i].length; j++) {
				if(JP_FULL_HIRAGANA_CYCLE_TABLE[i][j].equals(beforeWord)) {
					cntSame++;
				}
				if(JP_FULL_HIRAGANA_CYCLE_TABLE[i][j].equals(afterWord)) {
					cntSame++;
				}
				
				if(cntSame == 2) {
					isCycle = true;
					break;
				}
			}
		}
		return isCycle;
	}
	
	
	/** Toggle cycle table for full-width HIRAGANA */
    private static final String[][] JP_FULL_HIRAGANA_CYCLE_TABLE = {
        {"\u3042", "\u3044", "\u3046", "\u3048", "\u304a", "\u3041", "\u3043", "\u3045", "\u3047", "\u3049"},
        {"\u304b", "\u304d", "\u304f", "\u3051", "\u3053"},
        {"\u3055", "\u3057", "\u3059", "\u305b", "\u305d"},
        {"\u305f", "\u3061", "\u3064", "\u3066", "\u3068", "\u3063"},
        {"\u306a", "\u306b", "\u306c", "\u306d", "\u306e"},
        {"\u306f", "\u3072", "\u3075", "\u3078", "\u307b"},
        {"\u307e", "\u307f", "\u3080", "\u3081", "\u3082"},
        {"\u3084", "\u3086", "\u3088", "\u3083", "\u3085", "\u3087"},
        {"\u3089", "\u308a", "\u308b", "\u308c", "\u308d"},
        {"\u308f", "\u3092", "\u3093", "\u308e", "\u30fc"},
        {"。", "、", "!", "?"}
     //   JP_HIRAGANA_SYMBOL_CYCLE_TABLE
    };
    
    public static String getHiraganaCycleTableWord(String beforeWord){
    	for(int i=0;i<JP_FULL_HIRAGANA_CYCLE_TABLE.length;i++){
    		for(int j=0;j<JP_FULL_HIRAGANA_CYCLE_TABLE[i].length;j++){
    			if(JP_FULL_HIRAGANA_CYCLE_TABLE[i][j].equals(beforeWord)){
    				if(j == JP_FULL_HIRAGANA_CYCLE_TABLE[i].length - 1){
    					return JP_FULL_HIRAGANA_CYCLE_TABLE[i][0];
    				}else{
    					return JP_FULL_HIRAGANA_CYCLE_TABLE[i][j+1];
    				}
    			}
    		}
    	}
    	return "";
    }
    
	private static final HashMap<Integer, String> HIRAGANA_TABLE = new HashMap<Integer, String>(){{
		//"あ", "い", "う", "え", "お",
		put(0, "\u3042");  put(1, "\u3044");  put(2, "\u3046");  put(3, "\u3048");  put(4, "\u304A");
		//"か", "き", "く", "け", "こ",
		put(5, "\u304B");  put(6, "\u304D");  put(7, "\u304F");  put(8, "\u3051");  put(9, "\u3053");
		//"さ", "し", "す", "せ", "そ"
		put(10, "\u3055");  put(11, "\u3057");  put(12, "\u3059");  put(13, "\u305B");  put(14, "\u305D");
		//"た", "ち", "つ", "て", "と"
		put(15, "\u305F");  put(16, "\u3061");  put(17, "\u3064");  put(18, "\u3066");  put(19, "\u3068");
		//"な", "に", "ぬ", "ね", "の",
		put(20, "\u306A");  put(21, "\u306B");  put(22, "\u306C");  put(23, "\u306D");  put(24, "\u306E");
		//"は", "ひ", "ふ", "へ", "ほ"
		put(25, "\u306F");  put(26, "\u3072");  put(27, "\u3075");  put(28, "\u3078");  put(29, "\u307B");
		//"ま", "み", "む", "め", "も"
		put(30, "\u307E");  put(31, "\u307F");  put(32, "\u3080");  put(33, "\u3081");  put(34, "\u3082");
		//"や", "ゆ", "よ"
		put(35, "\u3084");  put(36, "\u3086");  put(37, "\u3088");
		//"ら", "り", "る", "れ", "ろ"
		put(38, "\u3089");  put(39, "\u308A");  put(40, "\u308B");  put(41, "\u308C");  put(42, "\u308D");
		//"わ", "を", "ん"
		put(43, "\u308F");  put(44, "\u3092");  put(45, "\u3093");	put(46, "ー");
	}};
	
	public static String getSimeji(int code){
		return HIRAGANA_TABLE.get(code);
	}
	
    /** Replace table for full-width HIRAGANA */
	private static final HashMap<String,String> HiraTak = new HashMap<String,String>() {{
		put( "\u3042", "\u3041");	put( "\u3044", "\u3043");	// あ, い
		put( "\u3046", "\u3045");	put( "\u3048", "\u3047");	// う, え
		put( "\u304A", "\u3049");	put( "\u304B", "\u304C");	// お, か
		put( "\u304D", "\u304E");	put( "\u304F", "\u3050");	// き, く 
		put( "\u3051", "\u3052");	put( "\u3053", "\u3054");	// け, こ
		put( "\u3055", "\u3056");	put( "\u3057", "\u3058");	// さ, し
		put( "\u3059", "\u305A");	put( "\u305B", "\u305C");	// す, せ
		put( "\u305D", "\u305E");	put( "\u305F", "\u3060");	// そ, た
		put( "\u3061", "\u3062");	put( "\u3064", "\u3063");	// ち, つ
		put( "\u3066", "\u3067");	put( "\u3068", "\u3069");	// て, と
		put( "\u306A", "\u306A");	put( "\u306B", "\u306B");	// な, に
		put( "\u306C", "\u306C");	put( "\u306D", "\u306D");	// ぬ, ね
		put( "\u306E", "\u306E");	put( "\u306F", "\u3070");	put( "\u3070", "\u3071");	// の, は, ば
		put( "\u3072", "\u3073");	put( "\u3073", "\u3074");	// ひ, び
		put( "\u3075", "\u3076");	put( "\u3076", "\u3077");	// ふ, ぶ
		put( "\u0000", "\u0000");	put( "\u0000", "\u0000");	// へ, べ
		put( "\u0000", "\u0000");	put( "\u0000", "\u0000");
		put( "\u0000", "\u0000");	put( "\u0000", "\u0000");
		put( "\u0000", "\u0000");	put( "\u0000", "\u0000");
		put( "\u0000", "\u0000");	put( "\u0000", "\u0000");
		put( "\u0000", "\u0000");	put( "\u0000", "\u0000");
		put( "\u0000", "\u0000");	put( "\u0000", "\u0000");
		put( "\u0000", "\u0000");	put( "\u0000", "\u0000");

	}};


	private static final HashMap<String, String> JP_FULL_HIRAGANA_REPLACE_TABLE = new HashMap<String, String>() {{
		put("\u3042", "\u3041"); put("\u3044", "\u3043"); put("\u3046", "\u3045"); put("\u3048", "\u3047"); put("\u304a", "\u3049");
		put("\u3041", "\u3042"); put("\u3043", "\u3044"); put("\u3045", "\u30f4"); put("\u3047", "\u3048"); put("\u3049", "\u304a");
		put("\u304b", "\u304c"); put("\u304d", "\u304e"); put("\u304f", "\u3050"); put("\u3051", "\u3052"); put("\u3053", "\u3054");
		put("\u304c", "\u304b"); put("\u304e", "\u304d"); put("\u3050", "\u304f"); put("\u3052", "\u3051"); put("\u3054", "\u3053");
		put("\u3055", "\u3056"); put("\u3057", "\u3058"); put("\u3059", "\u305a"); put("\u305b", "\u305c"); put("\u305d", "\u305e");
		put("\u3056", "\u3055"); put("\u3058", "\u3057"); put("\u305a", "\u3059"); put("\u305c", "\u305b"); put("\u305e", "\u305d");
		put("\u305f", "\u3060"); put("\u3061", "\u3062"); put("\u3064", "\u3063"); put("\u3066", "\u3067"); put("\u3068", "\u3069");
		put("\u3060", "\u305f"); put("\u3062", "\u3061"); put("\u3063", "\u3065"); put("\u3067", "\u3066"); put("\u3069", "\u3068");
		put("\u3065", "\u3064"); put("\u30f4", "\u3046");
		put("\u306f", "\u3070"); put("\u3072", "\u3073"); put("\u3075", "\u3076"); put("\u3078", "\u3079"); put("\u307b", "\u307c");
		put("\u3070", "\u3071"); put("\u3073", "\u3074"); put("\u3076", "\u3077"); put("\u3079", "\u307a"); put("\u307c", "\u307d");
		put("\u3071", "\u306f"); put("\u3074", "\u3072"); put("\u3077", "\u3075"); put("\u307a", "\u3078"); put("\u307d", "\u307b");
		put("\u3084", "\u3083"); put("\u3086", "\u3085"); put("\u3088", "\u3087");
		put("\u3083", "\u3084"); put("\u3085", "\u3086"); put("\u3087", "\u3088");
		put("\u308f", "\u308e");
		put("\u308e", "\u308f");
		put("\u309b", "\u309c");
		put("\u309c", "\u309b");
	}};

	public static String getTak(String name){
		return JP_FULL_HIRAGANA_REPLACE_TABLE.get(name);
	}
    
    
    /** Toggle cycle table for full-width KATAKANA */
    private static final String[][] JP_FULL_KATAKANA_CYCLE_TABLE = {
        {"\u30a2", "\u30a4", "\u30a6", "\u30a8", "\u30aa", "\u30a1", "\u30a3",
         "\u30a5", "\u30a7", "\u30a9"},
        {"\u30ab", "\u30ad", "\u30af", "\u30b1", "\u30b3"},
        {"\u30b5", "\u30b7", "\u30b9", "\u30bb", "\u30bd"},
        {"\u30bf", "\u30c1", "\u30c4", "\u30c6", "\u30c8", "\u30c3"},
        {"\u30ca", "\u30cb", "\u30cc", "\u30cd", "\u30ce"},
        {"\u30cf", "\u30d2", "\u30d5", "\u30d8", "\u30db"},
        {"\u30de", "\u30df", "\u30e0", "\u30e1", "\u30e2"},
        {"\u30e4", "\u30e6", "\u30e8", "\u30e3", "\u30e5", "\u30e7"},
        {"\u30e9", "\u30ea", "\u30eb", "\u30ec", "\u30ed"},
        {"\u30ef", "\u30f2", "\u30f3", "\u30ee", "\u30fc"},
        {"\u3001", "\u3002", "\uff1f", "\uff01", "\u30fb", "\u3000"}
    };

    /** Replace table for full-width KATAKANA */
    private static final HashMap<String,String> JP_FULL_KATAKANA_REPLACE_TABLE = new HashMap<String,String>() {{
        put("\u30a2", "\u30a1"); put("\u30a4", "\u30a3"); put("\u30a6", "\u30a5"); put("\u30a8", "\u30a7"); put("\u30aa", "\u30a9");
        put("\u30a1", "\u30a2"); put("\u30a3", "\u30a4"); put("\u30a5", "\u30f4"); put("\u30a7", "\u30a8"); put("\u30a9", "\u30aa");
        put("\u30ab", "\u30ac"); put("\u30ad", "\u30ae"); put("\u30af", "\u30b0"); put("\u30b1", "\u30b2"); put("\u30b3", "\u30b4");
        put("\u30ac", "\u30ab"); put("\u30ae", "\u30ad"); put("\u30b0", "\u30af"); put("\u30b2", "\u30b1"); put("\u30b4", "\u30b3");
        put("\u30b5", "\u30b6"); put("\u30b7", "\u30b8"); put("\u30b9", "\u30ba"); put("\u30bb", "\u30bc"); put("\u30bd", "\u30be");
        put("\u30b6", "\u30b5"); put("\u30b8", "\u30b7"); put("\u30ba", "\u30b9"); put("\u30bc", "\u30bb"); put("\u30be", "\u30bd");
        put("\u30bf", "\u30c0"); put("\u30c1", "\u30c2"); put("\u30c4", "\u30c3"); put("\u30c6", "\u30c7"); put("\u30c8", "\u30c9");
        put("\u30c0", "\u30bf"); put("\u30c2", "\u30c1"); put("\u30c3", "\u30c5"); put("\u30c7", "\u30c6"); put("\u30c9", "\u30c8");
        put("\u30c5", "\u30c4"); put("\u30f4", "\u30a6");
        put("\u30cf", "\u30d0"); put("\u30d2", "\u30d3"); put("\u30d5", "\u30d6"); put("\u30d8", "\u30d9"); put("\u30db", "\u30dc");
        put("\u30d0", "\u30d1"); put("\u30d3", "\u30d4"); put("\u30d6", "\u30d7"); put("\u30d9", "\u30da"); put("\u30dc", "\u30dd");
        put("\u30d1", "\u30cf"); put("\u30d4", "\u30d2"); put("\u30d7", "\u30d5"); put("\u30da", "\u30d8"); put("\u30dd", "\u30db");
        put("\u30e4", "\u30e3"); put("\u30e6", "\u30e5"); put("\u30e8", "\u30e7");
        put("\u30e3", "\u30e4"); put("\u30e5", "\u30e6"); put("\u30e7", "\u30e8");
        put("\u30ef", "\u30ee");
        put("\u30ee", "\u30ef");
    }};
    

	/** HashMap for Romaji-to-Kana conversion (Japanese mode) */
	private static final HashMap<String, String> RomkanToHiraTable = new HashMap<String, String>() {{
		put("la", "\u3041");        put("xa", "\u3041");        put("a", "\u3042");
		put("li", "\u3043");        put("lyi", "\u3043");       put("xi", "\u3043");
		put("xyi", "\u3043");       put("i", "\u3044");         put("yi", "\u3044");
		put("ye", "\u3044\u3047");      put("lu", "\u3045");        put("xu", "\u3045");
		put("u", "\u3046");         put("whu", "\u3046");       put("wu", "\u3046");
		put("wha", "\u3046\u3041");     put("whi", "\u3046\u3043");     put("wi", "\u3046\u3043");
		put("we", "\u3046\u3047");      put("whe", "\u3046\u3047");     put("who", "\u3046\u3049");
		put("le", "\u3047");        put("lye", "\u3047");       put("xe", "\u3047");
		put("xye", "\u3047");       put("e", "\u3048");         put("lo", "\u3049");
		put("xo", "\u3049");        put("o", "\u304a");         put("ca", "\u304b");
		put("ka", "\u304b");        put("ga", "\u304c");        put("ki", "\u304d");
		put("kyi", "\u304d\u3043");     put("kye", "\u304d\u3047");     put("kya", "\u304d\u3083");
		put("kyu", "\u304d\u3085");     put("kyo", "\u304d\u3087");     put("gi", "\u304e");
		put("gyi", "\u304e\u3043");     put("gye", "\u304e\u3047");     put("gya", "\u304e\u3083");
		put("gyu", "\u304e\u3085");     put("gyo", "\u304e\u3087");     put("cu", "\u304f");
		put("ku", "\u304f");        put("qu", "\u304f");        put("kwa", "\u304f\u3041");
		put("qa", "\u304f\u3041");      put("qwa", "\u304f\u3041");     put("qi", "\u304f\u3043");
		put("qwi", "\u304f\u3043");     put("qyi", "\u304f\u3043");     put("qwu", "\u304f\u3045");
		put("qe", "\u304f\u3047");      put("qwe", "\u304f\u3047");     put("qye", "\u304f\u3047");
		put("qo", "\u304f\u3049");      put("qwo", "\u304f\u3049");     put("qya", "\u304f\u3083");
		put("qyu", "\u304f\u3085");     put("qyo", "\u304f\u3087");     put("gu", "\u3050");
		put("gwa", "\u3050\u3041");     put("gwi", "\u3050\u3043");     put("gwu", "\u3050\u3045");
		put("gwe", "\u3050\u3047");     put("gwo", "\u3050\u3049");     put("ke", "\u3051");
		put("ge", "\u3052");        put("co", "\u3053");        put("ko", "\u3053");
		put("go", "\u3054");        put("sa", "\u3055");        put("za", "\u3056");
		put("ci", "\u3057");        put("shi", "\u3057");       put("si", "\u3057");
		put("syi", "\u3057\u3043");     put("she", "\u3057\u3047");     put("sye", "\u3057\u3047");
		put("sha", "\u3057\u3083");     put("sya", "\u3057\u3083");     put("shu", "\u3057\u3085");
		put("syu", "\u3057\u3085");     put("sho", "\u3057\u3087");     put("syo", "\u3057\u3087");
		put("ji", "\u3058");        put("zi", "\u3058");        put("jyi", "\u3058\u3043");
		put("zyi", "\u3058\u3043");     put("je", "\u3058\u3047");      put("jye", "\u3058\u3047");
		put("zye", "\u3058\u3047");     put("ja", "\u3058\u3083");      put("jya", "\u3058\u3083");
		put("zya", "\u3058\u3083");     put("ju", "\u3058\u3085");      put("jyu", "\u3058\u3085");
		put("zyu", "\u3058\u3085");     put("jo", "\u3058\u3087");      put("jyo", "\u3058\u3087");
		put("zyo", "\u3058\u3087");     put("su", "\u3059");        put("swa", "\u3059\u3041");
		put("swi", "\u3059\u3043");     put("swu", "\u3059\u3045");     put("swe", "\u3059\u3047");
		put("swo", "\u3059\u3049");     put("zu", "\u305a");        put("ce", "\u305b");
		put("se", "\u305b");        put("ze", "\u305c");        put("so", "\u305d");
		put("zo", "\u305e");        put("ta", "\u305f");        put("da", "\u3060");
		put("chi", "\u3061");       put("ti", "\u3061");        put("cyi", "\u3061\u3043");
		put("tyi", "\u3061\u3043");     put("che", "\u3061\u3047");     put("cye", "\u3061\u3047");
		put("tye", "\u3061\u3047");     put("cha", "\u3061\u3083");     put("cya", "\u3061\u3083");
		put("tya", "\u3061\u3083");     put("chu", "\u3061\u3085");     put("cyu", "\u3061\u3085");
		put("tyu", "\u3061\u3085");     put("cho", "\u3061\u3087");     put("cyo", "\u3061\u3087");
		put("tyo", "\u3061\u3087");     put("di", "\u3062");        put("dyi", "\u3062\u3043");
		put("dye", "\u3062\u3047");     put("dya", "\u3062\u3083");     put("dyu", "\u3062\u3085");
		put("dyo", "\u3062\u3087");     put("ltsu", "\u3063");      put("ltu", "\u3063");
		put("xtu", "\u3063");       put("", "\u3063");          put("tsu", "\u3064");
		put("tu", "\u3064");        put("tsa", "\u3064\u3041");     put("tsi", "\u3064\u3043");
		put("tse", "\u3064\u3047");     put("tso", "\u3064\u3049");     put("du", "\u3065");
		put("te", "\u3066");        put("thi", "\u3066\u3043");     put("the", "\u3066\u3047");
		put("tha", "\u3066\u3083");     put("thu", "\u3066\u3085");     put("tho", "\u3066\u3087");
		put("de", "\u3067");        put("dhi", "\u3067\u3043");     put("dhe", "\u3067\u3047");
		put("dha", "\u3067\u3083");     put("dhu", "\u3067\u3085");     put("dho", "\u3067\u3087");
		put("to", "\u3068");        put("twa", "\u3068\u3041");     put("twi", "\u3068\u3043");
		put("twu", "\u3068\u3045");     put("twe", "\u3068\u3047");     put("two", "\u3068\u3049");
		put("do", "\u3069");        put("dwa", "\u3069\u3041");     put("dwi", "\u3069\u3043");
		put("dwu", "\u3069\u3045");     put("dwe", "\u3069\u3047");     put("dwo", "\u3069\u3049");
		put("na", "\u306a");        put("ni", "\u306b");        put("nyi", "\u306b\u3043");
		put("nye", "\u306b\u3047");     put("nya", "\u306b\u3083");     put("nyu", "\u306b\u3085");
		put("nyo", "\u306b\u3087");     put("nu", "\u306c");        put("ne", "\u306d");
		put("no", "\u306e");        put("ha", "\u306f");        put("ba", "\u3070");
		put("pa", "\u3071");        put("hi", "\u3072");        put("hyi", "\u3072\u3043");
		put("hye", "\u3072\u3047");     put("hya", "\u3072\u3083");     put("hyu", "\u3072\u3085");
		put("hyo", "\u3072\u3087");     put("bi", "\u3073");        put("byi", "\u3073\u3043");
		put("bye", "\u3073\u3047");     put("bya", "\u3073\u3083");     put("byu", "\u3073\u3085");
		put("byo", "\u3073\u3087");     put("pi", "\u3074");        put("pyi", "\u3074\u3043");
		put("pye", "\u3074\u3047");     put("pya", "\u3074\u3083");     put("pyu", "\u3074\u3085");
		put("pyo", "\u3074\u3087");     put("fu", "\u3075");        put("hu", "\u3075");
		put("fa", "\u3075\u3041");      put("fwa", "\u3075\u3041");     put("fi", "\u3075\u3043");
		put("fwi", "\u3075\u3043");     put("fyi", "\u3075\u3043");     put("fwu", "\u3075\u3045");
		put("fe", "\u3075\u3047");      put("fwe", "\u3075\u3047");     put("fye", "\u3075\u3047");
		put("fo", "\u3075\u3049");      put("fwo", "\u3075\u3049");     put("fya", "\u3075\u3083");
		put("fyu", "\u3075\u3085");     put("fyo", "\u3075\u3087");     put("bu", "\u3076");
		put("pu", "\u3077");        put("he", "\u3078");        put("be", "\u3079");
		put("pe", "\u307a");        put("ho", "\u307b");        put("bo", "\u307c");
		put("po", "\u307d");        put("ma", "\u307e");        put("mi", "\u307f");
		put("myi", "\u307f\u3043");     put("mye", "\u307f\u3047");     put("mya", "\u307f\u3083");
		put("myu", "\u307f\u3085");     put("myo", "\u307f\u3087");     put("mu", "\u3080");
		put("me", "\u3081");        put("mo", "\u3082");        put("lya", "\u3083");
		put("xya", "\u3083");       put("ya", "\u3084");        put("lyu", "\u3085");
		put("xyu", "\u3085");       put("yu", "\u3086");        put("lyo", "\u3087");
		put("xyo", "\u3087");       put("yo", "\u3088");        put("ra", "\u3089");
		put("ri", "\u308a");        put("ryi", "\u308a\u3043");     put("rye", "\u308a\u3047");
		put("rya", "\u308a\u3083");     put("ryu", "\u308a\u3085");     put("ryo", "\u308a\u3087");
		put("ru", "\u308b");        put("re", "\u308c");        put("ro", "\u308d");
		put("lwa", "\u308e");       put("xwa", "\u308e");       put("wa", "\u308f");
		put("wo", "\u3092");        put("nn", "\u3093");        put("xn", "\u3093");
		put("vu", "\u30f4");        put("va", "\u30f4\u3041");      put("vi", "\u30f4\u3043");
		put("vyi", "\u30f4\u3043");     put("ve", "\u30f4\u3047");      put("vye", "\u30f4\u3047");
		put("vo", "\u30f4\u3049");      put("vya", "\u30f4\u3083");     put("vyu", "\u30f4\u3085");
		put("vyo", "\u30f4\u3087");     
		put("bb", "\u3063b");   put("cc", "\u3063c");   put("dd", "\u3063d");
		put("ff", "\u3063f");   put("gg", "\u3063g");   put("hh", "\u3063h");
		put("jj", "\u3063j");   put("kk", "\u3063k");   put("ll", "\u3063l");
		put("mm", "\u3063m");   put("pp", "\u3063p");   put("qq", "\u3063q");
		put("rr", "\u3063r");   put("ss", "\u3063s");   put("tt", "\u3063t");
		put("vv", "\u3063v");   put("ww", "\u3063w");   put("xx", "\u3063x");
		put("yy", "\u3063y");   put("zz", "\u3063z");   put("nb", "\u3093b");
		put("nc", "\u3093c");   put("nd", "\u3093d");   put("nf", "\u3093f");
		put("ng", "\u3093g");   put("nh", "\u3093h");   put("nj", "\u3093j");
		put("nk", "\u3093k");   put("nm", "\u3093m");   put("np", "\u3093p");
		put("nq", "\u3093q");   put("nr", "\u3093r");   put("ns", "\u3093s");
		put("nt", "\u3093t");   put("nv", "\u3093v");   put("nw", "\u3093w");
		put("nx", "\u3093x");   put("nz", "\u3093z");   put("nl", "\u3093l");
		put("-", "\u30fc"); put(".", "\u3002"); put(",", "\u3001"); put("?", "\uff1f"); put("/", "\u30fb");
		put("@", "\uff20"); put("#", "\uff03"); put("%", "\uff05"); put("&", "\uff06"); put("*", "\uff0a");
		put("+", "\uff0b"); put("=", "\uff1d"); put("(", "\uff08"); put(")", "\uff09");
		put("~", "\uff5e"); put("\"", "\uff02"); put("'", "\uff07"); put(":", "\uff1a"); put(";", "\uff1b");
		put("!", "\uff01"); put("^", "\uff3e"); put("\u00a5", "\uffe5"); put("$", "\uff04"); put("[", "\u300c");
		put("]", "\u300d"); put("_", "\uff3f"); put("{", "\uff5b"); put("}", "\uff5d");
		put("`", "\uff40"); put("<", "\uff1c"); put(">", "\uff1e"); put("\\", "\uff3c"); put("|", "\uff5c");
		put("1", "\uff11"); put("2", "\uff12"); put("3", "\uff13"); put("4", "\uff14"); put("5", "\uff15");
		put("6", "\uff16"); put("7", "\uff17"); put("8", "\uff18"); put("9", "\uff19"); put("0", "\uff10");
	}};

	public static String[] convertHiragana(String en){
		return convert(en, RomkanToHiraTable);
	}


	 /** HashMap for Romaji-to-Kana conversion (Japanese mode) */
    private static final HashMap<String, String> RomkanToKanaTable = new HashMap<String, String>() {{
        put("la", "\u30a1");        put("xa", "\u30a1");        put("a", "\u30a2");
        put("li", "\u30a3");        put("lyi", "\u30a3");       put("xi", "\u30a3");
        put("xyi", "\u30a3");       put("i", "\u30a4");         put("yi", "\u30a4");
        put("ye", "\u30a4\u30a7");      put("lu", "\u30a5");        put("xu", "\u30a5");
        put("u", "\u30a6");         put("whu", "\u30a6");       put("wu", "\u30a6");
        put("wha", "\u30a6\u30a1");     put("whi", "\u30a6\u30a3");     put("wi", "\u30a6\u30a3");
        put("we", "\u30a6\u30a7");      put("whe", "\u30a6\u30a7");     put("who", "\u30a6\u30a9");
        put("le", "\u30a7");        put("lye", "\u30a7");       put("xe", "\u30a7");
        put("xye", "\u30a7");       put("e", "\u30a8");         put("lo", "\u30a9");
        put("xo", "\u30a9");        put("o", "\u30aa");         put("ca", "\u30ab");
        put("lka", "\u30f5");       put("xka", "\u30f5");
        put("ka", "\u30ab");        put("ga", "\u30ac");        put("ki", "\u30ad");
        put("kyi", "\u30ad\u30a3");     put("kye", "\u30ad\u30a7");     put("kya", "\u30ad\u30e3");
        put("kyu", "\u30ad\u30e5");     put("kyo", "\u30ad\u30e7");     put("gi", "\u30ae");
        put("gyi", "\u30ae\u30a3");     put("gye", "\u30ae\u30a7");     put("gya", "\u30ae\u30e3");
        put("gyu", "\u30ae\u30e5");     put("gyo", "\u30ae\u30e7");     put("cu", "\u30af");
        put("ku", "\u30af");        put("qu", "\u30af");        put("kwa", "\u30af\u30a1");
        put("qa", "\u30af\u30a1");      put("qwa", "\u30af\u30a1");     put("qi", "\u30af\u30a3");
        put("qwi", "\u30af\u30a3");     put("qyi", "\u30af\u30a3");     put("qwu", "\u30af\u30a5");
        put("qe", "\u30af\u30a7");      put("qwe", "\u30af\u30a7");     put("qye", "\u30af\u30a7");
        put("qo", "\u30af\u30a9");      put("qwo", "\u30af\u30a9");     put("qya", "\u30af\u30e3");
        put("qyu", "\u30af\u30e5");     put("qyo", "\u30af\u30e7");     put("gu", "\u30b0");
        put("gwa", "\u30b0\u30a1");     put("gwi", "\u30b0\u30a3");     put("gwu", "\u30b0\u30a5");
        put("gwe", "\u30b0\u30a7");     put("gwo", "\u30b0\u30a9");
        put("lke", "\u30f6");       put("xke", "\u30f6");       put("ke", "\u30b1");
        put("ge", "\u30b2");        put("co", "\u30b3");        put("ko", "\u30b3");
        put("go", "\u30b4");        put("sa", "\u30b5");        put("za", "\u30b6");
        put("ci", "\u30b7");        put("shi", "\u30b7");       put("si", "\u30b7");
        put("syi", "\u30b7\u30a3");     put("she", "\u30b7\u30a7");     put("sye", "\u30b7\u30a7");
        put("sha", "\u30b7\u30e3");     put("sya", "\u30b7\u30e3");     put("shu", "\u30b7\u30e5");
        put("syu", "\u30b7\u30e5");     put("sho", "\u30b7\u30e7");     put("syo", "\u30b7\u30e7");
        put("ji", "\u30b8");        put("zi", "\u30b8");        put("jyi", "\u30b8\u30a3");
        put("zyi", "\u30b8\u30a3");     put("je", "\u30b8\u30a7");      put("jye", "\u30b8\u30a7");
        put("zye", "\u30b8\u30a7");     put("ja", "\u30b8\u30e3");      put("jya", "\u30b8\u30e3");
        put("zya", "\u30b8\u30e3");     put("ju", "\u30b8\u30e5");      put("jyu", "\u30b8\u30e5");
        put("zyu", "\u30b8\u30e5");     put("jo", "\u30b8\u30e7");      put("jyo", "\u30b8\u30e7");
        put("zyo", "\u30b8\u30e7");     put("su", "\u30b9");        put("swa", "\u30b9\u30a1");
        put("swi", "\u30b9\u30a3");     put("swu", "\u30b9\u30a5");     put("swe", "\u30b9\u30a7");
        put("swo", "\u30b9\u30a9");     put("zu", "\u30ba");        put("ce", "\u30bb");
        put("se", "\u30bb");        put("ze", "\u30bc");        put("so", "\u30bd");
        put("zo", "\u30be");        put("ta", "\u30bf");        put("da", "\u30c0");
        put("chi", "\u30c1");       put("ti", "\u30c1");        put("cyi", "\u30c1\u30a3");
        put("tyi", "\u30c1\u30a3");     put("che", "\u30c1\u30a7");     put("cye", "\u30c1\u30a7");
        put("tye", "\u30c1\u30a7");     put("cha", "\u30c1\u30e3");     put("cya", "\u30c1\u30e3");
        put("tya", "\u30c1\u30e3");     put("chu", "\u30c1\u30e5");     put("cyu", "\u30c1\u30e5");
        put("tyu", "\u30c1\u30e5");     put("cho", "\u30c1\u30e7");     put("cyo", "\u30c1\u30e7");
        put("tyo", "\u30c1\u30e7");     put("di", "\u30c2");        put("dyi", "\u30c2\u30a3");
        put("dye", "\u30c2\u30a7");     put("dya", "\u30c2\u30e3");     put("dyu", "\u30c2\u30e5");
        put("dyo", "\u30c2\u30e7");     put("ltsu", "\u30c3");      put("ltu", "\u30c3");
        put("xtu", "\u30c3");       put("", "\u30c3");          put("tsu", "\u30c4");
        put("tu", "\u30c4");        put("tsa", "\u30c4\u30a1");     put("tsi", "\u30c4\u30a3");
        put("tse", "\u30c4\u30a7");     put("tso", "\u30c4\u30a9");     put("du", "\u30c5");
        put("te", "\u30c6");        put("thi", "\u30c6\u30a3");     put("the", "\u30c6\u30a7");
        put("tha", "\u30c6\u30e3");     put("thu", "\u30c6\u30e5");     put("tho", "\u30c6\u30e7");
        put("de", "\u30c7");        put("dhi", "\u30c7\u30a3");     put("dhe", "\u30c7\u30a7");
        put("dha", "\u30c7\u30e3");     put("dhu", "\u30c7\u30e5");     put("dho", "\u30c7\u30e7");
        put("to", "\u30c8");        put("twa", "\u30c8\u30a1");     put("twi", "\u30c8\u30a3");
        put("twu", "\u30c8\u30a5");     put("twe", "\u30c8\u30a7");     put("two", "\u30c8\u30a9");
        put("do", "\u30c9");        put("dwa", "\u30c9\u30a1");     put("dwi", "\u30c9\u30a3");
        put("dwu", "\u30c9\u30a5");     put("dwe", "\u30c9\u30a7");     put("dwo", "\u30c9\u30a9");
        put("na", "\u30ca");        put("ni", "\u30cb");        put("nyi", "\u30cb\u30a3");
        put("nye", "\u30cb\u30a7");     put("nya", "\u30cb\u30e3");     put("nyu", "\u30cb\u30e5");
        put("nyo", "\u30cb\u30e7");     put("nu", "\u30cc");        put("ne", "\u30cd");
        put("no", "\u30ce");        put("ha", "\u30cf");        put("ba", "\u30d0");
        put("pa", "\u30d1");        put("hi", "\u30d2");        put("hyi", "\u30d2\u30a3");
        put("hye", "\u30d2\u30a7");     put("hya", "\u30d2\u30e3");     put("hyu", "\u30d2\u30e5");
        put("hyo", "\u30d2\u30e7");     put("bi", "\u30d3");        put("byi", "\u30d3\u30a3");
        put("bye", "\u30d3\u30a7");     put("bya", "\u30d3\u30e3");     put("byu", "\u30d3\u30e5");
        put("byo", "\u30d3\u30e7");     put("pi", "\u30d4");        put("pyi", "\u30d4\u30a3");
        put("pye", "\u30d4\u30a7");     put("pya", "\u30d4\u30e3");     put("pyu", "\u30d4\u30e5");
        put("pyo", "\u30d4\u30e7");     put("fu", "\u30d5");        put("hu", "\u30d5");
        put("fa", "\u30d5\u30a1");      put("fwa", "\u30d5\u30a1");     put("fi", "\u30d5\u30a3");
        put("fwi", "\u30d5\u30a3");     put("fyi", "\u30d5\u30a3");     put("fwu", "\u30d5\u30a5");
        put("fe", "\u30d5\u30a7");      put("fwe", "\u30d5\u30a7");     put("fye", "\u30d5\u30a7");
        put("fo", "\u30d5\u30a9");      put("fwo", "\u30d5\u30a9");     put("fya", "\u30d5\u30e3");
        put("fyu", "\u30d5\u30e5");     put("fyo", "\u30d5\u30e7");     put("bu", "\u30d6");
        put("pu", "\u30d7");        put("he", "\u30d8");        put("be", "\u30d9");
        put("pe", "\u30da");        put("ho", "\u30db");        put("bo", "\u30dc");
        put("po", "\u30dd");        put("ma", "\u30de");        put("mi", "\u30df");
        put("myi", "\u30df\u30a3");     put("mye", "\u30df\u30a7");     put("mya", "\u30df\u30e3");
        put("myu", "\u30df\u30e5");     put("myo", "\u30df\u30e7");     put("mu", "\u30e0");
        put("me", "\u30e1");        put("mo", "\u30e2");        put("lya", "\u30e3");
        put("xya", "\u30e3");       put("ya", "\u30e4");        put("lyu", "\u30e5");
        put("xyu", "\u30e5");       put("yu", "\u30e6");        put("lyo", "\u30e7");
        put("xyo", "\u30e7");       put("yo", "\u30e8");        put("ra", "\u30e9");
        put("ri", "\u30ea");        put("ryi", "\u30ea\u30a3");     put("rye", "\u30ea\u30a7");
        put("rya", "\u30ea\u30e3");     put("ryu", "\u30ea\u30e5");     put("ryo", "\u30ea\u30e7");
        put("ru", "\u30eb");        put("re", "\u30ec");        put("ro", "\u30ed");
        put("lwa", "\u30ee");       put("xwa", "\u30ee");       put("wa", "\u30ef");
        put("wo", "\u30f2");        put("nn", "\u30f3");        put("xn", "\u30f3");
        put("vu", "\u30f4");        put("va", "\u30f4\u30a1");      put("vi", "\u30f4\u30a3");
        put("vyi", "\u30f4\u30a3");     put("ve", "\u30f4\u30a7");      put("vye", "\u30f4\u30a7");
        put("vo", "\u30f4\u30a9");      put("vya", "\u30f4\u30e3");     put("vyu", "\u30f4\u30e5");
        put("vyo", "\u30f4\u30e7");     
        put("bb", "\u30c3b");   put("cc", "\u30c3c");   put("dd", "\u30c3d");
        put("ff", "\u30c3f");   put("gg", "\u30c3g");   put("hh", "\u30c3h");
        put("jj", "\u30c3j");   put("kk", "\u30c3k");   put("ll", "\u30c3l");
        put("mm", "\u30c3m");   put("pp", "\u30c3p");   put("qq", "\u30c3q");
        put("rr", "\u30c3r");   put("ss", "\u30c3s");   put("tt", "\u30c3t");
        put("vv", "\u30c3v");   put("ww", "\u30c3w");   put("xx", "\u30c3x");
        put("yy", "\u30c3y");   put("zz", "\u30c3z");   put("nb", "\u30f3b");
        put("nc", "\u30f3c");   put("nd", "\u30f3d");   put("nf", "\u30f3f");
        put("ng", "\u30f3g");   put("nh", "\u30f3h");   put("nj", "\u30f3j");
        put("nk", "\u30f3k");   put("nm", "\u30f3m");   put("np", "\u30f3p");
        put("nq", "\u30f3q");   put("nr", "\u30f3r");   put("ns", "\u30f3s");
        put("nt", "\u30f3t");   put("nv", "\u30f3v");   put("nw", "\u30f3w");
        put("nx", "\u30f3x");   put("nz", "\u30f3z");   put("nl", "\u30f3l");   
        put("-", "\u30fc"); put(".", "\u3002"); put(",", "\u3001"); put("?", "\uff1f"); put("/", "\u30fb");
    }};
	
    public static String[] convertKatakana(String en){
		return convert(en, RomkanToKanaTable);
	}
    
    /** Max length of the target text */
    public static final int MAX_LENGTH = 4;
    
    /**
     * convert Romaji to Full Katakana
     *
     * @param text              The input/output text
     * @param table             HashMap for Romaji-to-Kana conversion
     * @return                  {@code true} if conversion is compleated; {@code false} if not
     */
    public static String[] convert(String text, HashMap<String, String> table) {

    	String[] match = new String[2];
    	match[0] = "";
    	match[1] = "";
    	
    	int checkLength = Math.min(text.length(), MAX_LENGTH);
    	// 
    	for(int i=0;i<checkLength;i++){
    		match[0] = table.get(text.substring(i));

    		if (match[0] != null && match[0].length() < 3) {
				if(match[0].length() == 2){
					match[0].toLowerCase();
					char c = match[0].charAt(1);
					if(c >= 'a' && c <= 'z'){
						match[0] = String.valueOf(match[0].charAt(0));
						match[1] = String.valueOf(c);
					}
				}
    			return match;
    		}
    	}
    	
    	return null;
    }
    
    

    private static final String[][] SpecialSymbolTable = {
    	{"\u0040", "\u00AF", "\u005F", "\u002F", "\u0026"}, // @ - _ / &
    	{"\u0027", "\"", "\u003A", "\u003B"},	// 2 ' " : ;
    	{"\u002E", "\u002C", "\u0021", "\u003F"},	// 3 . , ! ?
    	{"\u007C", "\u007E", "\u003C", "\u003E"},	// 4 | ~ < >
    	{"\u002B", "\u002D", "\u003D", "\u0023"},	// 5 + - = #
    	{"\u0021", "\u0040"},	// 6 ! @
    	{"\u002F", "\u005E"},	// 7 / ^
    	{"\u0028", "\u0029"},	// 8 ( )
    	{"\u0023", "\u007E"},	// 9 # ~
    	{"\u0026", "\u0025"},	// 10 & %
    	{"\u005F", "\u002D"},	// 11 _ -
    	{"\u003C", "\u003E", "\u005B", "\u005D"},	// 12 < > [ ]
    	{"\u007B", "\u007D", "\u007C", "\u00A5"},	// 13 { } | ¥
    	{"\u0027", "\u002B"},	// 14 ' +
    	{"\u00B0", "\u00B7"},	// 15  °, ·
    };
    
    public static boolean isSpecialSymbol(String world) {
		if (world.length() > 0 && world != null) {

			for (int i = 0; i < SpecialSymbolTable.length; i++) {
				for (int j = 0; j < SpecialSymbolTable[i].length; j++) {
					if (world.equals(SpecialSymbolTable[i][j])) {
						return true;
					}
				}
			}
		}
		return false;
	}
    
    // not use
    public static String getSpecialSymbol(String beforeWord){

    	if(beforeWord.length() > 0 && beforeWord != null){
 
    		for(int i=0;i<SpecialSymbolTable.length;i++){
    			for(int j=0;j<SpecialSymbolTable[i].length;j++){
    				if(beforeWord.equals(SpecialSymbolTable[i][j])){
    					if(j == SpecialSymbolTable[i].length - 1){
        					return SpecialSymbolTable[i][0];
        				}else{
        					return SpecialSymbolTable[i][j + 1];
        				}
    				}
    			}
    		}
    	}
    	return "";
    }
    
	public static String getSpecialSymbol(String beforeWord, int index) {
		if (index != -1) { // Add, 170622 yunsuk
			
			if (beforeWord.length() > 0 && beforeWord != null) {
				int length = SpecialSymbolTable[index].length;

				for (int i = 0; i < length; i++) {
					if (beforeWord.equals(SpecialSymbolTable[index][i])) {
						if (i == length - 1) {
							return SpecialSymbolTable[index][0];
						} else {
							return SpecialSymbolTable[index][i + 1];
						}
					}
				}
			}
			return SpecialSymbolTable[index][0];
		}
		
		return null;
	}
    
    
    public static String[][] JP_FULL_ENG_CYCLE_TABLE = {
    	{"a", "b", "c"},
    	{"d", "e", "f"},
    	{"g", "h", "i"},
    	{"j", "k", "l"},
    	{"m", "n", "o"},
    	{"p", "q", "r", "s"},
    	{"t", "u", "v"},
    	{"w", "x", "y", "z"},
    };
    
    public static String getEngCycleTableWord(String beforeWord){
    	for(int i=0;i<JP_FULL_ENG_CYCLE_TABLE.length;i++){
    		for(int j=0;j<JP_FULL_ENG_CYCLE_TABLE[i].length;j++){
    			if(JP_FULL_ENG_CYCLE_TABLE[i][j].equals(beforeWord)){
    				if(j == JP_FULL_ENG_CYCLE_TABLE[i].length - 1){
    					return JP_FULL_ENG_CYCLE_TABLE[i][0];
    				}else{
    					return JP_FULL_ENG_CYCLE_TABLE[i][j+1];
    				}
    			}
    		}
    	}
    	return "";
    }
    
}