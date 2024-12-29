struct VS_OUTPUT
{
    float4 oD0 : COLOR0;
    float4 oD1 : COLOR1;
    float4 oT0 : TEXCOORD0;
    float4 oT1 : TEXCOORD1;
    float4 oT2 : TEXCOORD2;
    float4 oT3 : TEXCOORD3;
    float oFog : FOG;
    float4 oPos : POSITION;
    float4 oT7 : TEXCOORD7;
};
//global vs 2.0 vertex shader header for asm conversions. (preceded by VS_INPUT/VS_OUTPUT struct)
//not used if #define HLSL is at the top of the .vsc file.

float4 c[96*2+16] : register(c0);  // grab all the xbox specific constant register space

void hsge(out float4 dest, float4 src0, float4 src1)
{
	dest.x = (src0.x >= src1.x) ? 1.0f : 0.0f;
	dest.y = (src0.y >= src1.y) ? 1.0f : 0.0f;
	dest.z = (src0.z >= src1.z) ? 1.0f : 0.0f;
	dest.w = (src0.w >= src1.w) ? 1.0f : 0.0f;
}

void hslt(out float4 dest, float4 src0, float4 src1)
{
	dest.x = (src0.x < src1.x) ? 1.0f : 0.0f;
	dest.y = (src0.y < src1.y) ? 1.0f : 0.0f;
	dest.z = (src0.z < src1.z) ? 1.0f : 0.0f;
	dest.w = (src0.w < src1.w) ? 1.0f : 0.0f;
}

void hsincos(out float2 dest, in float angle)
{
	sincos(angle, dest.y, dest.x);
}

float dp2(float4 a, float4 b)
{
	return a.x*b.x+a.y*b.y;
}

float dp3(float4 a, float4 b)
{
	return a.x*b.x+a.y*b.y+a.z*b.z;
}

float dp4(float4 a, float4 b)
{
	return a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w;
}

