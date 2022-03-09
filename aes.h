#include <stdint.h>

#ifdef __GNUC__ // shut up gcc
#define SHL static __attribute__((__unused__))
#else
#define SHL static
#endif

#define WM_REDRAW 20

typedef struct{
   int16_t x,y,w,h;
} GRECT;

typedef struct{
	int16_t	ob_next;   // The next object
	int16_t	ob_head;   // First child
	int16_t	ob_tail;   // Last child
	uint16_t	ob_type;   // Object type
	uint16_t	ob_flags;  // Manipulation flags
	uint16_t	ob_state;  // Object status
	void		*ob_spec;  // More under object type
	GRECT		ob;
} OBJECT;

typedef struct{
	int16_t* cb_pcontrol; // Pointer to control array
	int16_t* cb_pglobal;  // Pointer to global array
	int16_t* cb_pintin;   // Pointer to int_in array
	int16_t* cb_pintout;  // Pointer to int_out array
	   void* cb_padrin;   // Pointer to adr_in array
	   void* cb_padrout;  // Pointer to adr_out array
} AESPB;

AESPB aespb;
void* addr_in[8];
void* addr_out[2];
int16_t control[5];
int16_t int_in[16];
int16_t int_out[10];
int16_t aes_global[15];

int16_t gl_apid;

static void aes(void){
	__asm__ __volatile__(
			"move.l %0,d1\n\t"
			"move.w #200,d0\n\t"
			"trap #2\n\t"
			:: "g"(&aespb)
			: "memory","cc");
}

static int16_t crys_if(
		int16_t opcode,
		int16_t num_int_in,
		int16_t num_int_out,
		int16_t num_addr_in)
{
	control[0]=opcode;
	control[1]=num_int_in;
	control[2]=num_int_out;
	control[3]=num_addr_in;
	aes();
	return(int_out[0]);
}

// Application Library

SHL int16_t appl_init(void){
	aespb.cb_pcontrol=control;
	aespb.cb_pglobal =aes_global;
	aespb.cb_pintin  =int_in;
	aespb.cb_pintout =int_out;
	aespb.cb_padrin  =addr_in;
	aespb.cb_padrout =addr_out;
	control[4]=0;
	gl_apid=crys_if(10,0,1,0);
	return gl_apid;
}

SHL int16_t appl_write(int16_t ap_wid,int16_t ap_wlength,void *ap_wpbuff){
   int_in[0] =ap_wid;
   int_in[1] =ap_wlength;
   addr_in[0]=ap_wpbuff;
	return(crys_if(12,2,1,1));
}

SHL int16_t appl_exit(void){
	return(crys_if(19,0,1,0));
}

// Menu Library

#define MN_SELECTED 10

SHL int16_t menu_icheck(
	OBJECT *me_ctree,
	int16_t me_citem,
	int16_t me_ccheck)
{
   int_in[0]  = me_citem;
   int_in[1]  = me_ccheck;
   addr_in[0] = me_ctree;
   return crys_if(31,2,1,1);
}

// Event Library

typedef struct point_coord{
	int16_t p_x;
	int16_t p_y;
} PXY;

typedef struct {
	int16_t emi_flags; // the event mask to watch
	int16_t emi_bclicks;		  /**< see mt_evnt_multi() */
	int16_t emi_bmask;		  /**< see mt_evnt_multi() */
	int16_t emi_bstate;		  /**< see mt_evnt_multi() */
	int16_t emi_m1leave;		  /**< TODO */
	GRECT emi_m1;             /**< the first rectangle to watch */
	int16_t emi_m2leave;		  /**< TODO */
	GRECT emi_m2;             /**< the second rectangle to watch */
	int16_t emi_tlow;		  	  /**< see mt_evnt_multi() */
	int16_t emi_thigh;          /**< the timer 32-bit value of interval split into int16_t type member */
} EVMULT_IN;

typedef struct {
	int16_t emo_events;	// the bitfield of events occured
	PXY   emo_mouse;
	int16_t emo_mbutton;
	int16_t emo_kmeta;
	int16_t emo_kreturn;
	int16_t emo_mclicks;
} EVMULT_OUT;

// evnt_multi flags
#define MU_KEYBD	0x0001 // Wait for a user keypress, see mt_evnt_multi() */
#define MU_BUTTON	0x0002 // Wait for the specified mouse button state, see mt_evnt_multi() */
#define MU_M1		0x0004 // Wait for a mouse/rectangle event as specified, see mt_evnt_multi() */
#define MU_M2		0x0008 // Wait for a mouse/rectangle event as specified, see mt_evnt_multi()  */
#define MU_MESAG	0x0010 // Wait for a message
#define MU_TIMER	0x0020 // Wait the specified amount of time

SHL int16_t evnt_timer(
	uint32_t ev_msec)
{
	uint16_t* msec=(uint16_t*)&ev_msec;
   int_in[0]=msec[1];
   int_in[1]=msec[0];
   return crys_if(24,2,1,0);
}

