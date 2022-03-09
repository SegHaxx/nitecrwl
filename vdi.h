#include <stdint.h>

#ifdef __GNUC__ // shut up gcc
#define SHL static __attribute__((__unused__))
#else
#define SHL static
#endif

typedef struct{
	int16_t* contrl; // Pointer to contrl array
	int16_t* intin;  // Pointer to intin array
   int16_t* ptsin;  // Pointer to ptsin array
   int16_t* intout; // Pointer to intout array
   int16_t* ptsout; // Pointer to ptsout array
} VDIPB;

VDIPB pblock;
int16_t contrl[12];
int16_t ptsin[1024];
int16_t ptsout[256];
int16_t intin[1024];
int16_t intout[512];

SHL void vdi(void){
	__asm__ __volatile__(
			"move.l %0,d1\n\t"
			"move.w #115,d0\n\t"
			"trap #2\n\t"
			:: "g"(&pblock)
			: "memory","cc");
}

SHL int16_t vdi_if(
		int16_t opcode,
		int16_t num_pts_in,
		int16_t num_int_in,
		int16_t handle)
{
	contrl[0]=opcode;
	contrl[1]=num_pts_in;
	contrl[3]=num_int_in;
	contrl[6]=handle;
	vdi();
	return(ptsout[0]);
}

SHL void v_pline(int16_t handle,int16_t count,int16_t *pxyarray){
	int16_t* prev=pblock.ptsin;
	pblock.ptsin=pxyarray;
	vdi_if(6,count,0,handle);
	pblock.ptsin=prev;
}

SHL void v_pmarker(int16_t handle,int16_t count,int16_t *pxyarray){
	for(int16_t i=0;i<count*2;++i){
		ptsin[i] = pxyarray[i];
	}
	vdi_if(7,count,0,handle);
}

SHL void v_gtext(int16_t handle,int16_t x,int16_t y,char* string){
   ptsin[0]=x;
   ptsin[1]=y;
   int16_t i=0;
   while((intin[i++]=*string++)){};
	vdi_if(8,1,i-1,handle);
}

SHL int16_t vsl_type(int16_t handle,int16_t style){
   intin[0] = style;
   return vdi_if(15,0,1,handle);
}

SHL int16_t vsl_width(int16_t handle,int16_t width){
   ptsin[0] = width;
   ptsin[1] = 0;
   return vdi_if(16,1,0,handle);
}

SHL int16_t vsl_color(int16_t handle,int16_t color_index){
	intin[0]=color_index;
	return vdi_if(17,0,1,handle);
}

SHL int16_t vsm_color(int16_t handle,int16_t color_index){
   intin[0]=color_index;
	return vdi_if(20,0,1,handle);
}

SHL int16_t vsf_interior(int16_t handle,int16_t style){
   intin[0]=style;
	return vdi_if(23,0,1,handle);
}

SHL int16_t vsf_color(int16_t handle,int16_t color_index){
   intin[0]=color_index;
	return vdi_if(25,0,1,handle);
}

SHL int16_t vswr_mode(int16_t handle,int16_t mode){
   intin[0]=mode;
	return vdi_if(32,0,1,handle);
}

// Control

SHL void v_opnvwk(
	int16_t* work_in,
	int16_t* handle,
   int16_t* work_out)
{
	pblock.contrl=contrl;
	pblock.intin =intin;
	pblock.ptsin =ptsin;
	pblock.intout=intout;
	pblock.ptsout=ptsout;

	for(int16_t i=0;i<11;++i){
		intin[i]=work_in[i];
	}
   vdi_if(100,0,11,*handle);
   *handle=contrl[6];
	for(int16_t i=0;i<45;++i){
		work_out[i]=intout[i];
	}
	for(int16_t i=0;i<12;++i){
		work_out[i+45]=ptsout[i];
	}
}

SHL void v_clsvwk(int16_t handle){
	vdi_if(101,0,0,handle);
}

SHL void vsl_ends(int16_t handle,int16_t beg_style,int16_t end_style){
   intin[0]=beg_style;
   intin[1]=end_style;
   vdi_if(108,0,2,handle);
}

SHL void vsl_udsty(int16_t handle,int16_t pattern){
   intin[0] = pattern;
	vdi_if(113,0,1,handle);
}

SHL void vr_recfl(int16_t handle,int16_t* pxyarray){
	ptsin[0]=pxyarray[0];
	ptsin[1]=pxyarray[1];
	ptsin[2]=pxyarray[2];
	ptsin[3]=pxyarray[3];
	vdi_if(114,2,0,handle);
}

SHL void v_show_c(int16_t handle,int16_t reset){
   intin[0] = reset;
	vdi_if(122,0,1,handle);
}

SHL void v_hide_c(int16_t handle){
	vdi_if(123,0,0,handle);
}
