// Анимированное сердце из частиц: C++ + OpenGL ES 2.0 (JNI)
#include <jni.h>
#include <GLES2/gl2.h>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <chrono>

static float rnd() { return rand() / (float)RAND_MAX; }
static float rr(float a, float b) { return a + (b - a) * rnd(); }

enum Kind { FLOAT, MAIN, BURST, RING };

struct Particle {
    float x, y, vx, vy, tx, ty, life, maxLife, size, ang;
    float r, g, b;
    Kind kind;
};

// Оттенки синего (как BLUE_SHADES в оригинале)
static const float BLUES[][3] = {
    {0x3b/255.f, 0x82/255.f, 0xf6/255.f}, {0x60/255.f, 0xa5/255.f, 0xfa/255.f},
    {0x93/255.f, 0xc5/255.f, 0xfd/255.f}, {0x25/255.f, 0x63/255.f, 0xeb/255.f},
    {0xbf/255.f, 0xdb/255.f, 0xfe/255.f}, {0x1d/255.f, 0x4e/255.f, 0xd8/255.f},
    {0.3f, 0.9f, 1.0f}};

static const int FLOAT_COUNT = 200, MAIN_COUNT = 14000, RING_COUNT = 1000, BURST_COUNT = 260;
static int mode = 0;
static const float REDS[][3] = {{0.94f,0.27f,0.27f},{0.97f,0.44f,0.44f},{0.99f,0.65f,0.65f},{0.86f,0.15f,0.15f},{1.0f,0.8f,0.8f},{0.72f,0.11f,0.11f},{1.0f,0.35f,0.5f}};

static std::vector<Particle> floatP, mainP, burstP, ringP;
static std::vector<float> verts;
static GLuint prog;
static GLint aPos, aCol, aSize, uRes;
static float W = 1, H = 1, S = 1, cx = 0, cy = 0, T = 0;
static std::chrono::steady_clock::time_point last;

// Точка внутри сердца (в системе координат «сердца», ~[-16..16] x [-17..13])
static void heartPoint(float &hx, float &hy) {
    float t = rnd() * 6.2831853f;
    float k = 1.0f - 0.45f * powf(rnd(), 2.2f);   // больше точек у контура
    float x = 16 * powf(sinf(t), 3);
    float y = 13 * cosf(t) - 5 * cosf(2 * t) - 2 * cosf(3 * t) - cosf(4 * t);
    hx = x * k; hy = (y + 2) * k;
}

static void setTarget(Particle &p) {
    float hx, hy; heartPoint(hx, hy);
    float pulse = 1.0f + 0.04f * sinf(T * 3.0f);
    p.tx = cx + hx * S * pulse;
    p.ty = cy - hy * S * pulse;
}

static void hsv(float h, float &r, float &g, float &b) {
    h = h - floorf(h); float x = h * 6.f; int i = (int)x; float f = x - i;
    float q = 1 - f;
    switch (i % 6) { case 0: r=1;g=f;b=0; break; case 1: r=q;g=1;b=0; break; case 2: r=0;g=1;b=f; break;
                     case 3: r=0;g=q;b=1; break; case 4: r=f;g=0;b=1; break; default: r=1;g=0;b=q; }
    r = 0.35f + 0.65f * r; g = 0.35f + 0.65f * g; b = 0.35f + 0.65f * b;
}
static void rainbow(Particle &p) { if (mode == 2) hsv(T * 0.35f + p.x * 0.0015f + p.y * 0.0012f, p.r, p.g, p.b); }

static void color(Particle &p) {
    const float *c = mode == 1 ? REDS[rand() % 7] : BLUES[rand() % 7];
    p.r = c[0]; p.g = c[1]; p.b = c[2];
}

static void spawnMain(Particle &p, bool stagger) {
    p.kind = MAIN;
    float a = rnd() * 6.2831853f, r = sqrtf(rnd());
    p.x = cx + cosf(a) * r * S * 12;                 // «бассейн» внизу
    p.y = cy + S * 19 + sinf(a) * r * S * 2.5f;
    p.vx = rr(-20, 20); p.vy = -rr(40, 160) * S / 12;
    p.maxLife = rr(3.0f, 6.0f);
    p.life = stagger ? rnd() * p.maxLife : 0;
    p.size = rr(4.5f, 10.0f) * (W / 540.f);
    p.ang = rnd() * 6.28f;
    color(p); setTarget(p);
}

