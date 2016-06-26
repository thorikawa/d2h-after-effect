#include "D2H.h"
#include <cmath>

#define N_DIVIDE 16
static const float UNIT = M_PI * 2.0 / N_DIVIDE;
static const float HALF_UNIT = UNIT / 2.0;
static const float HALF_UNIT_COS = cos(HALF_UNIT);

static PF_Err
About (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	
	suites.ANSICallbacksSuite1()->sprintf(	out_data->return_msg,
											"%s v%d.%d\r%s",
											STR(StrID_Name), 
											MAJOR_VERSION, 
											MINOR_VERSION, 
											STR(StrID_Description));
	return PF_Err_NONE;
}

static PF_Err 
GlobalSetup (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	out_data->my_version = PF_VERSION(	MAJOR_VERSION, 
										MINOR_VERSION,
										BUG_VERSION, 
										STAGE_VERSION, 
										BUILD_VERSION);

	out_data->out_flags =  PF_OutFlag_DEEP_COLOR_AWARE;	// just 16bpc, not 32bpc
	
	return PF_Err_NONE;
}

static PF_Err 
ParamsSetup (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err		err		= PF_Err_NONE;
	PF_ParamDef	def;	

	AEFX_CLR_STRUCT(def);

    PF_ADD_255_SLIDER(STR(StrID_Divide_Number_Param_Name), 1, DIVIDE_NUM_DISK_ID);

	out_data->num_params = D2H_NUM_PARAMS;

	return err;
}

static PF_Err
MySimpleGainFunc16 (
	void		*refcon, 
	A_long		xL, 
	A_long		yL, 
	PF_Pixel16	*inP, 
	PF_Pixel16	*outP)
{
    // TODO
	return PF_Interrupt_CANCEL;
}

static PF_Err
MySimpleGainFunc8 (
	void		*refcon, 
	A_long		xL, 
	A_long		yL, 
	PF_Pixel8	*inP, 
	PF_Pixel8	*outP)
{
	PF_Err		err = PF_Err_NONE;

	GainInfo	*giP	= reinterpret_cast<GainInfo*>(refcon);

    static const A_long wideStep = (giP->input->rowbytes / sizeof(PF_Pixel8));

    float xF = (float) xL / giP->input->width;
    float yF = (float) yL / giP->input->height;
    float x = (xF - 0.5) * 2.0;
    float y = (yF - 0.5) * 2.0;
    float phi = atan2(y, x) + M_PI;
    
    float remain = phi;
    while (remain >= UNIT) {
        remain -= UNIT;
    }
    float len = HALF_UNIT_COS / cos(remain - HALF_UNIT);
    
    float xNewF = (x / len + 1.0f) / 2.0f;
    float yNewF = (y / len + 1.0f) / 2.0f;
    A_long xNewL = xNewF * giP->input->width;
    A_long yNewL = yNewF * giP->input->height;
    A_long indexNewL = xNewL + yNewL * wideStep;

    if (giP) {
        if (xNewL < 0 || xNewL >= giP->input->width || yNewL < 0 || yNewL >= giP->input->height) {
            outP->alpha = 255;
            outP->blue = 0;
            outP->green = 0;
            outP->blue = 0;
        } else {
            PF_Pixel8 *newP;
            PF_Err err = PF_Err_NONE;
            PF_InData *in_data = giP->in_data;
            ERR(PF_GET_PIXEL_DATA8(giP->input, NULL, &newP));
            newP += indexNewL;
            outP->alpha		=	newP->alpha;
            outP->red		=	newP->red;
            outP->green		=	newP->green;
            outP->blue		=	newP->blue;
        }
	}

	return err;
}

static PF_Err 
Render (
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err				err		= PF_Err_NONE;
	AEGP_SuiteHandler	suites(in_data->pica_basicP);

	GainInfo			giP;
	AEFX_CLR_STRUCT(giP);
	A_long				linesL	= 0;

	linesL 		= output->extent_hint.bottom - output->extent_hint.top;
    giP.divideNumber 	= params[D2H_DIVIDE_NUM]->u.sd.value;
    giP.input   = &params[0]->u.ld;
    giP.in_data = in_data;
	
	if (PF_WORLD_IS_DEEP(output)){
		ERR(suites.Iterate16Suite1()->iterate(	in_data,
												0,								// progress base
												linesL,							// progress final
												&params[D2H_INPUT]->u.ld,	    // src
												NULL,							// area - null for all pixels
												(void*)&giP,					// refcon - your custom data pointer
												MySimpleGainFunc16,				// pixel function pointer
												output));
	} else {
		ERR(suites.Iterate8Suite1()->iterate(	in_data,
												0,								// progress base
												linesL,							// progress final
												&params[D2H_INPUT]->u.ld,	    // src
												NULL,							// area - null for all pixels
												(void*)&giP,					// refcon - your custom data pointer
												MySimpleGainFunc8,				// pixel function pointer
												output));	
	}

	return err;
}


DllExport	
PF_Err 
EntryPointFunc (
	PF_Cmd			cmd,
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	void			*extra)
{
	PF_Err		err = PF_Err_NONE;
	
	try {
		switch (cmd) {
			case PF_Cmd_ABOUT:

				err = About(in_data,
							out_data,
							params,
							output);
				break;
				
			case PF_Cmd_GLOBAL_SETUP:

				err = GlobalSetup(	in_data,
									out_data,
									params,
									output);
				break;
				
			case PF_Cmd_PARAMS_SETUP:

				err = ParamsSetup(	in_data,
									out_data,
									params,
									output);
				break;
				
			case PF_Cmd_RENDER:

				err = Render(	in_data,
								out_data,
								params,
								output);
				break;
		}
	}
	catch(PF_Err &thrown_err){
		err = thrown_err;
	}
	return err;
}

