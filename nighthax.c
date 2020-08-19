#include <gem.h>
#include <osbind.h>
//#include <stdio.h>

static OBJECT* menu;
static void* about_dialog;
static short vdi_handle;
static short ev_mtcount;

typedef struct{
	short x1;
	short y1;
	short x2;
	short y2;
} VRECT;

static short soundflag;
static short array3d32[7]={0x0,0x1,0x2,0x4,0x8,0x10,0x20};

typedef struct{
	VRECT vrect;
	short colordepth;
	VRECT delta;
	short score_x,score_y;
	short score_w,score_h;
	short score_textx,score_texty;
	short score_pad;
} BOARD_RENDER_T;

static short board_state[450][4];
static short board_state_all[450];
static short array5adc[7];
static short worm_scores[4];
static short worm_position[4];
static short worm_flash;
static short current_worm;
static short worm_brains[4][64];
static short worm_modes[4];
static short direction;
static short runmode;

static void set_runmode(short mode){
	short flag=0;
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
	runmode=mode;
	menu_icheck(menu,0x19,!flag);
	menu_icheck(menu,0x1a,flag);
}

static short rand(short n){
  long rand = Random();
  return (rand & 0x7fff) % n + 1;
}

static void FUN_00001126(void){
	short sVar1 = worm_brains[current_worm][board_state_all[worm_position[current_worm]]];
	if(sVar1 == 0){
		array5adc[0] = 0;
		short local_8 = board_state_all[worm_position[current_worm]];
		for(short i=1; i<7; ++i){
			if(local_8 % 2 == 0){
				array5adc[0] += 1;
				array5adc[array5adc[0]] = i;
			}
			local_8 /= 2;
		}
		if(0 < array5adc[0]){
			sVar1 = rand(array5adc[0]);
			direction = array5adc[sVar1];
		}
	}else{
		array5adc[0] = 1;
		array5adc[1] = sVar1;
		direction = sVar1;
	}
}

static void FUN_00001578(void){
	short sVar1 = current_worm;
	while ((current_worm = (current_worm + 1) % 4, worm_modes[current_worm] < 1 ||
				(board_state_all[worm_position[current_worm]] == 0x3f))) {
		if (current_worm == sVar1) {
			set_runmode(2);
			array5adc[0] = 0;
			return;
		}
	}
	FUN_00001126();
	if (worm_modes[current_worm] != 1) {
		return;
	}
	if(array5adc[0] < 2){
		return;
	}
	set_runmode(0);
}

static void set_worm_mode(short worm,short mode){
	worm_modes[worm] = mode;
	static short worm_menu_indexes[4]={0x22,0x2a,0x32,0x3a};
	short me_citem = worm_menu_indexes[worm];
	menu_icheck(menu,me_citem  ,mode==1);
	menu_icheck(menu,me_citem+3,mode==2);
	menu_icheck(menu,me_citem+6,mode==0);
}

static void worm_reset_brain(short worm){
	for(short i=0; i<64; ++i){
		worm_brains[worm][i] = 0;
	}
	worm_brains[worm][31] = 6;
	worm_brains[worm][47] = 5;
	worm_brains[worm][55] = 4;
	worm_brains[worm][59] = 3;
	worm_brains[worm][61] = 2;
	worm_brains[worm][62] = 1;
	worm_brains[worm][63] = 7;
}

static void menu_option_worm(short worm,short menu_item){
	switch(menu_item) {
		case 0x23: // Reset User Mode
			worm_reset_brain(worm);
		case 0x22: // User Mode
			set_worm_mode(worm,1);
			if (worm == current_worm) {
				set_runmode(0);
			}
			break;
		case 0x26: // Reset Wild Mode
			worm_reset_brain(worm);
		case 0x25: // Wild Mode
			set_worm_mode(worm,2);
			if (worm == current_worm) {
				set_runmode(1);
			}
			break;
		case 0x28: // Off
			set_worm_mode(worm,0);
			if (worm == current_worm) {
				FUN_00001578();
			}
	}
}