static void spawnFloat(Particle &p) {
    p.kind = FLOAT;
    p.x = rnd() * W; p.y = rnd() * H;
    p.vx = rr(-8, 8); p.vy = rr(5, 25);
    p.maxLife = rr(3, 8); p.life = rnd() * p.maxLife;   // stagger starting life
    p.size = rr(1.5f, 3.5f) * (W / 540.f);
    color(p);
}

static void spawnRing(Particle &p) {
    p.kind = RING;
    p.ang = rnd() * 6.2831853f;
    p.vx = rr(0.8f, 2.2f);                              // угловая скорость
    p.vy = rr(0.85f, 1.15f);                            // радиус
    p.size = rr(5.0f, 11.0f) * (W / 540.f);
    p.life = 0; p.maxLife = 1;
    color(p);
}

// Всплеск сердца в точке касания (spawnHeartBurst)
static void spawnHeartBurst(float x, float y) {
    for (int i = 0; i < BURST_COUNT; i++) {
        Particle p{}; p.kind = BURST;
        p.x = x; p.y = y;
        float a = rnd() * 6.2831853f, sp = rr(100, 600) * (W / 540.f);
        p.vx = cosf(a) * sp; p.vy = sinf(a) * sp;
        p.maxLife = rr(2.0f, 4.0f); p.life = 0;
        p.size = rr(4.0f, 10.0f) * (W / 540.f);
        color(p); setTarget(p);
        burstP.push_back(p);
    }
}

static void push(const Particle &p, float alpha, float size) {
    verts.insert(verts.end(), {p.x, p.y, p.r * alpha, p.g * alpha, p.b * alpha, alpha, size});
}

static void step(Particle &p, float dt, float k, float damp) {
    float ax = (p.tx - p.x) * k - p.vx * damp + rr(-1, 1) * 900 * S / 12;
    float ay = (p.ty - p.y) * k - p.vy * damp + rr(-1, 1) * 900 * S / 12;
    p.vx += ax * dt; p.vy += ay * dt;
    p.x += p.vx * dt; p.y += p.vy * dt;
}

static GLuint compile(GLenum t, const char *src) {
    GLuint s = glCreateShader(t); glShaderSource(s, 1, &src, nullptr); glCompileShader(s); return s;
}

