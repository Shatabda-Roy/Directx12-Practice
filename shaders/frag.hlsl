struct PS_INPUT {
    float4 Postion : SV_Position; // Interpolated vertex position (system value).
    float4 Color   : COLOR;       // Interpolated diffuse color.
};
float4 main(PS_INPUT input) :SV_TARGET
{
	return input.Color;
}