static void set_speed(short speed){
	static short speedtbl[]={0x32,0x96,500};
	for(short i=0; i<3; ++i){
		short flag=(speed==i);
		if(flag){
			ev_mtcount=speedtbl[i];
		}
		menu_icheck(menu,i+0x1c,flag);
	}
}

static void draw_worm_set_vdi_attributes(BOARD_RENDER_T* board, short color){
	vsl_width(vdi_handle,board->vrect.x2>319?3:1);
	if(board->colordepth>=16) {
		vsl_ends(vdi_handle,0,0);
		vsl_type(vdi_handle,1);
		short vdi_color=0;
		switch(color) {
			case 0:
				vdi_color=2;
				break;
			case 1:
				vdi_color=3;
				break;
			case 2:
				vdi_color=4;
				break;
			case 3:
				vdi_color=5;
				break;
			case 0xe:
				//vdi_color=0;
				break;
			case 0xf:
				vdi_color=1;
		}
		vsl_color(vdi_handle,vdi_color);
	}else{
		vsl_ends(vdi_handle,0,0);
		vsl_type(vdi_handle,1);
		//vsl_color(vdi_handle,1);
		//v_pline(vdi_handle,2,ptsin);
		vsl_color(vdi_handle,0);
		switch(color) {
			case 0:
				vsl_type(vdi_handle,3);
				break;
			case 1:
				vsl_type(vdi_handle,4);
				break;
			case 2:
				vsl_type(vdi_handle,5);
				break;
			case 3:
				vsl_type(vdi_handle,6);
				break;
			case 0xe:
				break;
			case 0xf:
				vsl_color(vdi_handle,1);
		}
	}
}

static void draw_worm_segment(BOARD_RENDER_T* board, VRECT line,short worm){
	draw_worm_set_vdi_attributes(board,worm);
	v_pline(vdi_handle,2,&line.x1);
}

static void draw_worm_box(BOARD_RENDER_T* board, VRECT rect,short worm){
	draw_worm_set_vdi_attributes(board,worm);
#if 0
	vsf_interior(vdi_handle,0);
	vsf_perimeter(vdi_handle,1);
	vr_recfl(vdi_handle,&rect.x1);
#else
	short ptsin[4];
	ptsin[0]=rect.x1;
	ptsin[1]=rect.y1;
	ptsin[2]=rect.x2;
	ptsin[3]=rect.y1;
	v_pline(vdi_handle,2,ptsin);
	ptsin[0]=rect.x2;
	ptsin[1]=rect.y2;
	ptsin[2]=rect.x1;
	ptsin[3]=rect.y2;
	v_pline(vdi_handle,2,ptsin);
	ptsin[0]=rect.x2;
	ptsin[1]=rect.y1;
	ptsin[2]=rect.x2;
	ptsin[3]=rect.y2;
	v_pline(vdi_handle,2,ptsin);
	ptsin[0]=rect.x1;
	ptsin[1]=rect.y2;
	ptsin[2]=rect.x1;
	ptsin[3]=rect.y1;
	v_pline(vdi_handle,2,ptsin);
#endif
}

static short FUN_00000756(short position){
	short tmp = (position / 0x2d) * 2;
	if (0x16 < position % 0x2d) {
		tmp += 1;
	}
	return tmp;
}

static short FUN_00000788(short position){
	short tmp = position % 0x2d;
	if (0x16 < tmp) {
		tmp += -0x17;
	}
	return tmp + -1;
}

