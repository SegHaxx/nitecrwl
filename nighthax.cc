#include <osbind.h>

#include "aes.h"
#include "vdi.h"

//#include <stdio.h>
//using namespace std;

typedef struct{
	int16_t x1;
	int16_t y1;
	int16_t x2;
	int16_t y2;
} VRECT;

class board_model{
	public:
		board_model(){reset();};
		void reset();
		void worm_generate_moves(void);
		void next_worm(void);
		bool worm_direction_is_valid_move(int16_t);
		void worm_reset_brain(int16_t);
		void worm_tick(int16_t,int16_t);
		void capture(int16_t,int16_t);
		void worm_rotate(int16_t);

		void set_runmode(int16_t mode){runmode=mode;};
		int16_t get_runmode(){return runmode;};
		void set_worm_pos(int16_t worm,int16_t pos){worm_position[worm]=pos;};
		int16_t get_worm_pos(int16_t worm){return worm_position[worm];};
		void set_worm_mode(int16_t worm,int16_t mode){worm_mode[worm]=mode;};
		void set_current_worm(int16_t worm){current_worm=worm;};
		int16_t get_current_worm(){return current_worm;};
		int16_t get_worm_score(int16_t worm){return worm_score[worm];};

		int16_t current_worm_valid_move_count;
		int16_t current_worm_direction;
	private:
		int16_t runmode;
		int16_t current_worm;
		int16_t worm_score[4];
		int16_t worm_position[4];
		int16_t worm_mode[4];
		int16_t current_worm_valid_moves[6];
		int16_t worm_brain[4][64];
	public:
		unsigned char state[450][4];
		unsigned char state_all[450];
};

class board_render{
	public:
		int16_t vdi;
		int16_t ev_mtcount;
		void set_vdi(int16_t v,int16_t c){vdi=v;colordepth=c;};
		void set_geometry(int16_t,int16_t,int16_t,int16_t);
		void set_worm_mode(int16_t,int16_t);
		void set_runmode(int16_t);
		void set_speed(int16_t);
		void reset();
		void redraw();
		void tick();
		bool key_event(int16_t);
		void worm_rotate(int16_t);
	private:
		VRECT vrect;
		int16_t colordepth;
		int16_t pad_x,pad_y;
		int16_t cell_w,cell_h;
		int16_t score_x,score_y;
		int16_t score_w,score_h;
		int16_t score_pad;
		int16_t score_textx,score_texty;
		int16_t worm_flash;
		void worm_tick(int16_t,int16_t);
		void capture(int16_t,int16_t);
		void worm_draw(int16_t,int16_t,int16_t);
		void worm_draw_get_line(VRECT*,int16_t,int16_t);
		void worm_draw_vdi(int16_t,void (*draw)(int16_t));
		void worm_draw_segment(VRECT*,int16_t);
		void worm_draw_box(VRECT*,int16_t);
		void worm_draw_score(int16_t);
		void position_to_xy(int16_t,int16_t&,int16_t&);
	public:
		board_model model;
};

static OBJECT* menu;
static OBJECT* about_dialog;

static bool soundflag;

inline void board_render::set_runmode(int16_t mode){
	model.set_runmode(mode);
	int16_t flag=0;
	switch(mode){
		case 0:
			break;
		case 1:
		case 2:
			flag=!flag;
			break;
		default:
			return;
	}
	menu_icheck(menu,0x19,!flag);
	menu_icheck(menu,0x1a,flag);
}

inline void board_model::worm_generate_moves(void){
	int16_t direction = worm_brain[current_worm][state_all[worm_position[current_worm]]];
	if(direction){	// go in learned direction
		current_worm_valid_move_count = 1;
		current_worm_valid_moves[0] = direction;
		current_worm_direction = direction;
		return;
	}
	// enumerate valid moves
	current_worm_valid_move_count = 0;
	int16_t state = state_all[worm_position[current_worm]];
	for(int16_t direction=1; direction<7; ++direction){
		if(state % 2 == 0){
			current_worm_valid_moves[current_worm_valid_move_count] = direction;
			++current_worm_valid_move_count;
		}
		state /= 2;
	}
	// pick a random valid move
	if(current_worm_valid_move_count){
		int16_t rnd = Random() % current_worm_valid_move_count;
		current_worm_direction = current_worm_valid_moves[rnd];
	}
}

