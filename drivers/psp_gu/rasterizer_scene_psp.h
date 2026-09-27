#pragma once

#include "servers/rendering/dummy/rasterizer_scene_dummy.h"

#include "servers/rendering/storage/render_scene_buffers.h"

// PSP'de ara buffer yok (doğrudan ekran framebuffer'ına çizilir); RendererSceneCull yine de geçerli bir
// RenderSceneBuffers ister, yoksa sahneyi çizmeden döner.
class RenderSceneBuffersPSP : public RenderSceneBuffers {
	GDSOFTCLASS(RenderSceneBuffersPSP, RenderSceneBuffers);

public:
	Size2i internal_size;
	RID render_target;

	void configure(const RenderSceneBuffersConfiguration *p_config) override {
		internal_size = p_config->get_internal_size();
		render_target = p_config->get_render_target();
	}
	void set_fsr_sharpness(float p_fsr_sharpness) override {}
	void set_texture_mipmap_bias(float p_texture_mipmap_bias) override {}
	void set_anisotropic_filtering_level(RSE::ViewportAnisotropicFiltering p_anisotropic_filtering_level) override {}
	void set_use_debanding(bool p_use_debanding) override {}
};

// 3D sahne çizimi (PSP GE). Dummy'den: sky/SDFGI/GI/decal vb. stub'lar miras.
class RasterizerScenePSP : public RasterizerSceneDummy {
public:
	void render_scene(const Ref<RenderSceneBuffers> &p_render_buffers, const CameraData *p_camera_data, const CameraData *p_prev_camera_data, const PagedArray<RenderGeometryInstance *> &p_instances, const PagedArray<RID> &p_lights, const PagedArray<RID> &p_reflection_probes, const PagedArray<RID> &p_voxel_gi_instances, const PagedArray<RID> &p_decals, const PagedArray<RID> &p_lightmaps, const PagedArray<RID> &p_fog_volumes, RID p_environment, RID p_camera_attributes, RID p_compositor, RID p_shadow_atlas, RID p_occluder_debug_tex, RID p_reflection_atlas, RID p_reflection_probe, int p_reflection_probe_pass, float p_screen_mesh_lod_threshold, const RenderShadowData *p_render_shadows, int p_render_shadow_count, const RenderSDFGIData *p_render_sdfgi_regions, int p_render_sdfgi_region_count, float p_window_output_max_value, const RenderSDFGIUpdateData *p_sdfgi_update_data = nullptr, RenderingServerTypes::RenderInfo *r_render_info = nullptr) override;

	Ref<RenderSceneBuffers> render_buffers_create() override;

private:
	Color _background_color(RID p_environment) const;
};
