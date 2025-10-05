#include <SDL2/SDL.h>
#include <stdio.h>     // fprintf, stderr
#include <math.h>      // sin, cos, M_PI, round
#include <stdbool.h>   // тип bool
#include <SDL2/SDL_ttf.h> // для текста


const int WIDTH = 800;  // ширина экрана 
const int HEIGHT = 600; // высота экрана


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// простая структура точек с целочисленными координатами
typedef struct { int x, y; } Point;

// структура треугольника
typedef struct { Point p1, p2, p3; } Triangle;

// матрица 3 на 3
typedef struct { double m[3][3]; } Mat3;


// ---- МАТЕМАТИКА ----

// Единичная матрица
Mat3 identity() {
    Mat3 mat = {{{1,0,0},{0,1,0},{0,0,1}}};
    return mat;
}

// масштабирование
Mat3 scaleMatrix(double s) {
    Mat3 mat = identity();
    mat.m[0][0] = s;
    mat.m[1][1] = s;
    return mat;
}

// вращение вокруг центра
Mat3 rotateMatrix(double angle_deg, Point center) {
    double rad = angle_deg * M_PI / 180.0;
    double c = cos(rad), s = sin(rad);
    Mat3 mat = identity();
    mat.m[0][0] = c;  mat.m[0][1] = -s;
    mat.m[1][0] = s;  mat.m[1][1] = c;

    // сдвиг чтобы вращение было относительно центра
    mat.m[0][2] = center.x - center.x*c + center.y*s;
    mat.m[1][2] = center.y - center.x*s - center.y*c;
    return mat;
}

// сдвиг
Mat3 translateMatrix(int dx, int dy) {
    Mat3 mat = identity();
    mat.m[0][2] = dx;
    mat.m[1][2] = dy;
    return mat;
}

// умножение матриц
Mat3 multiply(Mat3 a, Mat3 b) {
    Mat3 r = {{{0}}};
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            for (int k = 0; k < 3; k++)
                r.m[i][j] += a.m[i][k] * b.m[k][j];
    return r;
}

// применение матрицы к точке
Point transformPoint(Mat3 m, Point p) {
    double x = m.m[0][0]*p.x + m.m[0][1]*p.y + m.m[0][2];
    double y = m.m[1][0]*p.x + m.m[1][1]*p.y + m.m[1][2];
    return (Point){ (int)round(x), (int)round(y) };
}


// ---- SDL ----

// вывод текста
void renderText(SDL_Renderer *renderer, TTF_Font *font,
                const char *text, int x, int y) {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface *surface = TTF_RenderText_Blended(font, text, white);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect dst = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &dst);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

// ---- ЗАКРАСКА ----

