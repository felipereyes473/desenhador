#include <SDL3/SDL.h>

#define TARGET_FPS 120
#define DOT_LINE_WIDTH 3
#define COLOR_DEFAULT (SDL_FColor){ .r = 255, .g = 255, .b = 255, .a = SDL_ALPHA_OPAQUE };
#define COLOR_PURE_BLACK (SDL_FColor){ .r = 0, .g = 0, .b = 0, .a = SDL_ALPHA_OPAQUE };
#define COLOR_TRANSPARENT_BG (SDL_FColor) { .r = 0, .g = 0, .b = 0, .a = SDL_ALPHA_TRANSPARENT}

#define DEFAULT_ARRAY_CAPACITY 350

#define BLACK_COLOR (SDL_FColor) { .r = 69,  .g = 71, .b = 90, .a = SDL_ALPHA_OPAQUE};
#define RED_COLOR (SDL_FColor) { .r = 243, .g = 139, .b = 168, .a = SDL_ALPHA_OPAQUE};
#define GREEN_COLOR (SDL_FColor) { .r = 166, .g = 227, .b = 161, .a = SDL_ALPHA_OPAQUE};
#define YELLOW_COLOR (SDL_FColor) { .r = 249, .g = 226, .b = 175, .a = SDL_ALPHA_OPAQUE};
#define BLUE_COLOR (SDL_FColor) { .r = 137, .g = 180, .b = 250, .a = SDL_ALPHA_OPAQUE};
#define PINK_COLOR (SDL_FColor) { .r = 245, .g = 194, .b = 231, .a = SDL_ALPHA_OPAQUE};
#define CYAN_COLOR (SDL_FColor) { .r = 148, .g = 226, .b = 213, .a = SDL_ALPHA_OPAQUE};
#define GRAY_COLOR (SDL_FColor) { .r = 166, .g = 173, .b = 200, .a = SDL_ALPHA_OPAQUE};
#define WHITE_COLOR (SDL_FColor) { .r = 239, .g = 241, .b = 245, .a = SDL_ALPHA_OPAQUE};

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

typedef enum {
	DOT,
	RECT,
	SQUARE
} DrawingElementType;

typedef struct {
	DrawingElementType type;
	SDL_FPoint position;
	SDL_FColor color;
} PCPoint;

typedef struct {
	DrawingElementType type;
	SDL_FPoint start;
	SDL_FPoint end;
	SDL_FColor color;
} PRect;

typedef struct {
	DrawingElementType type;
	SDL_FRect area;
	SDL_FColor color;
} DSquare;

typedef union {
	DrawingElementType type;
	PCPoint point;
	DSquare square;
	PRect rect;
} DrawingObj;

typedef struct {
	DrawingObj *items;
	size_t size;
	size_t size_top;
	size_t capacity;
} DOArray;

typedef struct {
	PStatus status;
	DOArray objects;
	SDL_FPoint reference;
	SDL_FColor foreground;
	SDL_FColor background;
	bool is_light_mode;
	bool display_help_message;
	bool transparent;
} Painteru;

static SDL_FColor color_palette[9];

DrawingObj get_point_drawing_obj(float x, float y){

	PCPoint r = { .position = { .x = x, .y = y }};
	DrawingObj n_obj = { .point = r };
	return n_obj;
}

DrawingObj get_line_drawing_obj(float sx, float sy, int ex, int ey){

	SDL_FPoint start = { .x = sx, .y = sy };
	SDL_FPoint end = { .x = ex, .y = ey };

	PRect nrect = { .start = start, .end = end};
	DrawingObj n_obj = { .rect = nrect};
	return n_obj;
}

static void append_drawing_object(DOArray* list, DrawingObj new_element){
	list->size++;
	list->size_top = list->size;
	if(list->size >= list->capacity){
		list->capacity = SDL_floorf(list->capacity*1.25);
		list->items = (DrawingObj*)SDL_realloc(list->items, sizeof(DrawingObj)*list->capacity);
	}
	list->items[list->size -1] = new_element;
}

/* \deprecated code */
/* static void RenderLines(PRects* rects){ */
/*	for(size_t i = 0; i < rects->size; i++){ */
/*		SDL_RenderLine(renderer, */
/*				rects->lines[i].start.x, rects->lines[i].start.y, */
/*				rects->lines[i].end.x, rects->lines[i].end.y); */
/*	} */
/* } */

