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
	std::list<std::string> cbKeysV;
	std::list<std::string> cbKeysP;
	std::list<std::string> inputTextureKeys;
	std::list<std::string> outputTextureKeys;
	int vertexCount = 3;
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
		auto tex = mgr.getTexture(pipeline->outputTextureKeys.front())->getTexture();
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
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	ctx->Draw(pipeline->vertexCount, 0);
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

	// VS
	ImGui::PushID('v');
	if (ImGui::CollapsingHeader("Vertex Shader...")) {
		const std::string cbh = "cb #";
		int cbi = 0;
		for (auto it = pipeline->cbKeysV.begin(); it != pipeline->cbKeysV.end();) {
			if (ImGui::BeginCombo((cbh + std::to_string(cbi)).c_str(), it->c_str())) {
				for (auto& [key, cb] : mgr.getUBOs()) {
					bool selected = (*it == pipeline->vertexShaderKey);
					if (ImGui::Selectable(key.c_str(), selected)) {
						*it = key;
					}
				}
				ImGui::EndCombo();
			}
			ImGui::SameLine();
			ImGui::PushID(cbi);
			if (ImGui::Button("x")) {
				it = pipeline->cbKeysV.erase(it);
			}
			else {
				++it;
			}
			ImGui::PopID();
			cbi++;
		}
		if (ImGui::Button("+")) {
			pipeline->cbKeysV.push_back("");
		}
		if (ImGui::BeginCombo("Shader", pipeline->vertexShaderKey.c_str())) {
			for (auto& [key, vs] : mgr.getVertexShaders()) {
				bool selected = (key == pipeline->vertexShaderKey);
				if (ImGui::Selectable(key.c_str(), selected)) {
					pipeline->vertexShaderKey = key;
				}
			}
			ImGui::EndCombo();
		}
		if (auto vs = mgr.getVertexShader(pipeline->vertexShaderKey)) {
			ImGui::Indent();
			if (ImGui::CollapsingHeader((pipeline->vertexShaderKey + "##vs").c_str())) {
				vs->draw();
			}
			ImGui::Unindent();
		}
	}
	ImGui::PopID(); // 'v'

	// PS
	ImGui::PushID('p');
	if (ImGui::CollapsingHeader("Pixel Shader...")) {
		const std::string cbh = "cb #";
		int cbi = 0;
		for (auto it = pipeline->cbKeysV.begin(); it != pipeline->cbKeysV.end();) {
			if (ImGui::BeginCombo((cbh + std::to_string(cbi)).c_str(), it->c_str())) {
				for (auto& [key, cb] : mgr.getUBOs()) {
					bool selected = (*it == pipeline->vertexShaderKey);
					if (ImGui::Selectable(key.c_str(), selected)) {
						*it = key;
					}
				}
				ImGui::EndCombo();
			}
			ImGui::SameLine();
			ImGui::PushID(cbi);
			if (ImGui::Button("x")) {
				it = pipeline->cbKeysV.erase(it);
			}
			else {
				++it;
			}
			ImGui::PopID();
			cbi++;
		}
		if (ImGui::Button("+")) {
			pipeline->cbKeysV.push_back("");
		}
		if (ImGui::BeginCombo("Pixel Shader", pipeline->pixelShaderKey.c_str())) {
			for (auto& [key, vs] : mgr.getPixelShaders()) {
				bool selected = (key == pipeline->pixelShaderKey);
				if (ImGui::Selectable(key.c_str(), selected)) {
					pipeline->pixelShaderKey = key;
				}
			}
			ImGui::EndCombo();
		}
		if (auto vs = mgr.getPixelShader(pipeline->pixelShaderKey)) {
			if (ImGui::CollapsingHeader((pipeline->pixelShaderKey + "##vs").c_str())) {
				vs->draw();
			}
		}
	}
	ImGui::PopID(); // 'p'

	ImGui::PopID();
}