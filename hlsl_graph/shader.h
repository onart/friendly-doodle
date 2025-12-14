#ifndef __SHADER_H__
#define __SHADER_H__

#include <d3d11.h>

ID3D11VertexShader* createVertexShader(const void* data, size_t size);
ID3D11PixelShader* createPixelShader(const void* data, size_t size);

#endif // !__SHADER_H__