static void draw_worm(BOARD_RENDER_T* board,short position,short direction,short color){
	VRECT line;
	line.y1 = FUN_00000788(position);
	line.x1 = line.y1 * board->delta.x2 + board->delta.x1;
	line.y1 = FUN_00000756(position);
	line.y1 = line.y1 * board->delta.y2 + board->delta.y1;
	short sVar1 = FUN_00000756(position);
	if (sVar1 % 2 == 1) {
		line.x1 = board->delta.x2 / 2 + line.x1;
	}
	switch(direction) {
		case 1:
			line.x2 = board->delta.x2 / 2 + line.x1;
			line.y2 = line.y1;
			break;
		case 2:
			line.x2 = board->delta.x2 / 4 + line.x1;
			line.y2 = line.y1 - board->delta.y2 / 2;
			break;
		case 3:
			line.x2 = line.x1 - board->delta.x2 / 4;
			line.y2 = line.y1 - board->delta.y2 / 2;
			break;
		case 4:
			line.x2 = line.x1 - board->delta.x2 / 2;
			line.y2 = line.y1;
			break;
		case 5:
			line.x2 = line.x1 - board->delta.x2 / 4;
			line.y2 = board->delta.y2 / 2 + line.y1;
			break;
		case 6:
			line.x2 = board->delta.x2 / 4 + line.x1;
			line.y2 = board->delta.y2 / 2 + line.y1;
	}
	draw_worm_segment(board,line,color);
}

static void draw_worm_score(BOARD_RENDER_T* board,short worm){
	short score = worm_scores[worm];
	char str[4];
	str[3]='\0';
	for(short i=2; i>=0; --i){
		str[i] = score%10+'0';
		if(score==0){
			str[i]=' ';
		}
		score /= 10;
	}
	v_gtext(vdi_handle,
			board->score_pad * worm + board->score_textx,
			board->score_texty,str);
}

// pump a list of byte pairs into the soundchip
// end on -1
static void sound_play(char* tbl){
	if(!soundflag) return;
	while(*tbl!=-1){
		Giaccess(*tbl++,*tbl++);
	}		
}

static void sound_play_win_chord(void){
	char chord[]={
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
		-1};
	sound_play(chord);
}

static void sound_play_note(short note){
	char notetbl[6][5]={
		{0xdd,0x80,1,0x81,-1},
		{0x7b,0x80,1,0x81,-1},
		{0x3e,0x80,1,0x81,-1},
		{0xee,0x80,0,0x81,-1},
		{0xbd,0x80,0,0x81,-1},
		{0x9f,0x80,0,0x81,-1}};
	sound_play(notetbl[note-1]);
	char table[]={
		0xfe,0x87,
		0x10,0x88,
		0x00,0x89,
		0x00,0x89,
		0x00,0x8b,
		0x14,0x8c,
		0x00,0x8d,
		-1};
	sound_play(table);
}

static void sound_play_death(void){
	for(short note=6; 0<note; --note){
		sound_play_note(note);
		evnt_timer(0x32);
	}
}

static void do_capture(BOARD_RENDER_T* board,short worm){
	++worm_scores[worm];
	draw_worm_score(board,worm);
	sound_play_win_chord();
	board_state[worm_position[worm]][worm] = 0x3f;
	v_hide_c(vdi_handle);
	for(short direction=1; direction<7; ++direction){
		draw_worm(board,worm_position[worm],direction,0xe);
	}
	evnt_timer(3);
	for(short direction=1; direction<7; ++direction){
		draw_worm(board,worm_position[worm],direction,worm);
	}
	v_show_c(vdi_handle,1);
}

static void draw_worms_animate(BOARD_RENDER_T* board,short worm,short direction){
	short worm_pos=worm_position[worm];
	board_state_all[worm_pos]       += array3d32[direction];
	board_state[worm_pos][worm] += array3d32[direction];
	v_hide_c(vdi_handle);
	if (board_state_all[worm_pos] == 0x3f) {
		do_capture(board,worm);
	}else{
		sound_play_note(direction);
		draw_worm(board,worm_pos,direction,0xe);
		evnt_timer(3);
		draw_worm(board,worm_pos,direction,worm);
	}
	short direction_table[7]={0,1,-22,-23,-1,22,23};
	worm_pos += direction_table[direction];
	if (450 < worm_pos) {
		worm_pos -= 450;
	}
	if (worm_pos < 0) {
		worm_pos += 450;
	}
	short sVar1 = worm_pos % 45;
	if ((sVar1 == 0) || (sVar1 == 23)) {
		worm_pos += 20;
	}
	if ((sVar1 == 21) || (sVar1 == 44)) {
		worm_pos -= 20;
	}
	worm_position[worm] = worm_pos;
	short flip[7]={0,4,5,6,1,2,3};
	direction=flip[direction];
	board_state_all[worm_pos]       += array3d32[direction];
	board_state[worm_pos][worm] += array3d32[direction];
	if(board_state_all[worm_pos] == 0x3f){
		do_capture(board,worm);
		sound_play_death();
	}else{
		draw_worm(board,worm_pos,direction,0xe);
		evnt_timer(3);
		draw_worm(board,worm_pos,direction,worm);
	}
	v_show_c(vdi_handle,1);
}