inline void board_model::next_worm(void){
	int16_t start_worm = current_worm;
	while ((current_worm = (current_worm + 1) % 4,
				worm_mode[current_worm] < 1 ||
				(state_all[worm_position[current_worm]] == 0x3f))) {
		if (current_worm == start_worm) { // all worms dead
			set_runmode(2);
			current_worm_valid_move_count = 0;
			return;
		}
	}
	worm_generate_moves();
	if (worm_mode[current_worm] != 1) {
		return;
	}
	if(current_worm_valid_move_count < 2){
		return;
	}
	set_runmode(0);
}

inline void board_render::set_worm_mode(int16_t worm,int16_t mode){
	model.set_worm_mode(worm,mode);
	static char worm_menu_indexes[4]={0x22,0x2a,0x32,0x3a};
	int16_t me_citem = worm_menu_indexes[worm];
	menu_icheck(menu,me_citem  ,mode==1);
	menu_icheck(menu,me_citem+3,mode==2);
	menu_icheck(menu,me_citem+6,mode==0);
}

inline void board_model::worm_reset_brain(int16_t worm){
	for(int16_t i=0; i<64; ++i){
		worm_brain[worm][i] = 0;
	}
	worm_brain[worm][31] = 6;
	worm_brain[worm][47] = 5;
	worm_brain[worm][55] = 4;
	worm_brain[worm][59] = 3;
	worm_brain[worm][61] = 2;
	worm_brain[worm][62] = 1;
	worm_brain[worm][63] = 7;
}

inline void board_render::set_speed(int16_t speed){
	static int16_t speedtbl[]={50,150,500};
	for(int16_t i=0; i<3; ++i){
		int16_t flag=(speed==i);
		if(flag){
			ev_mtcount=speedtbl[i];
		}
		menu_icheck(menu,i+0x1c,flag);
	}
}

inline void board_render::worm_draw_vdi(int16_t worm,void (*draw)(int16_t)){
	vsl_width(vdi,colordepth<4||vrect.x2>319?3:1);
	if(colordepth>=16){
		vsl_ends(vdi,2,2);
		draw(worm+2);
		return;
	}
	vsl_ends(vdi,0,0);
	if(colordepth>=4){
		int16_t color=3;
		switch(worm){
			case -2:
				draw(0);
				break;
			case -1:
				draw(1);
				break;
			case 0:
				color=2;
			case 1:
				draw(color);
				break;
			case 2:
				color=2;
			case 3:
				draw(1);
				vsl_width(vdi,1);
				vsl_udsty(vdi,0xaaaa);
				vsl_type(vdi,7);
				vswr_mode(vdi,2);
				draw(color);
				vswr_mode(vdi,1);
				vsl_type(vdi,1);
		}
		return;
	}
	int16_t color=0;
	switch(worm){
		case -2:
			draw(0);
			break;
		case -1:
			draw(1);
			break;
		case 1:
			color=!color;
		case 2:
			draw(!color);
			vsl_width(vdi,1);
		case 0:
			draw(color);
			break;
		case 3:
			draw(1);
			vsl_width(vdi,1);
			vsl_udsty(vdi,0x6666);
			vsl_type(vdi,7);
			vswr_mode(vdi,3);
			draw(1);
			vswr_mode(vdi,1);
			vsl_type(vdi,1);
	}
}

typedef struct{
    template<typename Tret, typename T>
    static Tret lambda_ptr_exec(int16_t data) {
        return (Tret) (*(T*)fn<T>())(data);
    }

    template<typename Tret = void, typename Tfp = Tret(*)(int16_t), typename T>
    static Tfp ptr(T& t) {
        fn<T>(&t);
        return (Tfp) lambda_ptr_exec<Tret, T>;
    }

    template<typename T>
    static void* fn(void* new_fn = nullptr) {
        static void* fn;
        if (new_fn != nullptr)
            fn = new_fn;
        return fn;
    }
} Lambda;

