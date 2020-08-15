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

static VRECT playfield;
static short hires_flag;
static short soundflag;
static short array3d32[7]={0x0,0x1,0x2,0x4,0x8,0x10,0x20};
static short array3d54[4]={40,25,12,8};
static short array3d64[2][7]={{30,184,28,11,32,192,80},{68,336,31,27,72,384,160}};

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

static void draw_worm_set_vdi_attributes(short color){
	if(!hires_flag) {
		vsl_width(vdi_handle,1);
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
		vsl_width(vdi_handle,3);
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

static void draw_worm_segment(VRECT line,short worm){
	draw_worm_set_vdi_attributes(worm);
	v_pline(vdi_handle,2,&line.x1);
}

static void draw_worm_box(VRECT rect,short worm){
	draw_worm_set_vdi_attributes(worm);
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

static void draw_worm(short position,short direction,short color){
	VRECT line;
	line.y1 = FUN_00000788(position);
	line.x1 = line.y1 * array3d54[2] + array3d54[0];
	line.y1 = FUN_00000756(position);
	line.y1 = line.y1 * array3d54[3] + array3d54[1];
	short sVar1 = FUN_00000756(position);
	if (sVar1 % 2 == 1) {
		line.x1 = array3d54[2] / 2 + line.x1;
	}
	switch(direction) {
		case 1:
			line.x2 = array3d54[2] / 2 + line.x1;
			line.y2 = line.y1;
			break;
		case 2:
			line.x2 = array3d54[2] / 4 + line.x1;
			line.y2 = line.y1 - array3d54[3] / 2;
			break;
		case 3:
			line.x2 = line.x1 - array3d54[2] / 4;
			line.y2 = line.y1 - array3d54[3] / 2;
			break;
		case 4:
			line.x2 = line.x1 - array3d54[2] / 2;
			line.y2 = line.y1;
			break;
		case 5:
			line.x2 = line.x1 - array3d54[2] / 4;
			line.y2 = array3d54[3] / 2 + line.y1;
			break;
		case 6:
			line.x2 = array3d54[2] / 4 + line.x1;
			line.y2 = array3d54[3] / 2 + line.y1;
	}
	draw_worm_segment(line,color);
}

static void draw_worm_score(short worm){
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
			array3d64[hires_flag][6] * worm + array3d64[hires_flag][4],
			array3d64[hires_flag][5],str);
}

static void draw_playfield(void){
	vsf_interior(vdi_handle,1);
	v_hide_c(vdi_handle);
	vr_recfl(vdi_handle,&playfield.x1);
	vsm_type(vdi_handle,hires_flag?3:1);
	vsm_height(vdi_handle,hires_flag?5:1);
	vsm_color(vdi_handle,0);
	for(short y=0; y<20; y+=2) {
		for(short x=0; x<20; ++x){
			VRECT ptsin;
			ptsin.x1 = array3d54[2] * x + array3d54[0];
			ptsin.y1 = array3d54[3] * y + array3d54[1];
			ptsin.x2 = array3d54[2] * x + array3d54[0] + array3d54[2] / 2;
			ptsin.y2 = (y + 1) * array3d54[3] + array3d54[1];
			v_pmarker(vdi_handle,2,&ptsin.x1);
		}
	}
	for(short worm=0; worm<4; ++worm){
		VRECT rect;
		rect.x1=array3d64[hires_flag][6] * worm + array3d64[hires_flag][0];
		rect.y1=array3d64[hires_flag][1];
		rect.x2=array3d64[hires_flag][6] * worm + array3d64[hires_flag][2] + array3d64[hires_flag][0];
		rect.y2=array3d64[hires_flag][3]        + array3d64[hires_flag][1];
		draw_worm_box(rect,worm);
		draw_worm_score(worm);
	}
	v_show_c(vdi_handle,1);
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

static void do_capture(short worm){
	++worm_scores[worm];
	draw_worm_score(worm);
	sound_play_win_chord();
	board_state[worm_position[worm]][worm] = 0x3f;
	v_hide_c(vdi_handle);
	for(short direction=1; direction<7; ++direction){
		draw_worm(worm_position[worm],direction,0xe);
	}
	evnt_timer(3);
	for(short direction=1; direction<7; ++direction){
		draw_worm(worm_position[worm],direction,worm);
	}
	v_show_c(vdi_handle,1);
}

static void draw_worms_animate(short worm,short direction){
	short worm_pos=worm_position[worm];
	board_state_all[worm_pos]       += array3d32[direction];
	board_state[worm_pos][worm] += array3d32[direction];
	v_hide_c(vdi_handle);
	if (board_state_all[worm_pos] == 0x3f) {
		do_capture(worm);
	}else{
		sound_play_note(direction);
		draw_worm(worm_pos,direction,0xe);
		evnt_timer(3);
		draw_worm(worm_pos,direction,worm);
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
		do_capture(worm);
		sound_play_death();
	}else{
		draw_worm(worm_pos,direction,0xe);
		evnt_timer(3);
		draw_worm(worm_pos,direction,worm);
	}
	v_show_c(vdi_handle,1);
}

static void timer_tick(void){
	if((runmode == 0) || (runmode == 2)){
		if(array5adc[0]){
			v_hide_c(vdi_handle);
			draw_worm(worm_position[current_worm],direction,worm_flash?current_worm:0xf);
			v_show_c(vdi_handle,1);
			worm_flash=!worm_flash;
		}
		return;
	}
	worm_brains[current_worm][board_state_all[worm_position[current_worm]]] = direction;
	draw_worms_animate(current_worm,direction);
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

static void redraw(){
	draw_playfield();
	v_hide_c(vdi_handle);
	for(short i=1; i<451; ++i){
		if(FUN_000009e2(i) && board_state_all[i]){
			for(short worm=0; worm<4; ++worm){
				for(short direction=1; direction<7; direction++){
					if(board_state[i][worm] & array3d32[direction]){
						draw_worm(i,direction,worm);
					}
				}
			}
		}
	}
	v_show_c(vdi_handle,1);
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

static short handle_key_event(short event){
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
				draw_worm(worm_position[current_worm],direction,0xf);
				direction += 1;
				while (sVar2 = FUN_00001b28(direction,array5adc), sVar2 == 0) {
					direction += 1;
					if (6 < direction) {
						direction = 1;
					}
				}
				draw_worm(worm_position[current_worm],direction,current_worm);
				v_show_c(vdi_handle,1);
				worm_flash=1;
			}
			break;
		case 0x4d00:
			if (1 < array5adc[0]) {
				v_hide_c(vdi_handle);
				draw_worm(worm_position[current_worm],direction,0xf);
				direction += -1;
				while (sVar2 = FUN_00001b28(direction,array5adc), sVar2 == 0) {
					direction += -1;
					if (direction < 1) {
						direction = 6;
					}
				}
				draw_worm(worm_position[current_worm],direction,current_worm);
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
					draw_playfield();
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
					draw_playfield();
					set_runmode(1);
					current_worm = 0xffff;
					FUN_00001578();
					return 0;
				case 0x16:
					board_reset();
					draw_playfield();
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

static void handle_aes_events(void){
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
			timer_tick();
		}
		if((event & MU_MESAG) != 0) {
			switch(ev_mmgpbuff[0]){
				case MN_SELECTED:
					quit = menu_option_selected(ev_mmgpbuff[3],ev_mmgpbuff[4]);
					break;
				case WM_REDRAW:
				case WM_ONTOP:
					redraw();
			}
		}
		if((event & MU_KEYBD) != 0) {
			quit = handle_key_event(ev_mkreturn);
		}
		wind_update(0);
	}while(!quit);
}

static short initialize(void){
	if(appl_init()) return 0;

	short gr_hwchar,gr_hhchar,gr_hwbox,gr_hhbox;
	vdi_handle = graf_handle(&gr_hwchar,&gr_hhchar,&gr_hwbox,&gr_hhbox);

	static short work_in[16],work_out[57];
	for(short i=0; i<10; ++i)
		work_in[i] = 1;
	work_in[10] = 2;
	v_opnvwk(work_in,&vdi_handle,work_out);
	playfield.x1 = 0;
	playfield.y1 = gr_hhbox;
	playfield.x2 = work_out[0];
	playfield.y2 = work_out[1];
	hires_flag = (work_out[13]<16); // how many colors
	if(hires_flag){
		array3d54[0]*=2;
		array3d54[1]*=2;
		array3d54[2]*=2;
		array3d54[3]*=2;
	}

	if(!rsrc_load("NIGHT.RSC")){
		form_alert(1,"[3][Fatal error !|Can't find the resorce|file NIGHT.RSC][ Abort ]");
		return 0;
	}
	short index1,index2;
	if(!hires_flag){
		index1=1;
		index2=3;
	}else{
		index1=0;
		index2=2;
	}
	if(rsrc_gaddr(R_TREE,index1,&menu))
		menu_bar(menu,0x4644);
	rsrc_gaddr(R_TREE,index2,&about_dialog);

	v_show_c(vdi_handle,0);
	graf_mouse(0,0);

	soundflag = 1;
	menu_icheck(menu,0x20,1);
	board_reset();
	for(short i=0; i<4; ++i){
		worm_reset_brain(i);
		set_worm_mode(i,0);
	}
	set_worm_mode(0,1);
	draw_playfield();
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
	if(initialize()){
		handle_aes_events();
	}
	//Cnecin();
	quit();
	return 0;
}
