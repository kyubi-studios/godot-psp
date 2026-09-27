#pragma once

#include "servers/rendering/dummy/rasterizer_scene_dummy.h"

#include "core/templates/local_vector.h"
#include "core/templates/paged_allocator.h"
#include "servers/rendering/renderer_geometry_instance.h"
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
	// Nesne başına en fazla GE ışığı (donanım 4 ışık yuvası).
	static constexpr int MAX_LIGHTS = 4;

	class GeometryInstancePSP : public RenderGeometryInstanceBase {
	public:
		RID paired_lights[MAX_LIGHTS];
		int paired_light_count = 0;

		void _mark_dirty() override {}
		void set_use_lightmap(RID p_lightmap_instance, const Rect2 &p_lightmap_uv_scale, int p_lightmap_slice_index) override {}
		void set_lightmap_capture(const Color *p_sh9) override {}
		void pair_light_instance(const RID p_light_instance, RSE::LightType p_light_type, uint32_t p_placement_idx) override;
		void clear_light_instances() override { paired_light_count = 0; }
		void pair_reflection_probe_instances(const RID *p_reflection_probe_instances, uint32_t p_reflection_probe_instance_count) override {}
		void pair_decal_instances(const RID *p_decal_instances, uint32_t p_decal_instance_count) override {}
		void pair_voxel_gi_instances(const RID *p_voxel_gi_instances, uint32_t p_voxel_gi_instance_count) override {}
		void set_softshadow_projector_pairing(bool p_softshadow, bool p_projector) override {}
	};

	RenderGeometryInstance *geometry_instance_create(RID p_base) override;
	void geometry_instance_free(RenderGeometryInstance *p_geometry_instance) override;
	uint32_t geometry_instance_get_pair_mask() override;
	uint32_t get_max_lights_total() override { return 64; }
	uint32_t get_max_lights_per_mesh() override { return MAX_LIGHTS; }

	void render_scene(const Ref<RenderSceneBuffers> &p_render_buffers, const CameraData *p_camera_data, const CameraData *p_prev_camera_data, const PagedArray<RenderGeometryInstance *> &p_instances, const PagedArray<RID> &p_lights, const PagedArray<RID> &p_reflection_probes, const PagedArray<RID> &p_voxel_gi_instances, const PagedArray<RID> &p_decals, const PagedArray<RID> &p_lightmaps, const PagedArray<RID> &p_fog_volumes, RID p_environment, RID p_camera_attributes, RID p_compositor, RID p_shadow_atlas, RID p_occluder_debug_tex, RID p_reflection_atlas, RID p_reflection_probe, int p_reflection_probe_pass, float p_screen_mesh_lod_threshold, const RenderShadowData *p_render_shadows, int p_render_shadow_count, const RenderSDFGIData *p_render_sdfgi_regions, int p_render_sdfgi_region_count, float p_window_output_max_value, const RenderSDFGIUpdateData *p_sdfgi_update_data = nullptr, RenderingServerTypes::RenderInfo *r_render_info = nullptr) override;

	Ref<RenderSceneBuffers> render_buffers_create() override;

private:
	PagedAllocator<GeometryInstancePSP> geometry_instance_alloc_psp;

	// Çizim listesi girdisi (kare başına yeniden kullanılır, ayırma yok).
	struct DrawItem {
		GeometryInstancePSP *instance;
		const void *surface; // RendererPSP::Surface
		const void *material; // RendererPSP::Material (nullptr = varsayılan beyaz)
		uint32_t flags;
		float depth;
	};
	LocalVector<DrawItem> opaque_list;
	LocalVector<DrawItem> alpha_list;

	Color _background_color(RID p_environment) const;
	void _fill_lists(const PagedArray<RenderGeometryInstance *> &p_instances, const Transform3D &p_camera);
	void _draw_item(const DrawItem &p_item);
};
