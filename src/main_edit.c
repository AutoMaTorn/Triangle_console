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

// Структура точки с целочисленными координатами
typedef struct { int x, y; } Point;

// Матрица 3x3 для преобразований
typedef struct { double m[3][3]; } Mat3;

// _____________МАТЕМАТИКА______________

// Единичная матрица
Mat3 identity() {
    Mat3 mat = {{{1,0,0},{0,1,0},{0,0,1}}};
    return mat;
}

// Умножение матриц (добавленная функция)
Mat3 matMultiply(Mat3 a, Mat3 b) {
    Mat3 result = {{{0}}};
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            for (int k = 0; k < 3; k++) {
                result.m[i][j] += a.m[i][k] * b.m[k][j];
            }
        }
    }
    return result;
}

// Матрица масштабирования
Mat3 scaleMatrix(double s) {
    Mat3 mat = identity();
    mat.m[0][0] = s;
    mat.m[1][1] = s;
    return mat;
}

// Матрица поворота вокруг центра
Mat3 rotateMatrix(double angle_deg, Point center) {
    double rad = angle_deg * M_PI / 180.0;
    double c = cos(rad), s = sin(rad);
    return (Mat3){{
        {c, -s, center.x*(1-c) + center.y*s},
        {s, c, center.y*(1-c) - center.x*s},
        {0, 0, 1}
    }};
}

// Матрица переноса
Mat3 translateMatrix(int dx, int dy) {
    Mat3 mat = identity();
    mat.m[0][2] = dx;
    mat.m[1][2] = dy;
    return mat;
}

// Применение матрицы к точке
Point transformPoint(Mat3 m, Point p) {
    double x = m.m[0][0]*p.x + m.m[0][1]*p.y + m.m[0][2];
    double y = m.m[1][0]*p.x + m.m[1][1]*p.y + m.m[1][2];
    return (Point){ (int)round(x), (int)round(y) };
}

// _________________SDL___________________

// Вывод текста
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

// _________________ЗАКРАСКА________________

// векторное произведение (для проверки точки)
static double edge(Point a, Point b, Point c) {
    return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

// Проверка попадания точки внутрь треугольника
bool inside(Point p, Point a, Point b, Point c) {
    double w1 = edge(a,b,p);
    double w2 = edge(b,c,p);
    double w3 = edge(c,a,p);
    bool hasNeg = (w1 < 0) || (w2 < 0) || (w3 < 0);
    bool hasPos = (w1 > 0) || (w2 > 0) || (w3 > 0);
    return !(hasNeg && hasPos);
}

// Ограничение координат в пределах экрана
static void clampToScreen(int *minx, int *maxx, int *miny, int *maxy) {
    if (*minx < 0) *minx = 0;
    if (*miny < 0) *miny = 0;
    if (*maxx >= WIDTH) *maxx = WIDTH - 1;
    if (*maxy >= HEIGHT) *maxy = HEIGHT - 1;
}

// Закраска треугольника точками
void fillTrianglePoint(SDL_Renderer *renderer, Point points[3]) {
    int minx = fmin(points[0].x, fmin(points[1].x, points[2].x));
    int maxx = fmax(points[0].x, fmax(points[1].x, points[2].x));
    int miny = fmin(points[0].y, fmin(points[1].y, points[2].y));
    int maxy = fmax(points[0].y, fmax(points[1].y, points[2].y));

    clampToScreen(&minx, &maxx, &miny, &maxy);

    for (int y = miny; y <= maxy; y++) {
        for (int x = minx; x <= maxx; x++) {
            Point p = {x,y};
            if (inside(p, points[0], points[1], points[2])) {
                SDL_RenderDrawPoint(renderer,x,y);
            }
        }
    }
}

// ____________MAIN_______________

int main(int argc, char *argv[]) {
    // Инициализация SDL
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

    // Исходный треугольник (все точки в одном массиве)
    Point trianglePoints[3] = {{500, 200}, {400, 400}, {200, 400}};

    bool running = true;
    SDL_Event e;

    double angle = 0;
    double scale = 1.0;
    int dx = 0, dy = 0;

    // Инициализация TTF
    if (TTF_Init() == -1) {
        fprintf(stderr, "TTF error: %s\n", TTF_GetError());
        return 1;
    }
    TTF_Font *font = TTF_OpenFont("Arial.ttf", 16);
    if (!font) {
        fprintf(stderr, "Font load error: %s\n", TTF_GetError());
        return 1;
    }

    // Главный цикл
    while (running) {
        // Обработка событий
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

        // Очистка экрана
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Вычисление центра треугольника
        Point triangleCenter = {
            (trianglePoints[0].x + trianglePoints[1].x + trianglePoints[2].x) / 3,
            (trianglePoints[0].y + trianglePoints[1].y + trianglePoints[2].y) / 3
        };

        // Создание матрицы преобразования
        Mat3 transform = matMultiply(
            translateMatrix(dx, dy),
            matMultiply(
                translateMatrix(triangleCenter.x, triangleCenter.y),
                matMultiply(
                    rotateMatrix(angle, (Point){0, 0}),
                    matMultiply(
                        scaleMatrix(scale),
                        translateMatrix(-triangleCenter.x, -triangleCenter.y)
                    )
                )
            )
        );

        // Преобразование всех точек треугольника
        Point transformedPoints[3];
        for (int i = 0; i < 3; i++) {
            transformedPoints[i] = transformPoint(transform, trianglePoints[i]);
        }
        
        // Отрисовка треугольника
        SDL_SetRenderDrawColor(renderer, 100, 200, 255, 255);
        fillTrianglePoint(renderer, transformedPoints);

        // Контур треугольника
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        for (int i = 0; i < 3; i++) {
            int next = (i + 1) % 3;
            SDL_RenderDrawLine(renderer, 
                transformedPoints[i].x, transformedPoints[i].y,
                transformedPoints[next].x, transformedPoints[next].y);
        }

        // Подсказки управления
        renderText(renderer, font, "Arrows: Move | Q/E: Rotate | Z/X: Scale | Esc: Exit", 10, 10);

        // Обновление экрана
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    
    // Очистка ресурсов
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
    return 0;
}