inline void board_render::worm_draw_segment(VRECT* line,int16_t worm){
	auto draw_line = [=] (int16_t color) {
		vsl_color(vdi,color);
		v_pline(vdi,2,(int16_t*)line);
	};
	worm_draw_vdi(worm,Lambda::ptr(draw_line));
}

inline void board_render::worm_draw_box(VRECT* rect,int16_t worm){
	auto draw_box = [=] (int16_t color) {
		vsl_color(vdi,color);
		int16_t ptsin[4];
		ptsin[0]=rect->x1;
		ptsin[1]=rect->y1;
		ptsin[2]=rect->x2;
		ptsin[3]=rect->y1;
		v_pline(vdi,2,ptsin);
		ptsin[0]=rect->x2;
		ptsin[1]=rect->y2;
		//ptsin[2]=rect.x2;
		//ptsin[3]=rect.y1;
		v_pline(vdi,2,ptsin);
		//ptsin[0]=rect.x2;
		//ptsin[1]=rect.y2;
		ptsin[2]=rect->x1;
		ptsin[3]=rect->y2;
		v_pline(vdi,2,ptsin);
		ptsin[0]=rect->x1;
		ptsin[1]=rect->y1;
		//ptsin[2]=rect.x1;
		//ptsin[3]=rect.y2;
		v_pline(vdi,2,ptsin);
	};
	worm_draw_vdi(worm,Lambda::ptr(draw_box));
}

inline void board_render::position_to_xy(int16_t pos,int16_t& x,int16_t& y){
	x = pos % 45;
	if (x > 22) {
		x -= 23;
	}
	--x;
	y = (pos / 45) * 2;
	if (pos % 45 > 22) {
		++y;
	}
}

inline void board_render::worm_draw_get_line(
		VRECT* line, int16_t pos, int16_t direction){
	int16_t x,y;
	position_to_xy(pos,x,y);
	line->x1 = x * cell_w*4 + pad_x;
	line->y1 = y * cell_h*2 + pad_y;
	if (y % 2 == 1) {
		line->x1 += cell_w*2;
	}
	static char direction_table[6][2]={
		{ 2, 0},
		{ 1,-1},
		{-1,-1},
		{-2, 0},
		{-1, 1},
		{ 1, 1}};
	line->x2 = line->x1 + cell_w * direction_table[direction][0];
	line->y2 = line->y1 + cell_h * direction_table[direction][1];
}

inline void board_render::worm_draw(
		int16_t pos, int16_t direction, int16_t worm){
	VRECT line;
	worm_draw_get_line(&line,pos,direction);
	worm_draw_segment(&line,worm);
}

inline void board_render::worm_draw_score(int16_t worm){
	int16_t score = model.get_worm_score(worm);
	char str[4];
	str[3]='\0';
	for(int16_t i=2; i>=0; --i){
		str[i] = score%10+'0';
		if(score==0){
			str[i]=' ';
		}
		score /= 10;
	}
	v_gtext(vdi,
			score_pad * worm + score_textx,
			score_texty, str);
}

static void set_soundflag(bool flag){
	soundflag=flag;
	menu_icheck(menu,0x20,flag);
}

// pump a list of byte pairs into the soundchip
// end on -1
static void sound_play(unsigned char* tbl){
	if(!soundflag) return;
	while(*tbl!=0xff){
		Giaccess(*tbl++,*tbl++);
	}		
}

static void sound_play_win_chord(void){
	unsigned char chord[]={
		0xdd,0x80,
		0x01,0x81,
		0x7b,0x82,
		0x01,0x83,
		0x3e,0x84,
		0x01,0x85,
		0xf8,0x87,
		0x10,0x88,
		0x10,0x89,
		0x10,0x8a,
		0x00,0x8b,
		0x14,0x8c,
		0x00,0x8d,
		0xff};
	sound_play(chord);
}