static void board_tick(BOARD_RENDER_T* board){
	if((runmode == 0) || (runmode == 2)){
		if(array5adc[0]){
			v_hide_c(vdi_handle);
			draw_worm(board,worm_position[current_worm],direction,worm_flash?current_worm:0xf);
			v_show_c(vdi_handle,1);
			worm_flash=!worm_flash;
		}
		return;
	}
	worm_brains[current_worm][board_state_all[worm_position[current_worm]]] = direction;
	draw_worms_animate(board,current_worm,direction);
	FUN_00001578();
}

static short FUN_000009e2(short param_1){
	if(param_1 < 1 || 0x1c1 < param_1){
		return 0;
	}
	short sVar1 = param_1 % 0x2d;
	if(sVar1 < 1 || 0x2b < sVar1){
		return 0;
	}
	if (sVar1 < 0x15 || 0x17 < sVar1) {
		return 1;
	}
	return 0;
}

static void board_redraw(BOARD_RENDER_T* board){
	v_hide_c(vdi_handle);
	vsf_interior(vdi_handle,1);
	vr_recfl(vdi_handle,(short*)&board->vrect);
	vsm_type(vdi_handle,board->vrect.x2>319?3:1);
	vsm_height(vdi_handle,board->vrect.y2>199?5:1);
	vsm_color(vdi_handle,0);
	for(short y=0; y<20; y+=2) {
		for(short x=0; x<20; ++x){
			VRECT ptsin;
			ptsin.x1 = board->delta.x2 * x + board->delta.x1;
			ptsin.y1 = board->delta.y2 * y + board->delta.y1;
			ptsin.x2 = board->delta.x2 * x + board->delta.x1 + board->delta.x2 / 2;
			ptsin.y2 = (y + 1) * board->delta.y2 + board->delta.y1;
			v_pmarker(vdi_handle,2,&ptsin.x1);
		}
	}
	for(short worm=0; worm<4; ++worm){
		VRECT rect;
		rect.x1=board->score_pad * worm + board->score_x;
		rect.y1=board->score_y;
		rect.x2=board->score_pad * worm + board->score_w + board->score_x;
		rect.y2=board->score_h        + board->score_y;
		draw_worm_box(board,rect,worm);
		draw_worm_score(board,worm);
	}
	for(short i=1; i<451; ++i){
		if(FUN_000009e2(i) && board_state_all[i]){
			for(short worm=0; worm<4; ++worm){
				for(short direction=1; direction<7; direction++){
					if(board_state[i][worm] & array3d32[direction]){
						draw_worm(board,i,direction,worm);
					}
				}
			}
		}
	}
	v_show_c(vdi_handle,1);
}

// queue an AES redraw message
static void redraw(void){
	short msg[8];
	msg[0]=WM_REDRAW;
	appl_write(0,16,&msg);
}

static void FUN_0000178e(short *param_1,short param_2){
	param_1[param_2 * 0xc + 5] = param_1[param_2 * 0xc + 5] & 0xfffe;
}

static void do_about_dialog(void){
	short local_c;
	short local_a;
	short local_8;
	short local_6;

	void* fo_ch = &local_c;
	form_center(about_dialog,&local_6,&local_8,&local_a,fo_ch);
	short uVar1 = (long)fo_ch & 0xffff;
	form_dial(0,0,0,0,0,local_6,local_8,local_a,local_c);
	uVar1 &= 0xffff;
	form_dial(1,0,0,0,0,local_6,local_8,local_a,local_c);
	uVar1 &= 0xffff;
	objc_draw(about_dialog,0,10,local_6,local_8,local_a,local_c);
	form_do(about_dialog,uVar1 & 0xffff);
	form_dial(2,0,0,0,0,local_6,local_8,local_a,local_c);
	form_dial(3,0,0,0,0,local_6,local_8,local_a,local_c);
	FUN_0000178e(about_dialog,7);
	redraw();
	return;
}

