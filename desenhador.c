#include <SDL3/SDL.h>

#define TARGET_FPS 120

SDL_Renderer *renderer = NULL;
SDL_Window *window = NULL;

typedef enum {
	STARTING,
	PAINTING,
	DRAWING_LINE,
	DRAWING_SQUARE,
	IDLE,
	MOVING,
	SAVING,
	QUIT
} PStatus;

typedef struct {
	SDL_FPoint position;
	SDL_FColor color;
} PCPoint;

typedef struct {
	PCPoint *rects;
	size_t size;
} PCPointsArray;

typedef struct {
	SDL_FPoint start;
	SDL_FPoint end;
	SDL_FColor color;
} PRect;

typedef struct {
	PRect* lines;
	size_t size;
} PRects;

typedef struct {
	SDL_FRect area;
	SDL_FColor color;
} DSquare;

typedef struct {
	DSquare* squares;
	size_t size;
} DSquares;

typedef struct {
	float x, y;
} FPoint;

typedef struct {
	PStatus status;
	PCPointsArray points;
	PRects rects;
	DSquares squares;
	FPoint reference;
	SDL_FColor using_color;
} Painteru;

static SDL_FColor color_palette[9];

static void append_point(PCPointsArray* l, float x, float y, SDL_FColor color){
	SDL_FPoint r = { .x = x, .y = y };
	PCPoint color_point = { .position = r, .color = color };
	/* SDL_Log("color r:%f, g:%f b: %f a:%f \n", color.r, color.g, color.b, color.a); */

	l->size++;
	l->rects = SDL_realloc(l->rects, sizeof(PCPoint)*l->size);
	l->rects[l->size -1] = color_point;
	/* SDL_Log("current size %li \n", l->size); */
}

static void append_line(PRects* l, float sx, float sy, int ex, int ey, SDL_FColor color){
	SDL_FPoint start = {
		.x = sx,
		.y = sy
	};
	SDL_FPoint end = {
		.x = ex,
		.y = ey
	};
	PRect nrect = { .start = start, .end = end, .color = color};
	l->size++;
	l->lines = SDL_realloc(l->lines, sizeof(PRect)*l->size);
	l->lines[l->size -1] = nrect;
	/* SDL_Log("current size %li \n", l->size); */
}

/* static void RenderLines(PRects* rects){ */
/*	for(size_t i = 0; i < rects->size; i++){ */
/*		SDL_RenderLine(renderer, */
/*				rects->lines[i].start.x, rects->lines[i].start.y, */
/*				rects->lines[i].end.x, rects->lines[i].end.y); */
/*	} */
/* } */

static void handle_mouse_button_down(Painteru* p, SDL_MouseButtonEvent mouse){
	/* SDL_Log("status %i\n", p->status); */
	static const int color_selector_x_range = 50*8;
	static const int color_selector_y_range = 50;
	if(mouse.x < color_selector_x_range && mouse.y < color_selector_y_range){
		unsigned int color_index = (int)SDL_floorf(mouse.x / 50);
		/* SDL_Log("color selector pressed, index: %i\n", color_index); */
		p->using_color.r = color_palette[color_index].r;
		p->using_color.g = color_palette[color_index].g;
		p->using_color.b = color_palette[color_index].b;
		p->using_color.a = color_palette[color_index].a;
		return;
	}
	if(SDL_BUTTON_RIGHT == mouse.button){
		if(p->status == DRAWING_LINE){
			append_line(&p->rects, p->reference.x, p->reference.y, mouse.x, mouse.y, p->using_color);
			p->status = IDLE;
		} else {
			p->reference.x = mouse.x;
			p->reference.y = mouse.y;
			p->status = DRAWING_LINE;
		}
	} else {
		p->status = PAINTING;
	}
}

static void draw_dots_with_color(PCPointsArray points){
	for(size_t i = 0; i < points.size; i++){
		SDL_SetRenderDrawColor(renderer,
			points.rects[i].color.r,
			points.rects[i].color.g,
			points.rects[i].color.b,
			points.rects[i].color.a
		);
		SDL_RenderPoint(renderer,
			points.rects[i].position.x,
			points.rects[i].position.y
		);
	}
}

