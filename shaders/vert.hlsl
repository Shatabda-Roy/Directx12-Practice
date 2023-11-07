cbuffer ModelViewProjectionConstantBuffer : register(b0) {
	matrix mWorld;     // World matrix for object.
	matrix View; 	   // View matrix
	matrix Projection; // Projection matrix.
}
struct VS_INPUT {
	float3 vPos   : POSITION;
	float4 vColor : COLOR;
};

struct VS_OUTPUT {
	float4 Position : SV_Position;
	float4 Color    : COLOR;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT Output;
	float4 pos = float4(input.vPos,1.0f);
	// Transform the position from object space to homogeneous projection space.
	pos = mul(pos,mWorld);
	pos = mul(pos, View);
	pos = mul(pos,Projection);
	Output.Position = pos;
	// just pass through the data.
	Output.Color = input.vColor;

	return Output;
}
