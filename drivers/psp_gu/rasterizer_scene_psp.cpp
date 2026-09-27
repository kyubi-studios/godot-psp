#include "rasterizer_scene_psp.h"

#include "psp_gu.h"


#include "servers/rendering/rendering_server_globals.h"
#include "servers/rendering/storage/texture_storage.h"

Ref<RenderSceneBuffers> RasterizerScenePSP::render_buffers_create() {
	Ref<RenderSceneBuffersPSP> rb;
	rb.instantiate();
	return rb;
}

Color RasterizerScenePSP::_background_color(RID p_environment) const {
	Color c = RSG::texture_storage->get_default_clear_color();
	if (p_environment.is_valid() && is_environment(p_environment)) {
		RSE::EnvironmentBG bg = environment_get_background(p_environment);
		if (bg == RSE::ENV_BG_COLOR) {
			c = environment_get_bg_color(p_environment);
		}
	}
	return c;
}

void RasterizerScenePSP::render_scene(const Ref<RenderSceneBuffers> &p_render_buffers, const CameraData *p_camera_data, const CameraData *p_prev_camera_data, const PagedArray<RenderGeometryInstance *> &p_instances, const PagedArray<RID> &p_lights, const PagedArray<RID> &p_reflection_probes, const PagedArray<RID> &p_voxel_gi_instances, const PagedArray<RID> &p_decals, const PagedArray<RID> &p_lightmaps, const PagedArray<RID> &p_fog_volumes, RID p_environment, RID p_camera_attributes, RID p_compositor, RID p_shadow_atlas, RID p_occluder_debug_tex, RID p_reflection_atlas, RID p_reflection_probe, int p_reflection_probe_pass, float p_screen_mesh_lod_threshold, const RenderShadowData *p_render_shadows, int p_render_shadow_count, const RenderSDFGIData *p_render_sdfgi_regions, int p_render_sdfgi_region_count, float p_window_output_max_value, const RenderSDFGIUpdateData *p_sdfgi_update_data, RenderingServerTypes::RenderInfo *r_render_info) {
	if (!PSPGU::in_frame()) {
		return;
	}
	PSPGU::stats.scenes++;
	Color bg = _background_color(p_environment);
	PSPGU::clear(PSPGU::color_to_abgr(bg.r, bg.g, bg.b, 1.0f));
	// Viewport'un bekleyen temizleme isteği 3D'den sonra uygulanır; sahneyi silmesin diye tüketiyoruz (GLES3 gibi).
	Ref<RenderSceneBuffersPSP> rb = p_render_buffers;
	if (rb.is_valid() && rb->render_target.is_valid()) {
		RSG::texture_storage->render_target_disable_clear_request(rb->render_target);
	}
}