static void sound_play_note(int16_t note){
	unsigned char notetbl[6][5]={
		{0xdd,0x80,1,0x81,0xff},
		{0x7b,0x80,1,0x81,0xff},
		{0x3e,0x80,1,0x81,0xff},
		{0xee,0x80,0,0x81,0xff},
		{0xbd,0x80,0,0x81,0xff},
		{0x9f,0x80,0,0x81,0xff}};
	sound_play(notetbl[note]);
	unsigned char table[]={
		0xfe,0x87,
		0x10,0x88,
		0x00,0x89,
		0x00,0x89,
		0x00,0x8b,
		0x14,0x8c,
		0x00,0x8d,
		0xff};
	sound_play(table);
}

static void sound_play_death(void){
	for(int16_t note=5; note>=0; --note){
		sound_play_note(note);
		evnt_timer(0x32);
	}
}

inline void board_model::capture(int16_t worm_pos,int16_t worm){
	state[worm_pos][worm] = 0x3f;
	++worm_score[worm];
}

inline void board_render::capture(int16_t worm_pos,int16_t worm){
	sound_play_win_chord();
	model.capture(worm_pos,worm);
	worm_draw_score(worm);
	v_hide_c(vdi);
	for(int16_t direction=0; direction<6; ++direction){
		worm_draw(worm_pos,direction,-2);
	}
	evnt_timer(3);
	for(int16_t direction=0; direction<6; ++direction){
		worm_draw(worm_pos,direction,worm);
	}
	v_show_c(vdi,1);
}

inline void board_model::worm_tick(int16_t worm,int16_t direction){
	worm_brain[worm][state_all[worm_position[worm]]] = direction+1;
}

inline void board_render::worm_tick(int16_t worm,int16_t direction){
	int16_t worm_pos=model.get_worm_pos(worm);
	model.worm_tick(worm, direction);
	model.state_all[worm_pos]   += 1<<direction;
	model.state[worm_pos][worm] += 1<<direction;
	v_hide_c(vdi);
	if(model.state_all[worm_pos] == 0x3f){
		capture(worm_pos,worm);
	}else{
		VRECT line;
		worm_draw_get_line(&line,worm_pos,direction);
		sound_play_note(direction);
		worm_draw_segment(&line,-2);
		evnt_timer(3);
		worm_draw_segment(&line,worm);
	}
	signed char direction_table[6]={1,-22,-23,-1,22,23};
	worm_pos += direction_table[direction];
	if (450 < worm_pos) {
		worm_pos -= 450;
	}
	if (worm_pos < 0) {
		worm_pos += 450;
	}
	int16_t sVar1 = worm_pos % 45;
	if ((sVar1 == 0) || (sVar1 == 23)) {
		worm_pos += 20;
	}
	if ((sVar1 == 21) || (sVar1 == 44)) {
		worm_pos -= 20;
	}
	model.set_worm_pos(worm,worm_pos);
	char flip[6]={3,4,5,0,1,2};
	direction=flip[direction];
	model.state_all[worm_pos]   += 1<<direction;
	model.state[worm_pos][worm] += 1<<direction;
	if(model.state_all[worm_pos] == 0x3f){
		capture(worm_pos,worm);
		sound_play_death();
	}else{
		VRECT line;
		worm_draw_get_line(&line,worm_pos,direction);
		worm_draw_segment(&line,-2);
		evnt_timer(3);
		worm_draw_segment(&line,worm);
	}
	v_show_c(vdi,1);
}

inline void board_render::tick(){
	int16_t worm=model.get_current_worm();
	if((model.get_runmode() == 0) || (model.get_runmode() == 2)){
		if(model.current_worm_valid_move_count){
			v_hide_c(vdi);
			worm_draw(model.get_worm_pos(worm),model.current_worm_direction-1,worm_flash?worm:-1);
			v_show_c(vdi,1);
			worm_flash=!worm_flash;
		}
		return;
	}
	worm_tick(worm,model.current_worm_direction-1);
	model.next_worm();
}