VS_OUTPUT main(//input registers
	float4 va_position: POSITION0,
	float4 va_node_indices: BLENDINDICES0,
	float4 va_node_weights: BLENDWEIGHT0,
	float4 va_texcoord: TEXCOORD0,
	float4 va_normal: NORMAL0,
	float4 va_binormal: BINORMAL0,
	float4 va_tangent: TANGENT0,
	float4 va_anisotropic_binormal: BINORMAL1,
	float4 va_incident_radiosity: COLOR1,
	float4 va_secondary_texcoord: TEXCOORD1,
	float4 va_isq_secondary_position: POSITION1,
	float4 va_isq_secondary_node_indices: BLENDINDICES1,
	float4 va_isq_secondary_node_weights: BLENDWEIGHT1,
	float4 va_isq_select: TEXCOORD2,
	float4 va_color: COLOR0,
	float4 va_tint_factor: COLOR2,
	float4 va_dsq_plane: TEXCOORD3,
	float4 va_billboard_offset: TEXCOORD4,
	float4 va_billboard_axis: TEXCOORD5,
	float4 va_pca_cluster_id: TEXCOORD6,
	float4 va_pca_weights: TEXCOORD7) 
{
	VS_OUTPUT output;

//temps    
    float4 r0 = float4(1.0f, 1.0f, 1.0f, 1.0f);
    float4 r1 = float4(1.0f, 1.0f, 1.0f, 1.0f);
    float4 r2 = float4(1.0f, 1.0f, 1.0f, 1.0f);
    float4 r3 = float4(1.0f, 1.0f, 1.0f, 1.0f);
    float4 r4 = float4(1.0f, 1.0f, 1.0f, 1.0f);
    float4 r5 = float4(1.0f, 1.0f, 1.0f, 1.0f);
    float4 r6 = float4(1.0f, 1.0f, 1.0f, 1.0f);
    float4 r7 = float4(1.0f, 1.0f, 1.0f, 1.0f);
    float4 r8 = float4(1.0f, 1.0f, 1.0f, 1.0f);
    float4 r9 = float4(1.0f, 1.0f, 1.0f, 1.0f);
    float4 r10 = float4(1.0f, 1.0f, 1.0f, 1.0f);
    float4 r11 = float4(1.0f, 1.0f, 1.0f, 1.0f);
	float4 r12 = float4(1.0f, 1.0f, 1.0f, 1.0f);
	int4 a0;  //only use .x
	
//output registers
    float4 oT0 = float4(1.0f, 1.0f, 1.0f, 1.0f);
    float4 oT1 = float4(1.0f, 1.0f, 1.0f, 1.0f);
    float4 oT2 = float4(1.0f, 1.0f, 1.0f, 1.0f);
    float4 oT3 = float4(1.0f, 1.0f, 1.0f, 1.0f);
    float4 oT4 = float4(1.0f, 1.0f, 1.0f, 1.0f);
    float4 oT5 = float4(1.0f, 1.0f, 1.0f, 1.0f);
    float4 oT6 = float4(1.0f, 1.0f, 1.0f, 1.0f);
    float4 oT7 = float4(1.0f, 1.0f, 1.0f, 1.0f);

	float oFog = 1.0f;
	float oPts = 1.0f;
    
    float4 oD0 = float4(1.0f, 1.0f, 1.0f, 1.0f);
    float4 oD1 = float4(1.0f, 1.0f, 1.0f, 1.0f);
    
	float4 oPos = float4(1.0f, 1.0f, 1.0f, 1.0f);


	









#define _POSITION	r0	
#define _XFORM_X	r4 
#define _XFORM_Y	r5 
#define _XFORM_Z	r6 
	
	r4  = c[50] ;
 	r5  = c[51] ;
 	r6  = c[52] ;
 {
    float4 temp = (float4)(va_position * c[170].xyzz );
    	r10.x=temp.x;
    	r10.y=temp.y;
    	r10.z=temp.z;
}
{
    float4 temp = (float4)(r10 + c[171].xyzz );
    	r10.x=temp.x;
    	r10.y=temp.y;
    	r10.z=temp.z;
}
{
    float4 temp = (float4)(dot(r10.xyz, _XFORM_X.xyz));
    	r0.x=temp.x;
}
{
    float4 temp = (float4)(r0.xxxx + _XFORM_X.wwww );
    	r0.x=temp.x;
}
{
    float4 temp = (float4)(dot(r10.xyz, _XFORM_Y.xyz));
    	r0.y=temp.y;
}
{
    float4 temp = (float4)(r0.yyyy + _XFORM_Y.wwww );
    	r0.y=temp.y;
}
{
    float4 temp = (float4)(dot(r10.xyz, _XFORM_Z.xyz));
    	r0.z=temp.z;
}
{
    float4 temp = (float4)(r0.zzzz + _XFORM_Z.wwww );
    	r0.z=temp.z;
}
{
    float4 temp = (float4)(c[4].wwww  );
    	_POSITION.w=temp.w;
}


{
	float temptemp;
	temptemp = dot(_POSITION.xyz, c[0].xyz);
 	temptemp = temptemp + c[0].wwww ;
 {
    float4 temp = (float4)(temptemp );
    	r10.x=temp.x;
}
}
{
	float temptemp;
	temptemp = dot(_POSITION.xyz, c[1].xyz);
 	temptemp = temptemp + c[1].wwww ;
 {
    float4 temp = (float4)(temptemp );
    	r10.y=temp.y;
}
}
{
	float temptemp;
	temptemp = dot(_POSITION.xyz, c[2].xyz);
 	temptemp = temptemp + c[2].wwww ;
 {
    float4 temp = (float4)(temptemp );
    	r10.z=temp.z;
}
}
{
	float temptemp;
	temptemp = dot(_POSITION.xyz, c[3].xyz);
 	temptemp = temptemp + c[3].wwww ;
 {
    float4 temp = (float4)(temptemp );
    	r10.w=temp.w;
}
}

	oPos = r10 ;
 
	oFog = (r10.wwww * c[18 + 6].xxxx ) + c[18 + 6].yyyy ;
 {
    float4 temp = (float4)((r10.wwww * c[18 + 6].xxxx ) + c[18 + 6].yyyy );
    	oT0.z=temp.z;
}


{
    float4 temp = (float4)((_POSITION.xyyy * c[18 + 2].xyyy  ) + c[18 + 2].zwww );
    	oT0.x=temp.x;
    	oT0.y=temp.y;
}




	r11 = (r10.wwww * c[18 + 3].xzxx ) + r10 ;
 	r11 = r11 * c[18 + 3].xyxx ;
 {
    float4 temp = (float4)(1.0f / (r11.wwww ));
        r11.w=temp.w;
}
	oT1 = r11 * r11.wwww ;
 {
	float4 wookiewookie;
	hsge(wookiewookie, r11 , r11 );
{
    float4 temp = (float4)(wookiewookie);
    	oT1.w=temp.w;
}
}



{
    float4 temp = (float4)(_POSITION.xyyy * c[18 + 3].xyyy );
    	r11.x=temp.x;
    	r11.y=temp.y;
}
{
    float4 temp = (float4)(r11.xyyy - c[18 + 4].xyyy );
    	r11.x=temp.x;
    	r11.y=temp.y;
}
{
    float4 temp = (float4)(r11.xyyy * c[18 + 4].zwww );
    	r11.x=temp.x;
    	r11.y=temp.y;
}
{
    float4 temp = (float4)((r11.xyyy * c[5].wwww  ) + c[5].wwww  );
    	oT2.x=temp.x;
    	oT2.y=temp.y;
}


{
    float4 temp = (float4)(c[7]  - _POSITION );
    	r11.x=temp.x;
    	r11.y=temp.y;
    	r11.z=temp.z;
}
{
    float4 temp = (float4)(dot(r11.xyz, r11.xyz));
    	r11.w=temp.w;
}
{
    float4 temp = (float4)(rsqrt(r11.wwww ));
    	r11.w=temp.w;
}
{
    float4 temp = (float4)(r11 * r11.wwww );
    	r11.x=temp.x;
    	r11.y=temp.y;
    	r11.z=temp.z;
}

	r11 = r11 * c[5].wwww  ;
 {
    float4 temp = (float4)(r11.xyyy + c[5].wwww  );
    	oT3.x=temp.x;
    	oT3.y=temp.y;
}

	oD0 = c[18 + 5] ;
 
{
    float4 temp = (float4)(c[18 + 7] );
    	oD1.x=temp.x;
    	oD1.y=temp.y;
    	oD1.z=temp.z;
}



output.oD0=oD0;
output.oD1=oD1;
output.oT0=oT0;
output.oT1=oT1;
output.oT2=oT2;
output.oT3=oT3;
output.oFog=oFog;
output.oPos=oPos;
// cram anything additional here that needs to happen at the end of all vertex shaders.

	output.oT7.xyzw = oPos; //float4(oPos.z, oPos.w); 
	

	return output;
}
