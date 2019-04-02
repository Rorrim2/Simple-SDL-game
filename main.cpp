#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdlib.h>
#include <time.h>
#include<vector>

using namespace std;

float clamp(float val, float _min, float _max) {
	if (val < _min) {
		return _min;
	}
	if (val > _max) {
		return _max;
	}
	return val;
}
struct prostokat {
	prostokat (float w, float s, SDL_Point p): wysokosc(w), szerokosc(s), pozycja(p){}
	float wysokosc;
	float szerokosc;
	SDL_Point pozycja = {0, 0};
};

bool czy_prostokaty_nachodza (prostokat pierwszy, prostokat drugi) {
	return !(pierwszy.pozycja.x + pierwszy.szerokosc < drugi.pozycja.x || pierwszy.pozycja.y + pierwszy.wysokosc < drugi.pozycja.y ||
		pierwszy.pozycja.x > drugi.pozycja.x + drugi.szerokosc || pierwszy.pozycja.y > drugi.pozycja.y + drugi.wysokosc);
}
//tlo zielone 265 liczone od gory
struct grafika {
	SDL_Surface* surface;
	SDL_Texture* texture;
	SDL_Rect src = {0, 0, 0, 0};
	SDL_Rect dst = {0, 0, 0, 0};
};

struct particle {
	SDL_Point pozycja = {0, 0};
	SDL_Point szybkosc = {0, 0};
	grafika** gfx;
	double rotacja = 0;
	int LifeTime = 0;
};

struct animacja{
	grafika** klatka;
	int ilosc_klatek = 0;
	int obecna_klatka = 0;
	float czas_klatki = 0;
	float poprzedni_czas_klatki = 0;
};


grafika* load_grafika(string plik, SDL_Renderer* renderer) {
	grafika* temp = new grafika;

	SDL_Surface* obrazek = IMG_Load(plik.c_str());
	if (obrazek == NULL) {
		cout << "Could not load image: " << SDL_GetError();
		delete temp;
		return NULL;
	}
	SDL_Texture* tekstura = SDL_CreateTextureFromSurface(renderer, obrazek);
	if (tekstura == NULL) {
		cout << "Could not load texture: " << SDL_GetError();
		delete temp;
		return NULL;
	}

	temp->surface = obrazek;
	temp->texture = tekstura;
	temp->src.w = obrazek->w;
	temp->src.h = obrazek->h;
	temp->dst.w = obrazek->w;
	temp->dst.h = obrazek->h;

	return temp;
}

void delete_grafika(grafika* gf) {
	SDL_DestroyTexture(gf->texture);
	SDL_FreeSurface(gf->surface);
	delete gf;
}