static int16_t FUN_000009e2(int16_t pos){
	if(pos < 1 || pos > 449){
		return 0;
	}
	int16_t sVar1 = pos % 45;
	if(sVar1 < 1 || sVar1 > 43){
		return 0;
	}
	if (sVar1 < 21 || sVar1 > 23) {
		return 1;
	}
	return 0;
}

inline void board_render::redraw(){
	vsf_interior(vdi,1);
	vsf_color(vdi,1);
	v_hide_c(vdi);
	vr_recfl(vdi,(int16_t*)&vrect);
	for(int16_t worm=0; worm<4; ++worm){
		VRECT rect;
		rect.x1=score_pad * worm + score_x;
		rect.y1=score_y;
		rect.x2=score_pad * worm + score_w + score_x;
		rect.y2=score_h + score_y;
		worm_draw_box(&rect,worm);
		worm_draw_score(worm);
	}
	vsm_color(vdi,0);
	for(int16_t y=0; y<20; y+=2) {
		for(int16_t x=0; x<20; ++x){
			VRECT ptsin;
			ptsin.x1 = pad_x + x * cell_w*4;
			ptsin.y1 = pad_y + y * cell_h*2;
			ptsin.x2 = pad_x + x * cell_w*4 + cell_w*2;
			ptsin.y2 = pad_y + (y + 1) * cell_h*2;
			v_pmarker(vdi,2,(int16_t*)&ptsin);
		}
	}
	for(int16_t pos=1; pos<451; ++pos){
		if(FUN_000009e2(pos)){
			if(model.state_all[pos]){
				for(int16_t worm=0; worm<4; ++worm){

					auto draw_cell = [=] (int16_t color) {
						vsl_color(vdi,color);
						for(int16_t direction=0; direction<6; direction++){
							if(model.state[pos][worm] & 1<<direction){
								VRECT line;
								worm_draw_get_line(&line,pos,direction);
								v_pline(vdi,2,(int16_t*)&line);
							}
						}
					};
					worm_draw_vdi(worm,Lambda::ptr(draw_cell));
				}
			}
		}
	}
	v_show_c(vdi,1);
}

// queue an AES redraw message
static void redraw(void){
	int16_t msg[8];
	msg[0]=WM_REDRAW;
	msg[1]=gl_apid;
	//msg[2]=0;
	appl_write(gl_apid,16,&msg);
}

static void dialog_reset_button(OBJECT* obj,int16_t param_2){
	((int16_t*)obj)[param_2 * 0xc + 5] &= 0xfffe;
}

static void do_about_dialog(void){
	GRECT	z={0};
	GRECT c;
	form_center_grect(about_dialog,&c);
	form_dial_grect(FMD_START,&z,&c);
	form_dial_grect(FMD_GROW,&z,&c);
	objc_draw_grect(about_dialog,0,10,&c);
	form_do(about_dialog,0);
	form_dial_grect(FMD_SHRINK,&z,&c);
	form_dial_grect(FMD_FINISH,&z,&c);
	dialog_reset_button(about_dialog,7);
	redraw();
}

inline void board_model::reset(){
	for(int16_t i=0; i<450; ++i){
		state_all[i] = 0;
		for(int16_t worm=0; worm<4; ++worm){
			state[i][worm] = 0;
			//__asm__ volatile("" : "+g" (i) : :); // prevent optimization
		}
	}
	for(int16_t worm=0; worm<4; ++worm){
		worm_score[worm] = 0;
		worm_position[worm] = 0xd5;
	}
	current_worm_valid_move_count = 0;
}

inline void board_render::reset(){
	model.reset();
	redraw();
}

inline bool board_model::worm_direction_is_valid_move(int16_t direction){
	if(!current_worm_valid_move_count) {
		return false;
	}
	for(int16_t i=0; i<current_worm_valid_move_count; ++i){
		if (current_worm_valid_moves[i] == direction) {
			return true;
		}
	}
	return false;
}