static void board_reset(){
	for(short i=0; i<450; ++i){
		board_state_all[i] = 0;
		for(short worm=0; worm<4; ++worm){
			board_state[i][worm] = 0;
			//__asm__ volatile("" : "+g" (i) : :); // prevent optimization
		}
	}
	for(short worm=0; worm<4; ++worm){
		worm_scores[worm] = 0;
		worm_position[worm] = 0xd5;
	}
	array5adc[0] = 0;
	redraw();
	return;
}

static short FUN_00001b28(short param_1,short *param_2){
	if (0 < param_2[0]) {
		for(short i=1; i<=param_2[0]; ++i){
			if (param_2[i] == param_1) {
				return 1;
			}
		}
	}
	return 0;
}

static short handle_key_event(BOARD_RENDER_T* board,short event){
	short sVar2;
	switch(event&0xff00){
		case 0x1c00:
		case 0x7200:
			if (runmode != 2) {
				set_runmode(1);
			}
			break;
		case 0x4b00:
			if (1 < array5adc[0]) {
				v_hide_c(vdi_handle);
				draw_worm(board,worm_position[current_worm],direction,0xf);
				direction += 1;
				while (sVar2 = FUN_00001b28(direction,array5adc), sVar2 == 0) {
					direction += 1;
					if (6 < direction) {
						direction = 1;
					}
				}
				draw_worm(board,worm_position[current_worm],direction,current_worm);
				v_show_c(vdi_handle,1);
				worm_flash=1;
			}
			break;
		case 0x4d00:
			if (1 < array5adc[0]) {
				v_hide_c(vdi_handle);
				draw_worm(board,worm_position[current_worm],direction,0xf);
				direction += -1;
				while (sVar2 = FUN_00001b28(direction,array5adc), sVar2 == 0) {
					direction += -1;
					if (direction < 1) {
						direction = 6;
					}
				}
				draw_worm(board,worm_position[current_worm],direction,current_worm);
				v_show_c(vdi_handle,1);
				worm_flash=1;
			}
			break;
		case 0x6100:
			return 1;
	}
	return 0;
}

static short menu_option_selected(short me_ntitle,short menu_item){
	menu_tnormal(menu,me_ntitle,1);
	switch(me_ntitle) {
		case 3:
			break;
		case 4: 
			switch(menu_item) {
				case 0x14:
					board_reset();
					for(short worm=0; worm<4; ++worm){
						worm_reset_brain(worm);
						set_worm_mode(worm,2);
					}
					set_runmode(1);
					current_worm = 0xffff;
					FUN_00001578();
					return 0;
				case 0x15:
					board_reset();
					set_runmode(1);
					current_worm = 0xffff;
					FUN_00001578();
					return 0;
				case 0x16:
					board_reset();
					for(short worm=0; worm<4; ++worm) {
						worm_reset_brain(worm);
					}
					set_runmode(1);
					current_worm = 0xffff;
					FUN_00001578();
					return 0;
				case 0x17:
					return 1;
				default:
					return 0;
				case 0x19:
					if (runmode != 2) {
						set_runmode(0);
					}
					return 0;
				case 0x1a:
					if (runmode != 2) {
						set_runmode(1);
					}
					return 0;
				case 0x1c:
				case 0x1d:
				case 0x1e:
					set_speed(menu_item-0x1c);
					return 0;
				case 0x20:
					soundflag = !soundflag;
					menu_icheck(menu,0x20,soundflag);
					return 0;
			}
		case 5:
			menu_option_worm(0,menu_item);
			return 0;
		case 6:
			menu_option_worm(1,menu_item + -8);
			return 0;
		case 7:
			menu_option_worm(2,menu_item + -0x10);
			return 0;
		case 8:
			menu_option_worm(3,menu_item + -0x18);
		default:
			return 0;
	}
	if (menu_item != 0xb) {
		return 0;
	}
	do_about_dialog();
	return 0;
}

