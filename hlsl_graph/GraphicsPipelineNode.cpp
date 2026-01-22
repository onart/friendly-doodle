#include "GraphicsPipelineNode.h"
#include "shader.h"
#include "resourcemanager.h"
#include "imgui.h"
#include "image.h"
#include "d3ddev.h"
#include "ubo.h"

struct _GPData {
	std::string vertexShaderKey;
	std::string pixelShaderKey;
	std::vector<std::string> cbKeysV;
	std::vector<std::string> cbKeysP;
	std::vector<std::string> inputTextureKeys;
	std::vector<std::string> outputTextureKeys;
};

#define pipeline (( _GPData*)this->_pipeline)

GraphicsPipelineNode::GraphicsPipelineNode() {
	_pipeline = new _GPData();
}

void GraphicsPipelineNode::run() {
	auto ctx = D3D11Device::getContext();

	auto vs = mgr.getVertexShader(pipeline->vertexShaderKey);
	vs->bind();
	auto ps = mgr.getPixelShader(pipeline->pixelShaderKey);
	ps->bind();
	int i = 0;
	void* nullObject[1]{};
	for (auto& cb : pipeline->cbKeysV) {
		auto obj = mgr.getUBO(cb);
		if (obj) {
			obj->bind(UBO::BindOn::VertexShader, i);
		}
		else {
			ctx->VSSetConstantBuffers(i, 0, (ID3D11Buffer**)nullObject);
		}
		i++;
	}
	i = 0;

	for (auto& cb : pipeline->cbKeysP) {
		auto obj = mgr.getUBO(cb);
		if (obj) { obj->bind(UBO::BindOn::PixelShader, i++); }
		else {
			ctx->PSSetConstantBuffers(i, 0, (ID3D11Buffer**)nullObject);
		}
	}
	i = 0;
	
	for (auto& targ : pipeline->inputTextureKeys) {
		auto obj = mgr.getTexture(targ);
		if (obj) { 
			obj->bindInput(i);
		}
		else {
			ctx->PSSetShaderResources(i, 1, (ID3D11ShaderResourceView**)nullObject);
		}
		i++;
	}
	i = 0;

	ID3D11RenderTargetView* rtvs[8]{};
	for (auto& targ : pipeline->outputTextureKeys) {
		auto obj = mgr.getTexture(targ);
		if (obj) {
			rtvs[i] = obj->getRTV();
		}
		i++;
	}

	if (rtvs[0]) {
		auto tex = mgr.getTexture(pipeline->outputTextureKeys[0])->getTexture();
		if (tex) {
			D3D11_TEXTURE2D_DESC desc{};
			tex->GetDesc(&desc);
			D3D11_VIEWPORT vp{};
			vp.Width = static_cast<FLOAT>(desc.Width);
			vp.Height = static_cast<FLOAT>(desc.Height);
			vp.MinDepth = 0.0f;
			vp.MaxDepth = 1.0f;
			ctx->RSSetViewports(1, &vp);
		}
	}
	ctx->OMSetRenderTargets(i, rtvs, nullptr); // no dsv for now
	
	i = 0;
}

GraphicsPipelineNode::~GraphicsPipelineNode() {
	// Cleanup resources associated with the graphics pipeline
	if (_pipeline) {
		delete pipeline;
		_pipeline = nullptr;
	}
}

bool GraphicsPipelineNode::serializeDetails(stream& s) {
	// write shader keys / 
	return !s.hadFault();
}

void GraphicsPipelineNode::drawDetails() {
	ImGui::PushID(this);
	if (ImGui::BeginCombo("Vertex Shader", pipeline->vertexShaderKey.c_str())) {
		for (auto& [key, vs] : mgr.getVertexShaders()) {
			bool selected = (key == pipeline->vertexShaderKey);
			if (ImGui::Selectable(key.c_str(), selected)) {
				pipeline->vertexShaderKey = key;
			}
			if (selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::PopID();
}