// векторное произведение (для проверки точки)
static double edge(Point a, Point b, Point c) {
    return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

// проверка попадания точки внутрь треугольника
bool inside(Point p, Point a, Point b, Point c) {
    double w1 = edge(a,b,p);
    double w2 = edge(b,c,p);
    double w3 = edge(c,a,p);
    bool hasNeg = (w1 < 0) || (w2 < 0) || (w3 < 0);
    bool hasPos = (w1 > 0) || (w2 > 0) || (w3 > 0);
    return !(hasNeg && hasPos);
}

// закраска треугольника точками
void fillTrianglePoint(SDL_Renderer *renderer, Point a, Point b, Point c) {
    int minx = fmin(a.x, fmin(b.x, c.x));
    int maxx = fmax(a.x, fmax(b.x, c.x));
    int miny = fmin(a.y, fmin(b.y, c.y));
    int maxy = fmax(a.y, fmax(b.y, c.y));

    if (minx < 0) minx = 0;
    if (miny < 0) miny = 0;
    if (maxx >= WIDTH) maxx = WIDTH - 1;
    if (maxy >= HEIGHT) maxy = HEIGHT - 1;

    for (int y = miny; y <= maxy; y++) {
        for (int x = minx; x <= maxx; x++) {
            Point p = {x,y};
            if (inside(p,a,b,c)) {
                SDL_RenderDrawPoint(renderer,x,y);
            }
        }
    }
}


// ---- MAIN ----
int main(int argc, char *argv[]) {
    // инициализация SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Triangle Transformations",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIDTH, HEIGHT, SDL_WINDOW_SHOWN
    ); 
    
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // исходный треугольник (в "локальных" координатах)
    Triangle tri = { {500, 200}, {400, 400}, {200, 400} };

    bool running = true;
    SDL_Event e;

    double angle = 0;   // угол поворота
    double scale = 1.0; // масштаб
    int dx = 0, dy = 0; // сдвиг 

    // // центр вращения/масштабирования 
    Point center = { WIDTH / 2, HEIGHT / 2 };

    // инициализация TTF
    if (TTF_Init() == -1) {
        fprintf(stderr, "TTF error: %s\n", TTF_GetError());
        return 1;
    }
    TTF_Font *font = TTF_OpenFont("Arial.ttf", 16);
    if (!font) {
        fprintf(stderr, "Font load error: %s\n", TTF_GetError());
        return 1;
    }

    // главный цикл
    while (running) {
        // события 
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_ESCAPE: running = false; break;
                    case SDLK_UP:    dy -= 5; break;
                    case SDLK_DOWN:  dy += 5; break;
                    case SDLK_LEFT:  dx -= 5; break;
                    case SDLK_RIGHT: dx += 5; break;
                    case SDLK_q: angle -= 5; break;
                    case SDLK_e: angle += 5; break;
                    case SDLK_z: scale *= 0.9; break;
                    case SDLK_x: scale *= 1.1; break;
                }
            }
        }

        // очистка
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // ВЫЧИСЛЯЕМ ЦЕНТР ТРЕУГОЛЬНИКА В ИСХОДНЫХ КООРДИНАТАХ
        Point triangleCenter = {
            (tri.p1.x + tri.p2.x + tri.p3.x) / 3,
            (tri.p1.y + tri.p2.y + tri.p3.y) / 3
        };

        // ПРАВИЛЬНЫЙ ПОРЯДОК ПРЕОБРАЗОВАНИЙ:
        // 1. Перенос в начало координат (относительно центра)
        // 2. Масштабирование
        // 3. Поворот  
        // 4. Перенос обратно + дополнительный сдвиг
        Mat3 M = multiply(
            translateMatrix(dx, dy),    // 4. конечный перенос

            multiply(
                translateMatrix(triangleCenter.x, triangleCenter.y), // 3. возврат обратно

                multiply(
                    rotateMatrix(angle, (Point){0, 0}),       // 2. поворот вокруг начала координат

                    multiply(
                        scaleMatrix(scale),                   // 1b. масштабирование
                        translateMatrix(-triangleCenter.x, -triangleCenter.y)  // 1a. перенос в начало координат
                    )
                )
            )
        );

        // преобразуем вершины
        Point p1 = transformPoint(M, tri.p1);
        Point p2 = transformPoint(M, tri.p2);
        Point p3 = transformPoint(M, tri.p3);
        
        // закрашиваем треугольник точками
        SDL_SetRenderDrawColor(renderer, 100,200,255,255);
        fillTrianglePoint(renderer, p1,p2,p3);

        // контур белым
        SDL_SetRenderDrawColor(renderer,255,255,255,255);
        SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
        SDL_RenderDrawLine(renderer, p2.x, p2.y, p3.x, p3.y);
        SDL_RenderDrawLine(renderer, p3.x, p3.y, p1.x, p1.y);

        // подсказки
        renderText(renderer, font, "Arrows: Move | Q/E: Rotate | Z/X: Scale | Esc: Exit", 10, 10);

        // показать результат
        SDL_RenderPresent(renderer);

        SDL_Delay(16); // ограничение FPS
    }
    
    // очистка
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    TTF_CloseFont(font);
    TTF_Quit();
    return 0;
}