static void handle_aes_events(BOARD_RENDER_T* board){
	short quit = 0;
	do {
		short dummy;
		short ev_mkreturn;
		short ev_mmgpbuff[8];
		short event = evnt_multi(MU_KEYBD|MU_MESAG|MU_TIMER,
				0,0,0,0,0,0,0,0,0,0,0,0,0,
				ev_mmgpbuff,
				ev_mtcount,
				&dummy,&dummy,&dummy,&dummy,
				&ev_mkreturn,
				&dummy);
		wind_update(1);
		if(event & MU_TIMER){
			board_tick(board);
		}
		if((event & MU_MESAG) != 0) {
			switch(ev_mmgpbuff[0]){
				case MN_SELECTED:
					quit = menu_option_selected(ev_mmgpbuff[3],ev_mmgpbuff[4]);
					break;
				case WM_REDRAW:
				case WM_ONTOP:
					board_redraw(board);
			}
		}
		if((event & MU_KEYBD) != 0) {
			quit = handle_key_event(board,ev_mkreturn);
		}
		wind_update(0);
	}while(!quit);
}

static short initialize(BOARD_RENDER_T* board){
	if(appl_init()) return 0;

	short gr_hwchar,gr_hhchar,gr_hwbox,gr_hhbox;
	vdi_handle = graf_handle(&gr_hwchar,&gr_hhchar,&gr_hwbox,&gr_hhbox);

	static short work_in[16],work_out[57];
	for(short i=0; i<10; ++i)
		work_in[i] = 1;
	work_in[10] = 2;
	v_opnvwk(work_in,&vdi_handle,work_out);
	board->vrect.x1 = 0;
	board->vrect.y1 = gr_hhbox;
	board->vrect.x2 = work_out[0];
	board->vrect.y2 = work_out[1];
	board->colordepth = work_out[13];

	if(!rsrc_load("NIGHT.RSC")){
		form_alert(1,"[3][Fatal error !|Can't find the resorce|file NIGHT.RSC][ Abort ]");
		return 0;
	}

	board->delta.x1=40;
	board->delta.y1=25;
	board->delta.x2=12;
	board->delta.y2=8;

	board->score_x=30;
	board->score_y=184;
	board->score_w=28;
	board->score_h=11;
	board->score_textx=32;
	board->score_texty=192;
	board->score_pad=80;

	short menu_index=1;
	if(board->vrect.x2>319){	// high rez horizontal
		board->delta.x1*=2;
		board->delta.x2*=2;
		board->score_x=68;
		board->score_w=31;
		board->score_textx=72;
		board->score_pad*=2;
		menu_index=0;
	}
	if(board->vrect.y2>199){	// high rez vertical
		board->delta.y1*=2;
		board->delta.y2*=2;
		board->score_y=365;
		board->score_h=27;
		board->score_texty*=2;
	}

	if(rsrc_gaddr(R_TREE,menu_index,&menu))
		menu_bar(menu,0x4644);
	rsrc_gaddr(R_TREE,board->colordepth>=16?3:2,&about_dialog);

	v_show_c(vdi_handle,0);
	graf_mouse(0,0);

	soundflag = 1;
	menu_icheck(menu,0x20,1);
	for(short i=0; i<4; ++i){
		worm_reset_brain(i);
		set_worm_mode(i,0);
	}
	set_worm_mode(0,1);
	board_reset();
	current_worm = 0;
	FUN_00001126();
	set_speed(1);
	set_runmode(0);
	return 1;
}

static void quit(void){
	v_clsvwk(vdi_handle);
	appl_exit();
	return;
}

int main(int argc, char *argv[]){
	BOARD_RENDER_T board;
	if(initialize(&board)){
		handle_aes_events(&board);
	}
	quit();
	return 0;
}