inline void board_model::worm_rotate(int16_t delta){
	current_worm_direction += delta;
	while(!worm_direction_is_valid_move(current_worm_direction)){
		current_worm_direction += delta;
		if(current_worm_direction>6){
			current_worm_direction=1;
		}
		if(current_worm_direction<1){
			current_worm_direction=6;
		}
	}
}

inline void board_render::worm_rotate(int16_t delta){
	if(model.current_worm_valid_move_count>2){
		int16_t worm=model.get_current_worm();
		v_hide_c(vdi);
		worm_draw(model.get_worm_pos(worm),model.current_worm_direction-1,-1);
		model.worm_rotate(delta);
		worm_draw(model.get_worm_pos(worm),model.current_worm_direction-1,worm);
		v_show_c(vdi,1);
		worm_flash=1;
	}
}

inline bool board_render::key_event(int16_t event){
	// high byte is scancode
	switch(event&0xff00){
		case 0x1c00:	// Return
		case 0x7200:	// Enter (keypad)
			if (model.get_runmode() != 2) {
				set_runmode(1);
			}
			break;
		case 0x4b00:	// Left arrow
			worm_rotate(1);
			break;
		case 0x4d00:	// Right arrow
			worm_rotate(-1);
			break;
		case 0x6100:	// Undo
			return true;
	}
	return false;
}

static void menu_option_worm(
		board_render* board, int16_t worm, int16_t menu_item){
	int16_t current_worm=board->model.get_current_worm();
	switch(menu_item) {
		case 0x23:	// Reset User Mode
			board->model.worm_reset_brain(worm);
		case 0x22:	// User Mode
			board->set_worm_mode(worm,1);
			if (worm == current_worm) {
				board->set_runmode(0);
			}
			break;
		case 0x26:	// Reset Wild Mode
			board->model.worm_reset_brain(worm);
		case 0x25:	// Wild Mode
			board->set_worm_mode(worm,2);
			if (worm == current_worm) {
				board->set_runmode(1);
			}
			break;
		case 0x28:	// Off
			board->set_worm_mode(worm,0);
			if (worm == current_worm) {
				board->model.next_worm();
			}
	}
}

static int16_t menu_option_selected(
		board_render* board, int16_t me_ntitle, int16_t menu_item){
	menu_tnormal(menu,me_ntitle,1);
	switch(me_ntitle) {
		case 3:
			if(menu_item==0xb){
				do_about_dialog();
			}
			return 0;
		case 4: 
			switch(menu_item) {
				case 0x14:	// Demo
					board->reset();
					for(int16_t worm=0; worm<4; ++worm){
						board->model.worm_reset_brain(worm);
						board->set_worm_mode(worm,2);
					}
					board->set_runmode(1);
					board->model.set_current_worm(-1);
					board->model.next_worm();
					return 0;
				case 0x15:	// New Board
					board->reset();
					board->set_runmode(1);
					board->model.set_current_worm(-1);
					board->model.next_worm();
					return 0;
				case 0x16:	// Reset All
					board->reset();
					for(int16_t worm=0; worm<4; ++worm) {
						board->model.worm_reset_brain(worm);
					}
					board->set_runmode(1);
					board->model.set_current_worm(-1);
					board->model.next_worm();
					return 0;
				case 0x17:	// Quit
					return 1;
				case 0x19:	// Halt
					if (board->model.get_runmode() != 2) {
						board->set_runmode(0);
					}
					return 0;
				case 0x1a:	// Run
					if (board->model.get_runmode() != 2) {
						board->set_runmode(1);
					}
					return 0;
				case 0x1c:	// Fast
				case 0x1d:	// Medium
				case 0x1e:	// Slow
					board->set_speed(menu_item-0x1c);
					return 0;
				case 0x20:	//	Sound
					set_soundflag(!soundflag);
					return 0;
				default:
					return 0;
			}
		case 5:
		case 6:
		case 7:
		case 8:
			{
				int16_t worm=me_ntitle-5;
				menu_option_worm(board,worm,menu_item-(8*worm));
				return 0;
			}
	}
	return 0;
}

