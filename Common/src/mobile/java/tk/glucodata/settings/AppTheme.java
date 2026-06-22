package tk.glucodata.settings;

import androidx.annotation.NonNull;
import java.util.ArrayList;
import java.util.List;
import tk.glucodata.R;

public enum AppTheme {
    // DARK THEMES
    DEFAULT("Default", R.style.AppTheme_AndroidDefault, true),
    DARK("Dark", R.style.AppTheme_Dark, true),
    OCEANIC("Oceanic Dark", R.style.AppTheme_OceanicDark, true),
    SUNSET("Sunset Elegance", R.style.AppTheme_SunsetElegance, true),
    OBSIDIAN("Developer Obsidian", R.style.AppTheme_DeveloperObsidian, true),
    AMBERDUSK("Amber Dusk", R.style.AppTheme_AmberDusk, true),
    MusicNeon("Music Neon", R.style.AppTheme_MusicNeon, true),
    SocialSunset("Social Sunset", R.style.AppTheme_SocialSunset, true),
    RoyalNight("Royal Night", R.style.AppTheme_RoyalNight, true),
    OLEDBlack("OLED Black", R.style.AppTheme_OLEDBlack, true),
    HighVisibility("High Visibility", R.style.AppTheme_HighVisibility, true),
    SolarizedDark("Solarized Dark", R.style.AppTheme_SolarizedDark, true),
    CyberpunkNeon("Cyberpunk Neon", R.style.AppTheme_CyberpunkNeon, true),
CUPERTINO_DARK("Cupertino Dark", R.style.AppTheme_CupertinoDark, true),
MIDNIGHT_INDIGO("Midnight Indigo", R.style.AppTheme_MidnightIndigo, true),
DEEP_TEAL("Deep Teal", R.style.AppTheme_DeepTeal, true),
PLUM_NIGHT("Plum Night", R.style.AppTheme_PlumNight, true),
TERMINAL_GREEN("Terminal Green", R.style.AppTheme_TerminalGreen, true),


// DARK THEMES

// LIGHT THEMES

    // LIGHT THEMES
    LIGHT("Light", R.style.AppTheme_Light, false),
    CLASSIC("Material Classic", R.style.AppTheme_MaterialClassic, false),
    FOREST("Forest Light", R.style.AppTheme_ForestLight, false),
    NORDICFROST("Nordic Frost", R.style.AppTheme_NordicFrost, false),
    LAVENDERDREAM("Lavender Dream", R.style.AppTheme_LavenderDream, false),
    MessagingEmerald("Messaging Emerald", R.style.AppTheme_MessagingEmerald, false),
    VideoCrimson("Video Crimson", R.style.AppTheme_VideoCrimson, false),
    LearningLime("Learning Lime", R.style.AppTheme_LearningLime, false),
    MinimalInk("Minimal Ink", R.style.AppTheme_MinimalInk, false),
    CoffeeCream("Coffee Cream", R.style.AppTheme_CoffeeCream, false),
    CloudBlue("Cloud Blue", R.style.AppTheme_CloudBlue, false),
    VintageParchment("Vintage Parchment", R.style.AppTheme_VintageParchment, false),
    MatchaMint("Matcha Mint", R.style.AppTheme_MatchaMint, false),
    RoseQuartz("Rose Quartz", R.style.AppTheme_RoseQuartz, false),
CUPERTINO_LIGHT("Cupertino Light", R.style.AppTheme_CupertinoLight, false),
SAGE_PAPER("Sage Paper", R.style.AppTheme_SagePaper, false),
SKY_MIST("Sky Mist", R.style.AppTheme_SkyMist, false),
PEACH_SOFT("Peach Soft", R.style.AppTheme_PeachSoft, false),
CANDY_PASTEL("Candy Pastel", R.style.AppTheme_CandyPastel, false),

    // PAINTING-INSPIRED THEMES
    STARRY_NIGHT("Starry Night", R.style.AppTheme_StarryNight, true),
    GIRL_WITH_PEARL("Girl with Pearl", R.style.AppTheme_GirlWithPearl, true),
    NIGHT_WATCH("Night Watch", R.style.AppTheme_NightWatch, true),
    ROTHKO_DUSK("Rothko Dusk", R.style.AppTheme_RothkoDusk, true),
    ALMOND_BLOSSOM("Almond Blossom", R.style.AppTheme_AlmondBlossom, false),
    GREAT_WAVE("Great Wave", R.style.AppTheme_GreatWave, false),
    WATER_LILIES("Water Lilies", R.style.AppTheme_WaterLilies, false),
    SUNFLOWERS("Sunflowers", R.style.AppTheme_Sunflowers, false),
    MONDRIAN_GRID("Mondrian Grid", R.style.AppTheme_MondrianGrid, false),
    UKIYOE_VERMILION("Ukiyo-e Vermilion", R.style.AppTheme_UkiyoeVermilion, false),

    // ERGONOMIC / PERCEPTION-DRIVEN THEMES
    SUNLIGHT_READABLE("Sunlight Readable", R.style.AppTheme_SunlightReadable, false),
    PAPER_LOW_GLARE("Paper Low Glare", R.style.AppTheme_PaperLowGlare, false),
    DEUTERANOPIA_SAFE("Deuteranopia Safe", R.style.AppTheme_DeuteranopiaSafe, false),
    PROTANOPIA_SAFE_DARK("Protanopia Safe Dark", R.style.AppTheme_ProtanopiaSafeDark, true),
    TRITANOPIA_SAFE("Tritanopia Safe", R.style.AppTheme_TritanopiaSafe, false),
    SENIOR_CONTRAST("Senior Contrast", R.style.AppTheme_SeniorContrast, false),
    ALARM_RESERVED("Alarm Reserved", R.style.AppTheme_AlarmReserved, true),
    PERIPHERAL_SALIENCE("Peripheral Salience", R.style.AppTheme_PeripheralSalience, true);

    private final String displayName;
    private final int styleResId;
    private final boolean isDark;

    AppTheme(String displayName, int styleResId, boolean isDark) {
        this.displayName = displayName;
        this.styleResId = styleResId;
        this.isDark = isDark;
    }

    public int getStyleResId() {
        return styleResId;
    }

    public boolean isDark() {
        return isDark;
    }

    public static AppTheme[] getThemesByGroup(boolean darkGroup) {
        int count=0;
        for(AppTheme theme : values()) 
            if(theme.isDark == darkGroup) 
                ++count;
        AppTheme[] uit=new AppTheme[count];
        int it=0;
        for(AppTheme theme : values()) 
            if(theme.isDark == darkGroup) 
                uit[it++]=theme;
        return uit;
       }

    @NonNull
    @Override
    public String toString() {
        return displayName;
    }
}