int main(int argc, char** argv) {
	srand(time(0));
	float time;
	//float time_breath;
	float lastTime = 0;
	//float lastTime_breath = 0;
	//float time_walk;
	//float lastTime_walk = 0;

	float delta_t = 0;


	vector<particle*> lista;

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
		return 1;
	}

	SDL_Window* window = NULL;
	SDL_Renderer* renderer = NULL;

	const int sizeX = 640;
	const int sizeY = 480;

	window = SDL_CreateWindow(
        "An SDL2 window",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
		sizeX,
        sizeY,
        0
    );
	renderer = SDL_CreateRenderer(window, -1, 0);

	if (window == NULL) {
		cout << "Could not create window: " << SDL_GetError();
		return 1;
	}

	animacja ludzik[3];

	grafika* tlo;
	tlo = load_grafika("./gfx/tlo.png", renderer);
	//Owca
	animacja owca[1];

	owca[0].klatka = new grafika*[2];
	owca[0].klatka[0] = load_grafika("./gfx/owca_1.png", renderer);
	owca[0].klatka[1] = load_grafika("./gfx/owca_down.png", renderer);
	owca[0].ilosc_klatek = 2;
	owca[0].czas_klatki = 100;
	//Oddychanie
	ludzik[0].klatka = new grafika*[2];
	ludzik[0].klatka[0] = load_grafika("./gfx/front_0.png", renderer);
	ludzik[0].klatka[1] = load_grafika("./gfx/front_1.png", renderer);
	ludzik[0].czas_klatki = 350;
	ludzik[0].ilosc_klatek = 2;
	//Chodzenie
	ludzik[1].klatka = new grafika*[3];
	ludzik[1].klatka[0] = load_grafika("./gfx/side_move_0.png", renderer);
	ludzik[1].klatka[1] = load_grafika("./gfx/side_move_1.png", renderer);
	ludzik[1].klatka[2] = load_grafika("./gfx/side_move_2.png", renderer);
	ludzik[1].czas_klatki = 100;
	ludzik[1].ilosc_klatek = 3;
	//Zabijanie
	ludzik[2].klatka = new grafika*[3];
	ludzik[2].klatka[0] = load_grafika("./gfx/sword_0.png", renderer);
	ludzik[2].klatka[1] = load_grafika("./gfx/sword_1.png", renderer);
	ludzik[2].klatka[2] = load_grafika("./gfx/sword_up.png", renderer);
	ludzik[2].czas_klatki = 200;
	ludzik[2].ilosc_klatek = 3;

	int ludzik_obecna_animacja = 0;
	bool ludzik_w_prawo = true;
	int atak = 0;
	bool smierc = false;
	bool martwa = false;
	int znikanie = 0;
	SDL_Point pos = {100, 0};
	SDL_Point pos_owcy = {100, 280};

	//SDL_Rect	src = {0, 0, obrazek->w, obrazek->h};
	//SDL_Rect	dst time_breath= {0, 0, obrazek->w / 2, obrazek->h / 2};

	SDL_Event event;
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	//int oldX;
	//int oldY;
	int ostatni_czas = SDL_GetTicks();

	bool forceQuit = false;
	bool blisko_owcy = false;
	for(; event.type != SDL_QUIT && forceQuit != true; SDL_PollEvent(&event)) {

		delta_t = -(ostatni_czas - SDL_GetTicks()) / 1000.0;

		ostatni_czas = SDL_GetTicks();

		int obecna = ludzik[ludzik_obecna_animacja].obecna_klatka;
		int obecna_owca = owca[0].obecna_klatka;


		if (smierc == true) {
			obecna_owca = 1;
			martwa = true;
		}
		else if (smierc == false) {
			obecna_owca = 0;
			martwa = false;
		}
		grafika* obecna_klatka = ludzik[ludzik_obecna_animacja].klatka[obecna];
		grafika* obecna_klatka_owcy = owca[0].klatka[obecna_owca];

		obecna_klatka->dst.x = pos.x;
		obecna_klatka->dst.y = pos.y;

		obecna_klatka_owcy->dst.x = pos_owcy.x;
		obecna_klatka_owcy->dst.y = pos_owcy.y;

		SDL_RenderClear(renderer);
		SDL_RenderCopy(
			renderer,
			tlo->texture,
			&tlo->src,
			&tlo->dst
				);
		int tempH = pos.y + obecna_klatka_owcy->dst.h / 2;
		if (pos_owcy.y < tempH) {
			SDL_RenderCopyEx(
				renderer,
				obecna_klatka_owcy->texture,
				&obecna_klatka_owcy->src,
				&obecna_klatka_owcy->dst,
				0,
				NULL,
				SDL_FLIP_NONE
			);
		}
		SDL_RenderCopyEx(
			renderer,
			obecna_klatka->texture,
			&obecna_klatka->src,
			&obecna_klatka->dst,
			0,
			NULL,
			ludzik_w_prawo? SDL_FLIP_HORIZONTAL: SDL_FLIP_NONE
		);
		if (pos_owcy.y >= tempH) {
			SDL_RenderCopyEx(
				renderer,
				obecna_klatka_owcy->texture,
				&obecna_klatka_owcy->src,
				&obecna_klatka_owcy->dst,
				0,
				NULL,
				SDL_FLIP_NONE
			);
		}


		SDL_RenderPresent(renderer);

		float last = ludzik[ludzik_obecna_animacja].poprzedni_czas_klatki;
		float delay = ludzik[ludzik_obecna_animacja].czas_klatki;
		if (SDL_GetTicks() - delay > last) {
			ludzik[ludzik_obecna_animacja].obecna_klatka += 1;
			if (ludzik[ludzik_obecna_animacja].obecna_klatka >= ludzik[ludzik_obecna_animacja].ilosc_klatek) {
				ludzik[ludzik_obecna_animacja].obecna_klatka = 0;
			}
			ludzik[ludzik_obecna_animacja].poprzedni_czas_klatki = SDL_GetTicks();
		}

		ludzik_obecna_animacja = 0;

		SDL_Point oldPos = {pos.x, pos.y};
		//SDL_Point oldPos_owcy = {pos_owcy.x, pos_owcy.y};
		switch(event.type) {
			case(SDL_KEYDOWN): {
				if (event.key.keysym.scancode == SDL_SCANCODE_DOWN) {
					pos.y += 1;
					ludzik_obecna_animacja = 1;
				} else if (event.key.keysym.scancode == SDL_SCANCODE_UP) {
					pos.y -= 1;
					ludzik_obecna_animacja = 1;
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
					pos.x += 1;
					ludzik_obecna_animacja = 1;
					ludzik_w_prawo = true;
				} else if (event.key.keysym.scancode == SDL_SCANCODE_LEFT) {
					pos.x -= 1;
					ludzik_obecna_animacja = 1;
					ludzik_w_prawo = false;
				}

				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
					 atak = SDL_GetTicks() + 600;
				}

				time = SDL_GetTicks();
				if (time > lastTime + 2) {
					lastTime = time;
				} else {
					pos.x = oldPos.x;
					pos.y = oldPos.y;
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
					forceQuit = true;
				}
			}
		}
		prostokat obraz_postac(obecna_klatka ->dst.h, obecna_klatka ->dst.w, pos);
		prostokat obraz_owcy (obecna_klatka_owcy -> dst.h, obecna_klatka_owcy -> dst.w, pos_owcy);



		if (atak > SDL_GetTicks()) {
			ludzik_obecna_animacja = 2;
			if (czy_prostokaty_nachodza(obraz_postac, obraz_owcy) == true) {
				//pos.y = oldPos.y;
				//pos.x = oldPos.x;
				blisko_owcy = true;
			}
			if (blisko_owcy == true) {
				smierc = true;
				znikanie = SDL_GetTicks() + 1000;
				blisko_owcy = false;
			}
		} else {
			if (ludzik_obecna_animacja == 2) {
				ludzik_obecna_animacja = 0;
			}
		}
		if (znikanie < SDL_GetTicks() && martwa == true) {
			do {
				obraz_owcy.pozycja.y = (rand()%84 + 306);
				obraz_owcy.pozycja.x = (rand()%542);
			}while(czy_prostokaty_nachodza(obraz_postac, obraz_owcy) == true);
			pos_owcy.x = obraz_owcy.pozycja.x;
			pos_owcy.y = obraz_owcy.pozycja.y;
			smierc = false;
		}


		
		pos.y	= clamp(pos.y, (sizeY / 2) - 100, sizeY - obecna_klatka->dst.h);
		pos.x	= clamp(pos.x, 0, sizeX - obecna_klatka->dst.w);
	}


	delete_grafika(ludzik[0].klatka[0]);
	delete_grafika(ludzik[0].klatka[1]);
	delete_grafika(ludzik[1].klatka[0]);
	delete_grafika(ludzik[1].klatka[1]);
	delete_grafika(ludzik[1].klatka[2]);
	delete_grafika(owca[0].klatka[0]);
	delete_grafika(owca[0].klatka[1]);

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}