SHL int16_t evnt_multi_fast(
	const EVMULT_IN* em_in,
	int16_t msg[],
	EVMULT_OUT* em_out)
{
	int16_t* prev_intin =aespb.cb_pintin;
	int16_t* prev_intout=aespb.cb_pintout;
	aespb.cb_pintin =(int16_t*)em_in;
	aespb.cb_pintout=(int16_t*)em_out;
	addr_in[0]=msg;
	crys_if(25,16,7,1);
	aespb.cb_pintin =prev_intin;
	aespb.cb_pintout=prev_intout;
	return em_out->emo_events;
}

// Menu Library

#define MENU_INSTALL 1

SHL int16_t menu_bar(OBJECT* me_btree,int16_t me_bshow){
   int_in[0] =me_bshow;
   addr_in[0]=me_btree;
   return crys_if(30,1,1,1);
}	

SHL int16_t menu_tnormal(
	OBJECT* me_ntree,
	int16_t me_ntitle,
	int16_t me_nnormal)
{
   addr_in[0]=me_ntree;
   int_in[0] =me_ntitle;
   int_in[1] =me_nnormal;
   return crys_if(33,2,1,1);
}

// obj

SHL int16_t objc_draw_grect(
		OBJECT* tree,
		int16_t start,
		int16_t depth,
		const GRECT* clip)
{
	addr_in[0]=tree;
	int_in[0]=start;
	int_in[1]=depth;
	*(GRECT*)(int_in+2)=*clip;
	return crys_if(42,6,1,1);
}

// forms

SHL int16_t form_do(OBJECT* fo_dotree,int16_t fo_dostartob){
	int_in[0]=fo_dostartob;
	addr_in[0]=fo_dotree;
	return crys_if(50,1,1,1);
}

#define FMD_START 0
#define FMD_GROW 1
#define FMD_SHRINK 2
#define FMD_FINISH 3

SHL int16_t form_dial_grect(
		int16_t fo_diflag,
		GRECT* fo_dilittl,
		GRECT* fo_dibig)
{
	int_in[0]  = fo_diflag;
	if(fo_dilittl){
		int_in[1]  = fo_dilittl->x;
		int_in[2]  = fo_dilittl->y;
		int_in[3]  = fo_dilittl->w;
		int_in[4]  = fo_dilittl->h;
	}
	int_in[5]  = fo_dibig->x;
	int_in[6]  = fo_dibig->y;
	int_in[7]  = fo_dibig->w;
	int_in[8]  = fo_dibig->h;
	return crys_if(51,9,1,1);
}

SHL int16_t form_alert(int16_t fo_adefbttn,const char* fo_astring){
   int_in[0] =fo_adefbttn;
   addr_in[0]=(char*)fo_astring;
	return crys_if(52,1,1,1);
}

SHL int16_t form_center_grect(OBJECT* fo_ctree,GRECT* fo_c){
   addr_in[0]=fo_ctree;
   int16_t ret=crys_if(54,0,5,1);
   fo_c->x=int_out[1];
   fo_c->y=int_out[2];
   fo_c->w=int_out[3];
   fo_c->h=int_out[4];
   return(ret);
}

// Graphics Library

SHL int16_t graf_handle(
	int16_t* gr_hwchar,
	int16_t* gr_hhchar,
	int16_t* gr_hwbox,
	int16_t* gr_hhbox)
{
   int16_t ret=crys_if(77,0,5,0);
   *gr_hwchar=int_out[1];
   *gr_hhchar=int_out[2];
   *gr_hwbox =int_out[3];
   *gr_hhbox =int_out[4];
   return ret;
}

typedef struct{
	int16_t mf_xhot;     // X-position hot-spot
	int16_t mf_yhot;     // Y-position hot-spot
	int16_t mf_nplanes;  // Number of planes
	int16_t mf_fg;       // Mask colour
	int16_t mf_bg;       // Pointer colour
	int16_t mf_mask[16]; // Mask form
	int16_t mf_data[16]; // Pointer form
} MFORM;

#define ARROW 0

int16_t graf_mouse(int16_t gr_monumber,MFORM* gr_mofaddr){
   int_in[0]=gr_monumber;
   addr_in[0]=gr_mofaddr;
	return crys_if(78,1,1,1);
}

// Window Library

SHL int16_t wind_update(int16_t wi_ubegend){
   int_in[0]=wi_ubegend;
	return crys_if(107,1,1,0);
}

// Resource Library

SHL int16_t rsrc_load(const char* re_lpfname){
	addr_in[0]=(void*)re_lpfname;
	return crys_if(110,0,1,1);
}

#define R_TREE 0 // Object tree

SHL int16_t rsrc_gaddr(int16_t re_gtype,int16_t re_gindex,void* gaddr){
	int_in[0]=re_gtype;
	int_in[1]=re_gindex;
	//control[4]=1;
	int16_t ret=crys_if(112,2,1,0);
	//control[4]=0;
	*((void**)gaddr)=(void*)addr_out[0];
	return ret;
}