/**
 *
 * black = rgb(69, 71, 90)
 * red = rgb(243, 139, 168)
 * green = rgb(166, 227, 161)
 * yellow = rgb(249, 226, 175)
 * blue = rgb(137, 180, 250)
 * pink = rgb(245, 194, 231)
 * cyan = rgb(148, 226, 213)
 * gray = rgb(166, 173, 200)
 *
 * */
static void init_color_palette(void){

	color_palette[0] = (SDL_FColor) { .r = 69,  .g = 71, .b = 90, .a = SDL_ALPHA_OPAQUE};
	color_palette[1] = (SDL_FColor) { .r = 243, .g = 139, .b = 168, .a = SDL_ALPHA_OPAQUE};
	color_palette[2] = (SDL_FColor) { .r = 166, .g = 227, .b = 161, .a = SDL_ALPHA_OPAQUE};
	color_palette[3] = (SDL_FColor) { .r = 249, .g = 226, .b = 175, .a = SDL_ALPHA_OPAQUE};
	color_palette[4] = (SDL_FColor) { .r = 137, .g = 180, .b = 250, .a = SDL_ALPHA_OPAQUE};
	color_palette[5] = (SDL_FColor) { .r = 245, .g = 194, .b = 231, .a = SDL_ALPHA_OPAQUE};
	color_palette[6] = (SDL_FColor) { .r = 148, .g = 226, .b = 213, .a = SDL_ALPHA_OPAQUE};
	color_palette[7] = (SDL_FColor) { .r = 166, .g = 173, .b = 200, .a = SDL_ALPHA_OPAQUE};
	color_palette[8] = (SDL_FColor) { .r = 255, .g = 255, .b = 255, .a = SDL_ALPHA_OPAQUE};

}

static void draw_color_selector(void){

	SDL_FRect block = { .x = 0, .y = 0, .w = 50, .h = 50 };
	for(short i = 0; i < 9; i++){
		SDL_SetRenderDrawColor(renderer,
				color_palette[i].r,
				color_palette[i].g,
				color_palette[i].b,
				SDL_ALPHA_OPAQUE);
		SDL_RenderFillRect(renderer, &block);
		block.x += 50;
	}
}

static void draw_lines_with_color(PRects rects){
	for(size_t i = 0; i < rects.size; i++){
		SDL_SetRenderDrawColor(renderer,
			rects.lines[i].color.r,
			rects.lines[i].color.g,
			rects.lines[i].color.b,
			rects.lines[i].color.a
		);
		SDL_RenderLine(renderer,
			rects.lines[i].start.x,
			rects.lines[i].start.y,
			rects.lines[i].end.x,
			rects.lines[i].end.y
		);
	}
}

static void draw_squares_with_color(DSquares squares){
	for(size_t i = 0; i < squares.size; i++){
		SDL_SetRenderDrawColor(renderer,
			squares.squares[i].color.r,
			squares.squares[i].color.g,
			squares.squares[i].color.b,
			squares.squares[i].color.a
		);
		SDL_FRect square_begin_drew = {
			.x = squares.squares[i].area.x,
			.y = squares.squares[i].area.y,
			.w = squares.squares[i].area.w,
			.h = squares.squares[i].area.h
		};
		SDL_RenderRect(renderer, &square_begin_drew);
	}
}

float get_lower_float(float a, float b){
	if(a < b){
		return a;
	} else {
		return b;
	}
}

float get_higher_float(float a, float b){
	if(a > b){
		return a;
	} else {
		return b;
	}
}

static void append_square(DSquares* squares, FPoint reference, SDL_FColor color){
	float mx, my;
	SDL_GetMouseState(&mx, &my);

	const float lx = get_lower_float(reference.x, mx);
	const float ly = get_lower_float(reference.y, my);
	const float hx = get_higher_float(reference.x, mx);
	const float hy = get_higher_float(reference.y, my);
	SDL_FRect reference_rect = {
		.x = lx,
		.y = ly,
		.w = hx - lx,
		.h = hy - ly
	};
	DSquare color_square = {
		.area = reference_rect,
		.color = color
	};
	squares->size++;
	squares->squares = SDL_realloc(squares->squares, sizeof(DSquare)*squares->size);
	squares->squares[squares->size -1] = color_square;
}