extern "C" {

JNIEXPORT void JNICALL Java_com_example_heart_MainActivity_nMode(JNIEnv *, jclass, jint m) {
    mode = m;
    for (auto *v : {&floatP, &mainP, &burstP, &ringP}) for (auto &p : *v) color(p);
}

JNIEXPORT void JNICALL Java_com_example_heart_MainActivity_nInit(JNIEnv *, jclass) {
    const char *vs =
        "attribute vec2 aPos; attribute vec4 aCol; attribute float aSize; uniform vec2 uRes;"
        "varying vec4 vCol; void main(){ vec2 p = aPos / uRes * 2.0 - 1.0;"
        "gl_Position = vec4(p.x, -p.y, 0.0, 1.0); gl_PointSize = aSize; vCol = aCol; }";
    const char *fs =
        "precision mediump float; varying vec4 vCol; void main(){"
        "float d = length(gl_PointCoord - 0.5) * 2.0; if(d > 1.0) discard;"
        "float a = pow(1.0 - d, 2.0); gl_FragColor = vCol * a * 2.4; }";
    prog = glCreateProgram();
    glAttachShader(prog, compile(GL_VERTEX_SHADER, vs));
    glAttachShader(prog, compile(GL_FRAGMENT_SHADER, fs));
    glLinkProgram(prog);
    aPos = glGetAttribLocation(prog, "aPos"); aCol = glGetAttribLocation(prog, "aCol");
    aSize = glGetAttribLocation(prog, "aSize"); uRes = glGetUniformLocation(prog, "uRes");
    last = std::chrono::steady_clock::now();
}

JNIEXPORT void JNICALL Java_com_example_heart_MainActivity_nResize(JNIEnv *, jclass, jint w, jint h) {
    W = w; H = h; glViewport(0, 0, w, h);
    cx = W / 2; cy = H * 0.42f;
    S = fminf(W / 36.f, H / 42.f);
    floatP.assign(FLOAT_COUNT, {});  for (auto &p : floatP) spawnFloat(p);   // создание float-частиц
    mainP.assign(MAIN_COUNT, {});    for (auto &p : mainP) spawnMain(p, true);
    ringP.assign(RING_COUNT, {});    for (auto &p : ringP) spawnRing(p);
    burstP.clear();
}

JNIEXPORT void JNICALL Java_com_example_heart_MainActivity_nTouch(JNIEnv *, jclass, jfloat x, jfloat y) {
    if (burstP.size() < 6000) spawnHeartBurst(x, y);
}

JNIEXPORT void JNICALL Java_com_example_heart_MainActivity_nDraw(JNIEnv *, jclass) {
    auto now = std::chrono::steady_clock::now();
    float dt = fminf(std::chrono::duration<float>(now - last).count(), 0.05f);
    last = now; T += dt;

    // --- обновление ---
    for (auto &p : floatP) {
        p.life += dt; p.x += p.vx * dt; p.y -= p.vy * dt;
        if (p.life > p.maxLife || p.y < 0) spawnFloat(p), p.life = 0;
    }
    for (auto &p : mainP) {
        p.life += dt;
        step(p, dt, p.life < 1.0f ? 1.5f : 6.0f, 2.2f);   // подъём, затем «прилипание» к сердцу
        if (p.life > p.maxLife) spawnMain(p, false);
        else if (rnd() < 0.01f) setTarget(p);              // мерцание/дрейф по сердцу
    }
    for (size_t i = 0; i < burstP.size();) {
        auto &p = burstP[i]; p.life += dt;
        step(p, dt, p.life > 0.4f ? 8.0f : 0.0f, 2.0f);
        if (p.life > p.maxLife) { burstP[i] = burstP.back(); burstP.pop_back(); } else i++;
    }
    for (auto &p : ringP) {
        p.ang += p.vx * dt;
        p.x = cx + cosf(p.ang) * S * 13 * p.vy;
        p.y = cy + S * 19 + sinf(p.ang) * S * 2.6f * p.vy;
    }

    if (mode == 2) for (auto *v : {&floatP, &mainP, &burstP, &ringP}) for (auto &p : *v) rainbow(p);
    // --- сборка вершин ---
    verts.clear();
    for (auto &p : floatP) { float f = 1 - fabsf(2 * p.life / p.maxLife - 1); push(p, 0.5f * f, p.size); }
    for (auto &p : mainP)  { float f = sinf(fminf(p.life / p.maxLife, 1.f) * 3.14159f);
                             push(p, 0.35f + 0.65f * f, p.size); }
    for (auto &p : burstP) { float f = 1 - p.life / p.maxLife; push(p, f, p.size * 1.2f); }
    for (auto &p : ringP)  { push(p, 0.7f + 0.3f * sinf(T * 4 + p.ang * 3), p.size); }

    // --- рисование ---
    glClearColor(0, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE);      // аддитивное свечение
    glUseProgram(prog); glUniform2f(uRes, W, H);
    const int stride = 7 * sizeof(float);
    glEnableVertexAttribArray(aPos);  glVertexAttribPointer(aPos, 2, GL_FLOAT, GL_FALSE, stride, verts.data());
    glEnableVertexAttribArray(aCol);  glVertexAttribPointer(aCol, 4, GL_FLOAT, GL_FALSE, stride, verts.data() + 2);
    glEnableVertexAttribArray(aSize); glVertexAttribPointer(aSize, 1, GL_FLOAT, GL_FALSE, stride, verts.data() + 6);
    glDrawArrays(GL_POINTS, 0, (GLsizei)(verts.size() / 7));
}

}
