
package tk.glucodata;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.style.LineBackgroundSpan;
import android.text.style.ReplacementSpan;
import android.util.Log;
import android.widget.TextView;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Draws the [x] / [ ] symbols in an aligned column beside each RTL menu,
 * without changing Android's layout of the Persian labels and their links.
 *
 * The HTML contains an invisible U+2063 delimiter at the START and END
 * of each menu-with-boxes. This provides exact per-menu ranges even when
 * Html.fromHtml splits colors into separate spans for links and line breaks.
 *
 * IMPORTANT: Use this helper WITH the supplied introhelp.in.html; the HTML
 * and Java helper are a matched pair.
 */
public final class HelpCheckboxAlignment {
    private static final String TAG = "HelpCheckboxAlignment";
    private static final char MENU_BOUNDARY = '\u2063';
    private static final Pattern BOX = Pattern.compile("\\[[xX ]\\]");
    private static final int MENU_YELLOW = 0xFFFFFF6D;

    private HelpCheckboxAlignment() {}

    public static CharSequence alignLeft(CharSequence formattedHelp, TextView textView) {
        if (formattedHelp == null || textView == null) return formattedHelp;
        String plain = formattedHelp.toString();
        SpannableStringBuilder out = new SpannableStringBuilder(formattedHelp);
        Paint labelPaint = new Paint(textView.getPaint());
        float gap = labelPaint.getTextSize() * 0.45f;
        int scan = 0;
        int menus = 0;
        int boxes = 0;

        while (true) {
            int begin = plain.indexOf(MENU_BOUNDARY, scan);
            if (begin == -1) break;
            int finish = plain.indexOf(MENU_BOUNDARY, begin + 1);
            if (finish == -1) break;
            // The separators are invisible; only the text between them is
            // considered a menu. All rows contribute to the maximum width,
            // including rows that do not have a checkbox.
            float widestLabel = 0;
            int rowStart = begin + 1;
            while (rowStart < finish) {
                int nl = plain.indexOf('\n', rowStart);
                int rowEnd = nl < 0 || nl > finish ? finish : nl;
                String label = labelOnly(plain.substring(rowStart, rowEnd));
                if (!label.isEmpty()) {
                    widestLabel = Math.max(widestLabel, labelPaint.measureText(label));
                }
                rowStart = rowEnd + 1;
            }

            Matcher matcher = BOX.matcher(plain);
            matcher.region(begin + 1, finish);
            while (matcher.find()) {
                String box = matcher.group();
                out.setSpan(new InvisibleBox(), matcher.start(), matcher.end(),
                        Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                out.setSpan(new MenuColumnBox(box, widestLabel, gap),
                        matcher.start(), matcher.end(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                boxes++;
            }
            menus++;
            scan = finish + 1;
        }
        Log.i(TAG, "Aligned " + boxes + " checkboxes in " + menus + " menus");
        return out;
    }

    private static String labelOnly(String line) {
        String noBox = BOX.matcher(line).replaceAll("");
        StringBuilder sb = new StringBuilder(noBox.length());
        for (int i = 0; i < noBox.length(); i++) {
            char c = noBox.charAt(i);
            // Strip Bidi controls added by the HTML source, but do NOT strip
            // Persian ZWNJ (U+200C), which affects Arabic shaping and width.
            if (c == '\u200e' || c == '\u200f' || c == '\u061c'
                    || c == '\u2066' || c == '\u2067' || c == '\u2068'
                    || c == '\u2069' || c == MENU_BOUNDARY) continue;
            sb.append(c);
        }
        return sb.toString().trim();
    }

    /** Remove the original inline checkbox, including its layout width. */
    private static final class InvisibleBox extends ReplacementSpan {
        @Override
        public int getSize(Paint p, CharSequence text, int start, int end,
                           Paint.FontMetricsInt fm) {
            return 0;
        }

        @Override
        public void draw(Canvas canvas, CharSequence text, int start, int end,
                         float x, int top, int y, int bottom, Paint paint) {
            // Drawn by MenuColumnBox, using the line's absolute right edge.
        }
    }

    /**
     * The x coordinate comes from the line's content area, NOT from the
     * checkbox's RTL/LTR run position. This removes all dependence on the
     * length of the label on the current line.
     */
    private static final class MenuColumnBox implements LineBackgroundSpan {
        private final String box;
        private final float widestLabel;
        private final float gap;

        MenuColumnBox(String box, float widestLabel, float gap) {
            this.box = box;
            this.widestLabel = widestLabel;
            this.gap = gap;
        }

        @Override
        public void drawBackground(Canvas canvas, Paint paint, int left, int right,
                                   int top, int baseline, int bottom,
                                   CharSequence text, int start, int end,
                                   int lineNumber) {
            Paint glyph = new Paint(paint);
            glyph.setColor(MENU_YELLOW);
            glyph.setTypeface(Typeface.MONOSPACE);
            glyph.setTextAlign(Paint.Align.LEFT);
            float width = glyph.measureText(box);
            // All labels end at the same RIGHT edge. Place each checkbox
            // just LEFT of the longest label, in a fixed column per menu.
            float x = right - widestLabel - gap - width;
            canvas.drawText(box, x, baseline, glyph);
        }
    }
}