static void handle_key_pressed(Painteru* p, SDL_KeyboardEvent key){
	switch(key.key){
		case SDLK_S:
			SDL_Log("status: %i\n", p->status);
			if(p->status != DRAWING_SQUARE){
				p->status = DRAWING_SQUARE;
				float mx, my;
				SDL_GetMouseState(&mx, &my);
				p->reference.x = mx;
				p->reference.y = my;
			} else {
				append_square(&p->squares, p->reference, p->using_color);
				p->status = IDLE;
			}
			break;
	}
}

static void draw_guide_square(FPoint p){
			float mx, my;
			SDL_GetMouseState(&mx, &my);
			/* SDL_Log("mouse position %f, %f\n", mx, my); */
			/* SDL_SetRenderDrawColor(renderer, */
			/* 	p.using_color.r, */
			/* 	p.using_color.g, */
			/* 	p.using_color.b, */
			/* 	SDL_ALPHA_OPAQUE */
			/* ); */

			const float lx = get_lower_float(p.x, mx);
			const float ly = get_lower_float(p.y, my);
			const float hx = get_higher_float(p.x, mx);
			const float hy = get_higher_float(p.y, my);
			/* SDL_Log("lower x %f\t lower y%f\n", lx, ly); */
			/* SDL_Log("reference %f, %f\n", (float)p.reference.x, (float)p.reference.y); */
			SDL_FRect reference_rect = {
				.x = lx,
				.y = ly,
				.w = hx - lx,
				.h = hy - ly
				};
			SDL_RenderRect(renderer, &reference_rect);
}

int main(int argc, char** argv){
	(void)(argc);
	(void)(argv);

	if(!SDL_CreateWindowAndRenderer("desenhador", 800, 600, SDL_WINDOW_RESIZABLE, &window, &renderer)){
		SDL_Log("não ci pode crear o ventaninha");
	}

	Painteru p = { .status = STARTING };
	p.points.size = 0;
	init_color_palette();
	p.using_color = (SDL_FColor){ .r = 255, .g = 255, .b = 255, .a = 255 };
	bool done = false;
	SDL_Event event;
	int c_delay = SDL_floorf((1.0f/TARGET_FPS)*1000);
	SDL_Log("delay set to: %i", c_delay);

	while(!done){
		SDL_Delay(c_delay);
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(renderer);
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);

		if(SDL_PollEvent(&event)){
			switch(event.type){
				case SDL_EVENT_QUIT:
					done = true;
					break;

				case SDL_EVENT_MOUSE_BUTTON_DOWN:
					handle_mouse_button_down(&p, event.button);
					break;

				case SDL_EVENT_MOUSE_BUTTON_UP:
					if(p.status == PAINTING){
						append_point(&p.points, event.button.x, event.button.y, p.using_color);
						p.status = IDLE;
					}
					break;

				case SDL_EVENT_KEY_DOWN:
					handle_key_pressed(&p, event.key);
					break;
			}
			if(p.status == PAINTING){
				append_point(&p.points, event.button.x, event.button.y, p.using_color);
			}
		}

		if(p.status == DRAWING_LINE){
			float mx, my;
			SDL_GetMouseState(&mx, &my);
			/* SDL_Log("mouse position %f, %f\n", mx, my); */
			SDL_SetRenderDrawColor(renderer,
				p.using_color.r,
				p.using_color.g,
				p.using_color.b,
				SDL_ALPHA_OPAQUE
			);
			SDL_RenderLine(renderer, mx, my, p.reference.x, p.reference.y);
		}

		if(p.status == DRAWING_SQUARE){
			SDL_SetRenderDrawColor(renderer,
				p.using_color.r,
				p.using_color.g,
				p.using_color.b,
				SDL_ALPHA_OPAQUE
			);
			draw_guide_square(p.reference);
		}
		draw_color_selector();
		draw_dots_with_color(p.points);
		draw_lines_with_color(p.rects);
		draw_squares_with_color(p.squares);
		/* SDL_RenderLine */
		SDL_RenderPresent(renderer);
		}
	SDL_free(p.points.rects);
}
