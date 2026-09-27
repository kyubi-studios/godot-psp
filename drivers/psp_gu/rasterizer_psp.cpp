#include "rasterizer_psp.h"

#include "psp_gu.h"
#include "rasterizer_scene_psp.h"
#include "storage/light_storage_psp.h"
#include "storage/material_storage_psp.h"
#include "storage/mesh_storage_psp.h"
#include "storage/texture_storage_psp.h"
#include "storage/utilities_psp.h"


void RasterizerPSP::initialize() {
	PSPGU::init();
}

void RasterizerPSP::begin_frame(double p_frame_step) {
	RasterizerDummy::begin_frame(p_frame_step);
	PSPGU::frame_begin();
}

void RasterizerPSP::end_frame(bool p_present) {
	PSPGU::frame_end(p_present);
}

RasterizerPSP::RasterizerPSP() {
	// Önce dummy örneklerini sil (yıkıcıları singleton'ları sıfırlar), sonra PSP sürümlerini kur.
	memdelete(scene);
	memdelete(texture_storage);
	memdelete(mesh_storage);
	memdelete(material_storage);
	memdelete(utilities);
	memdelete(light_storage);
	utilities = memnew(RendererPSP::Utilities);
	light_storage = memnew(RendererPSP::LightStorage);
	texture_storage = memnew(RendererPSP::TextureStorage);
	material_storage = memnew(RendererPSP::MaterialStorage);
	mesh_storage = memnew(RendererPSP::MeshStorage);
	scene = memnew(RasterizerScenePSP);
}
