package com.example.heart;

import android.app.Activity;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.FrameLayout;
import android.widget.TextView;
import android.graphics.Color;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class MainActivity extends Activity {
    static { System.loadLibrary("heart"); }
    static native void nInit();
    static native void nResize(int w, int h);
    static native void nDraw();
    static native void nTouch(float x, float y);
    static native void nMode(int m);

    private GLSurfaceView view;

    @Override protected void onCreate(Bundle b) {
        super.onCreate(b);
        view = new GLSurfaceView(this);
        view.setEGLContextClientVersion(2);
        view.setRenderer(new GLSurfaceView.Renderer() {
            public void onSurfaceCreated(GL10 g, EGLConfig c) { nInit(); }
            public void onSurfaceChanged(GL10 g, int w, int h) { nResize(w, h); }
            public void onDrawFrame(GL10 g) { nDraw(); }
        });
        view.setOnTouchListener((v, e) -> {
            if (e.getAction() == MotionEvent.ACTION_DOWN || e.getAction() == MotionEvent.ACTION_MOVE) {
                final float x = e.getX(), y = e.getY();
                view.queueEvent(() -> nTouch(x, y));
            }
            return true;
        });
        FrameLayout root = new FrameLayout(this);
        root.addView(view);
        final TextView text = new TextView(this);
        text.setTextColor(Color.WHITE); text.setTextSize(16); text.setGravity(Gravity.CENTER);
        text.setPadding(50, 30, 50, 30);
        final ScrollView scroll = new ScrollView(this);
        scroll.setBackgroundColor(Color.argb(150, 0, 0, 0));
        scroll.addView(text); scroll.setAlpha(0f); scroll.setVisibility(View.GONE);
        int maxH = (int) (getResources().getDisplayMetrics().heightPixels * 0.42f);
        FrameLayout.LayoutParams sp = new FrameLayout.LayoutParams(-1, -2, Gravity.BOTTOM);
        root.addView(scroll, sp);
        final LinearLayout box = new LinearLayout(this);
        box.setOrientation(LinearLayout.VERTICAL); box.setGravity(Gravity.CENTER_HORIZONTAL);
        final Button b1 = mk("Открой если тебе грустно");
        final Button b2 = mk("Открой если мы расстались или на грани расставания");
        box.addView(b1); box.addView(b2);
        FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(-2, -2, Gravity.BOTTOM | Gravity.CENTER_HORIZONTAL);
        lp.bottomMargin = 100;
        root.addView(box, lp);
        final String t1 = "Привет, солнышко. Я чувствую, что тебе грустно, я всё равно с тобой, я здесь, с тобой, рядом... Тебе не нужно держаться бодрячком. Можешь написать, что на душе, или просто рассказать, как прошёл день, я с радостью послушаю. Ты мне очень дорога 🤍, я понимаю что я уже служу и я забросил это приложение, но я предполагал что мы можем расстаться, как бы много я тебя не знал, я пытаюсь быть ближе, ведь только это нас связывает";
        final String t2 = "Привет. Я долго думал, как и что тебе написать, потому что не хочу отделаться пустыми словами. Хочу сказать всё честно.\nЯ вижу, что между нами стало тяжело. Мы далеко друг от друга, общаемся только текстом, и в переписке легко не услышать друг друга, по-своему понять интонацию и обидеться. Я понимаю, что тебе было грустно, одиноко и, возможно, обидно из-за меня. Если я делал что-то, что тебя ранило, или, наоборот, не делал того, что тебе было нужно, мне правда жаль. Я не хочу оправдываться, хочу понять.\nПоэтому я хочу просто спросить: что ты сейчас чувствуешь? Что тебя больше всего ранит или утомляет? Чего тебе не хватает от меня? Я обещаю слушать до конца, не перебивать и не защищаться. Если нужно время подумать, бери его, я понимаю и не буду лезть или осуждать";
        final int[] tall = {maxH};
        View.OnClickListener open = v -> {
            final boolean sad = v == b1;
            view.queueEvent(() -> nMode(sad ? 1 : 2));
            text.setText(sad ? t1 : t2);
            box.setVisibility(View.GONE);
            scroll.setVisibility(View.VISIBLE);
            scroll.post(() -> { if (scroll.getHeight() > tall[0]) { scroll.getLayoutParams().height = tall[0]; scroll.requestLayout(); } });
            scroll.animate().alpha(1f).setDuration(1500).start();
        };
        b1.setOnClickListener(open); b2.setOnClickListener(open);
        setContentView(root);
    }
    private Button mk(String s) {
        Button b = new Button(this);
        b.setText(s); b.setTextColor(Color.WHITE); b.setAllCaps(false);
        b.setBackgroundColor(Color.parseColor("#2563eb"));
        LinearLayout.LayoutParams l = new LinearLayout.LayoutParams(-2, -2);
        l.bottomMargin = 20; b.setLayoutParams(l);
        return b;
    }
    @Override protected void onPause()  { super.onPause();  view.onPause(); }
    @Override protected void onResume() { super.onResume(); view.onResume(); }
}