static void handle_aes_events(board_render* board){
	EVMULT_IN in={0};
	EVMULT_OUT out={0};
	int16_t msgbuff[8];
	in.emi_flags=MU_KEYBD|MU_MESAG|MU_TIMER;
	while(1){
		in.emi_tlow=board->ev_mtcount;
		int16_t event=evnt_multi_fast(&in,msgbuff,&out);
		wind_update(1);
		if(event & MU_KEYBD){
			if(board->key_event(out.emo_kreturn)) return;
		}
		if(event & MU_MESAG){
			switch(msgbuff[0]){
				case MN_SELECTED:
					if(menu_option_selected(board,msgbuff[3],msgbuff[4])) return;
					break;
				case WM_REDRAW:
				//case WM_ONTOP:
					board->redraw();
			}
		}
		if(event & MU_TIMER){
			board->tick();
		}
		wind_update(0);
	}
}

inline void board_render::set_geometry(
		int16_t x1, int16_t y1, int16_t x2, int16_t y2){
	vrect.x1 = x1;
	vrect.y1 = y1;
	vrect.x2 = x2;
	vrect.y2 = y2;
	int16_t w=x2-x1;
	int16_t h=y2-y1;

	// these values derive from system font metrics
	score_w=27;
	score_h=(y1<16)?11:25;
	score_textx=2;
	score_texty=(y1<16)?8:18;
	score_pad=78;
	if(x2>319){	// high rez horizontal
		score_w=35;
		score_textx=6;
		score_pad*=2;
	}
	
	// cell width of 3 on 320 screen
	cell_w=(int16_t)w/88;
	cell_h=(int16_t)(h-score_h)/42;
	pad_x=x1+(w-cell_w*78)/2; // center x
	pad_y=y1+(h-score_h-cell_h*39)/2;	// center y

	// place scoreboard
	score_x=(w-(score_w*4+(score_pad-score_w)*3))/2; // center x
	score_y=pad_y+cell_h*40+(cell_h/2);
	score_textx+=score_x;
	score_texty+=score_y;
}

static bool initialize(board_render* board){
	appl_init();
	if(gl_apid==-1) return false;

	if(!rsrc_load("NIGHT.RSC")){
		form_alert(1,"[3][| |Loading NIGHT.RSC failed.][ Quit ]");
		return false;
	}

	int16_t gr_hwchar,gr_hhchar,gr_hwbox,gr_hhbox;
	int16_t vdi=graf_handle(&gr_hwchar,&gr_hhchar,&gr_hwbox,&gr_hhbox);

	int16_t in[16],out[57];
	for(int16_t i=0; i<10; ++i)
		in[i] = 1;
	in[10] = 2;
	v_opnvwk(in,&vdi,out);

	rsrc_gaddr(R_TREE,out[13]>=16?3:2,&about_dialog);
	rsrc_gaddr(R_TREE,out[0]>319?0:1,&menu);
	menu_bar(menu,MENU_INSTALL);

	board->set_vdi(vdi,out[13]);
	board->set_geometry(0,gr_hhbox,out[0],out[1]);

	set_soundflag(1);
	for(int16_t i=0; i<4; ++i){
		board->model.worm_reset_brain(i);
		board->set_worm_mode(i,0);
	}
	board->set_worm_mode(0,1);
	board->model.set_current_worm(0);
	board->model.worm_generate_moves();
	board->set_speed(1);
	board->set_runmode(0);

	graf_mouse(ARROW,0);
	v_show_c(vdi,0);

	redraw();
	return true;
}

static void quit(int16_t vdi){
	v_clsvwk(vdi);
	appl_exit();
	return;
}

int main(void){
	board_render board;
	if(!initialize(&board)){
		return 1;
	}
	handle_aes_events(&board);
	quit(board.vdi);
	return 0;
}