static void handle_mouse_button_down(Painteru* p, SDL_MouseButtonEvent mouse){
	/* SDL_Log("status %i\n", p->status); */
	static const int color_selector_x_range = 50*9;
	static const int color_selector_y_range = 50;
	if(mouse.x < color_selector_x_range && mouse.y < color_selector_y_range){
		unsigned int color_index = (int)SDL_floorf(mouse.x / 50);
		/* SDL_Log("color selector pressed, index: %i\n", color_index); */
		p->foreground.r = color_palette[color_index].r;
		p->foreground.g = color_palette[color_index].g;
		p->foreground.b = color_palette[color_index].b;
		p->foreground.a = color_palette[color_index].a;
		return;
	}
	if(SDL_BUTTON_RIGHT == mouse.button){
		if(p->status == DRAWING_LINE){
			DrawingObj n_line = get_line_drawing_obj(p->reference.x, p->reference.y, mouse.x, mouse.y);
			n_line.rect.color = p->foreground;
			n_line.type = RECT;
			append_drawing_object(&p->objects, n_line);
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

static void draw_circle_at(SDL_FPoint center, int radius){
	for(int x = 0 - radius; x < radius*2; x++){
		for(int y = 0 - radius; y < radius*2; y++){
			if(x*x + y*y <= radius*radius){
				SDL_RenderPoint(renderer, center.x+x, center.y+y);
			}
		}
	}
}

static void draw_dot_with_color(PCPoint point){
	SDL_SetRenderDrawColor(renderer,
		point.color.r,
		point.color.g,
		point.color.b,
		point.color.a
	);
	draw_circle_at(point.position, DOT_LINE_WIDTH);
	SDL_RenderPoint(renderer,
		point.position.x,
		point.position.y
	);
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

	color_palette[0] = BLACK_COLOR;
	color_palette[1] = RED_COLOR;
	color_palette[2] = GREEN_COLOR;
	color_palette[3] = YELLOW_COLOR;
	color_palette[4] = BLUE_COLOR;
	color_palette[5] = PINK_COLOR;
	color_palette[6] = CYAN_COLOR;
	color_palette[7] = GRAY_COLOR;
	color_palette[8] = WHITE_COLOR;

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

static void draw_line_with_color(PRect rect){
	SDL_SetRenderDrawColor(renderer,
		rect.color.r,
		rect.color.g,
		rect.color.b,
		rect.color.a
	);
	SDL_RenderLine(renderer,
		rect.start.x,
		rect.start.y,
		rect.end.x,
		rect.end.y
	);
}

static void draw_square_with_color(DSquare square){
	SDL_SetRenderDrawColor(renderer,
		square.color.r,
		square.color.g,
		square.color.b,
		square.color.a
	);
	SDL_FRect square_begin_drew = {
		.x = square.area.x,
		.y = square.area.y,
		.w = square.area.w,
		.h = square.area.h
	};
	SDL_RenderRect(renderer, &square_begin_drew);
}

static void draw_elements(DOArray elements){
	for(size_t i = 0; i < elements.size; i++){
		switch(elements.items[i].type){
			case DOT:
				draw_dot_with_color(elements.items[i].point);
				break;
			case RECT:
				draw_line_with_color(elements.items[i].rect);
				break;
			case SQUARE:
				draw_square_with_color(elements.items[i].square);
				break;
		}
	}
}

DrawingObj get_square_drawing_obj(SDL_FPoint reference){
	float mx, my;
	SDL_GetMouseState(&mx, &my);

	/* const float lx = get_lower_float(reference.x, mx); */
	const float lx = reference.x > mx ? reference.x : mx;
	const float ly = reference.y > my ? reference.y : my;
	const float hx = reference.x < mx ? reference.x : mx;
	const float hy = reference.y < my ? reference.y : my;

	SDL_FRect reference_rect = {
		.x = lx,
		.y = ly,
		.w = hx - lx,
		.h = hy - ly
	};
	DrawingObj n_square = { .square = { .area = reference_rect }};
	return n_square;
}

static void set_background_color(Painteru* p){
	if(p->is_light_mode){
		p->background = COLOR_DEFAULT;
		p->foreground = BLACK_COLOR;
	} else {
		p->foreground = WHITE_COLOR;
		p->background = COLOR_PURE_BLACK;
	}
	if(p->transparent){
		/* SDL_Log("transparent bg"); */
		p->background = COLOR_TRANSPARENT_BG;
	}
}

static void toggle_theme_mode(Painteru* p){
	p->is_light_mode = !p->is_light_mode;
	set_background_color(p);
}

static void handle_key_pressed(Painteru* p, SDL_KeyboardEvent key){
	switch(key.key){
		case SDLK_S:
			if(p->status != DRAWING_SQUARE){
				p->status = DRAWING_SQUARE;
				float mx, my;
				SDL_GetMouseState(&mx, &my);
				p->reference =(SDL_FPoint) { .x = mx, .y = my};
			} else {
				DrawingObj n_square = get_square_drawing_obj(p->reference);
				n_square.square.color = p->foreground;
				n_square.type = SQUARE;
				append_drawing_object(&p->objects, n_square);
				p->status = IDLE;
			}
			break;
		case SDLK_U:
			if(p->objects.size >= 1){
				p->objects.size--;
			}
			break;
		case SDLK_R:
			if(p->objects.size_top > p->objects.size){
				p->objects.size++;
			}
			break;
		case SDLK_M:
			toggle_theme_mode(p);
			break;
	}
}

static void draw_guide_square(SDL_FPoint p){
	float mx, my;
	SDL_GetMouseState(&mx, &my);

	const float lx = p.x > mx ? p.x : mx;
	const float ly = p.y > my ? p.y : my;
	const float hx = p.x < mx ? p.x : mx;
	const float hy = p.y < my ? p.y : my;

	SDL_FRect reference_rect = {
		.x = lx,
		.y = ly,
		.w = hx - lx,
		.h = hy - ly
	};
	SDL_RenderRect(renderer, &reference_rect);
}

static void handle_flags(Painteru* p, int count, char** args){
	for(int i = 0; i < count; i++){
		if(SDL_strcmp(args[i], "-?") == 0){
			p->display_help_message = true;
		}
		if(SDL_strcmp(args[i], "-h") == 0){
			p->display_help_message = true;
		}
		if(SDL_strcmp(args[i], "--help") == 0){
			p->display_help_message = true;
		}
		if(SDL_strcmp(args[i], "-l") == 0){
			p->is_light_mode = true;
		}
		if(SDL_strcmp(args[i], "--light-mode") == 0){
			p->is_light_mode = true;
		}
		if(SDL_strcmp(args[i], "--transparent") == 0){
			p->transparent = true;
		}
		if(SDL_strcmp(args[i], "-t") == 0){
			p->transparent = true;
		}
	}
}

static void create_window(void){
	if(!SDL_CreateWindowAndRenderer("desenhador", 800, 600, SDL_WINDOW_RESIZABLE | SDL_WINDOW_TRANSPARENT, &window, &renderer)){
		SDL_Log("não ci pode crear o ventaninha");
		SDL_Quit();
	}
}

static void clear_background(SDL_FColor background){
	SDL_SetRenderDrawColor(renderer, background.r, background.g, background.b, background.a);
	SDL_RenderClear(renderer);
	/* SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE); */
}

static void draw_reference_guides(PStatus status, SDL_FPoint reference, SDL_FColor foreground){
	if(status == DRAWING_LINE){
		float mx, my;
		SDL_GetMouseState(&mx, &my);
		SDL_SetRenderDrawColor(renderer,
				foreground.r,
				foreground.g,
				foreground.b,
				SDL_ALPHA_OPAQUE
				);
		SDL_RenderLine(renderer, mx, my, reference.x, reference.y);
	}

	if(status == DRAWING_SQUARE){
		SDL_SetRenderDrawColor(renderer,
			foreground.r,
			foreground.g,
			foreground.b,
			SDL_ALPHA_OPAQUE
		);
		draw_guide_square(reference);
	}
}

/*
 * --help or -h for help message
 * */
void show_help_message(void){
	SDL_Log("Desenhador ( .-.)");
	SDL_Log("Usage: desenhador [OPTION...]");
	SDL_Log("\t-t\t--transparent\t\tset this to transparent background");
	SDL_Log("\t-l\t--light-mode\t\tuse this to white background");
	SDL_Log("\t-h\t--help\t-?\t\tdisplay this help message");
}

int main(int argc, char** argv){

	create_window();
	init_color_palette();

	Painteru p = { .status = STARTING, .transparent = false};
	p.objects.capacity = DEFAULT_ARRAY_CAPACITY;
	p.objects.items = (DrawingObj*)SDL_malloc(sizeof(DrawingObj)*DEFAULT_ARRAY_CAPACITY);
	handle_flags(&p, argc, argv);
	if(p.display_help_message){
		show_help_message();
		return 0;
	}
	set_background_color(&p);

	bool done = false;
	int c_delay = SDL_floorf((1.0f/TARGET_FPS)*1000);
	SDL_Log("[INFO] delay set to: %i", c_delay);
	SDL_Event event;

	while(!done){
		SDL_Delay(c_delay);

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
						DrawingObj n_point = get_point_drawing_obj(event.button.x, event.button.y);
						n_point.point.color = p.foreground;
						n_point.type = DOT;
						append_drawing_object(&p.objects, n_point);
						p.status = IDLE;
					}
					break;
				case SDL_EVENT_KEY_DOWN:
					handle_key_pressed(&p, event.key);
					break;

				case SDL_EVENT_MOUSE_MOTION:
					if(p.status == PAINTING){
						DrawingObj n_point = get_point_drawing_obj(event.button.x, event.button.y);
						n_point.point.color = p.foreground;
						n_point.type = DOT;
						append_drawing_object(&p.objects, n_point);
					} else {
						if(IDLE == p.status) continue;
					}
					break;

				default:
					/* continue; */
					break;
			}

			clear_background(p.background);
			draw_reference_guides(p.status, p.reference, p.foreground);
			draw_color_selector();
			draw_elements(p.objects);
			SDL_RenderPresent(renderer);
		}
		/* draw_dots_with_color(p.points); */
		/* draw_lines_with_color(p.rects); */
		/* draw_squares_with_color(p.squares); */
		/* SDL_RenderLine */
	}
	SDL_free(p.objects.items);
}
