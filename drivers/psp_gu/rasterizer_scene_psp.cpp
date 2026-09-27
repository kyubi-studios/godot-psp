#include "rasterizer_scene_psp.h"

#include "psp_gu.h"
#include "storage/material_storage_psp.h"
#include "storage/mesh_storage_psp.h"

#include "servers/rendering/rendering_server_globals.h"
#include "servers/rendering/storage/texture_storage.h"

#include <pspgu.h>

using namespace RendererPSP;

// Godot Transform3D → GE 4x4 (sütun sıralı; ScePspFMatrix4.x/y/z/w = sütunlar).
static void _to_gu_matrix(const Transform3D &p_xf, ScePspFMatrix4 &r_m) {
	const Basis &b = p_xf.basis;
	r_m.x = { b.rows[0][0], b.rows[1][0], b.rows[2][0], 0.0f };
	r_m.y = { b.rows[0][1], b.rows[1][1], b.rows[2][1], 0.0f };
	r_m.z = { b.rows[0][2], b.rows[1][2], b.rows[2][2], 0.0f };
	r_m.w = { p_xf.origin.x, p_xf.origin.y, p_xf.origin.z, 1.0f };
}

static void _to_gu_matrix(const Projection &p_proj, ScePspFMatrix4 &r_m) {
	const Vector4 *c = p_proj.columns;
	r_m.x = { (float)c[0].x, (float)c[0].y, (float)c[0].z, (float)c[0].w };
	r_m.y = { (float)c[1].x, (float)c[1].y, (float)c[1].z, (float)c[1].w };
	r_m.z = { (float)c[2].x, (float)c[2].y, (float)c[2].z, (float)c[2].w };
	r_m.w = { (float)c[3].x, (float)c[3].y, (float)c[3].z, (float)c[3].w };
}

/* GEOMETRY INSTANCE */

void RasterizerScenePSP::GeometryInstancePSP::pair_light_instance(const RID p_light_instance, RSE::LightType p_light_type, uint32_t p_placement_idx) {
	if (paired_light_count < MAX_LIGHTS) {
		paired_lights[paired_light_count++] = p_light_instance;
	}
}

RenderGeometryInstance *RasterizerScenePSP::geometry_instance_create(RID p_base) {
	RSE::InstanceType type = RSG::utilities->get_base_type(p_base);
	ERR_FAIL_COND_V(!((1 << type) & RSE::INSTANCE_GEOMETRY_MASK), nullptr);
	GeometryInstancePSP *gi = geometry_instance_alloc_psp.alloc();
	gi->data = memnew(GeometryInstancePSP::Data);
	gi->data->base = p_base;
	gi->data->base_type = type;
	return gi;
}

void RasterizerScenePSP::geometry_instance_free(RenderGeometryInstance *p_geometry_instance) {
	GeometryInstancePSP *gi = static_cast<GeometryInstancePSP *>(p_geometry_instance);
	ERR_FAIL_NULL(gi);
	if (gi->data) {
		memdelete(gi->data);
		gi->data = nullptr;
	}
	geometry_instance_alloc_psp.free(gi);
}

uint32_t RasterizerScenePSP::geometry_instance_get_pair_mask() {
	return (1 << RSE::INSTANCE_LIGHT);
}

/* RENDER */

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

void RasterizerScenePSP::_fill_lists(const PagedArray<RenderGeometryInstance *> &p_instances, const Transform3D &p_camera) {
	opaque_list.clear();
	alpha_list.clear();
	MeshStorage *meshes = MeshStorage::get_psp_singleton();
	MaterialStorage *materials = MaterialStorage::get_psp_singleton();
	const Vector3 cam_pos = p_camera.origin;
	const Vector3 cam_dir = -p_camera.basis.get_column(2);

	for (uint32_t i = 0; i < p_instances.size(); i++) {
		GeometryInstancePSP *gi = static_cast<GeometryInstancePSP *>(p_instances[i]);
		if (gi->data->base_type != RSE::INSTANCE_MESH) {
			continue; // Multimesh/particles desteklenmiyor.
		}
		Mesh *mesh = meshes->get_mesh(gi->data->base);
		if (!mesh) {
			continue;
		}
		const float depth = cam_dir.dot(gi->transformed_aabb.get_center() - cam_pos);
		for (uint32_t s = 0; s < mesh->surfaces.size(); s++) {
			const Surface &surf = mesh->surfaces[s];
			if (!surf.vertices) {
				continue;
			}
			RID mat_rid = gi->data->material_override;
			if (mat_rid.is_null() && s < (uint32_t)gi->data->surface_materials.size()) {
				mat_rid = gi->data->surface_materials[s];
			}
			if (mat_rid.is_null()) {
				mat_rid = surf.material;
			}
			const Material *mat = materials->get_material(mat_rid);
			const uint32_t flags = mat ? materials->get_material_flags(mat) : 0;
			DrawItem item = { gi, &surf, mat, flags, depth };
			if (flags & (SHADER_BLEND_ALPHA | SHADER_BLEND_ADD)) {
				alpha_list.push_back(item);
			} else {
				opaque_list.push_back(item);
			}
		}
	}
	// Opaklar: material'e göre grupla (durum değişimi az); saydamlar: arkadan öne.
	struct OpaqueSort {
		bool operator()(const DrawItem &a, const DrawItem &b) const { return a.material < b.material; }
	};
	struct AlphaSort {
		bool operator()(const DrawItem &a, const DrawItem &b) const { return a.depth > b.depth; }
	};
	opaque_list.sort_custom<OpaqueSort>();
	alpha_list.sort_custom<AlphaSort>();
}

void RasterizerScenePSP::_draw_item(const DrawItem &p_item) {
	const Surface &surf = *(const Surface *)p_item.surface;
	const Material *mat = (const Material *)p_item.material;
	const uint32_t flags = p_item.flags;

	// Model matrisi: dünya * (yüzeyin 16-bit pozisyon ölçeği).
	Transform3D model = p_item.instance->transform * Transform3D(Basis::from_scale(surf.pos_half), surf.pos_center);
	ScePspFMatrix4 m;
	_to_gu_matrix(model, m);
	sceGuSetMatrix(GU_MODEL, &m);

	Color albedo = mat ? mat->albedo : Color(1, 1, 1, 1);
	if (flags & SHADER_CUSTOM) {
		albedo = Color(1, 1, 1, albedo.a);
	}
	sceGuColor(PSPGU::color_to_abgr(albedo.r, albedo.g, albedo.b, albedo.a));

	sceGuDisable(GU_LIGHTING);
	sceGuDisable(GU_TEXTURE_2D);
	sceGuDisable(GU_CULL_FACE);

	sceGuDrawArray(surf.primitive, surf.vertex_type | GU_TRANSFORM_3D, surf.index_count > 0 ? surf.index_count : surf.vertex_count, surf.indices, surf.vertices);
	PSPGU::stats.draws_accum++;
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

	ScePspFMatrix4 m;
	_to_gu_matrix(p_camera_data->main_projection, m);
	sceGuSetMatrix(GU_PROJECTION, &m);
	_to_gu_matrix(p_camera_data->main_transform.affine_inverse(), m);
	sceGuSetMatrix(GU_VIEW, &m);

	_fill_lists(p_instances, p_camera_data->main_transform);

	sceGuDepthMask(GU_FALSE); // GU: GU_FALSE = derinlik yazılır
	sceGuDisable(GU_BLEND);
	for (const DrawItem &item : opaque_list) {
		_draw_item(item);
	}
	if (!alpha_list.is_empty()) {
		sceGuEnable(GU_BLEND);
		sceGuDepthMask(GU_TRUE); // saydamlar derinlik yazmaz
		for (const DrawItem &item : alpha_list) {
			if (item.flags & SHADER_BLEND_ADD) {
				sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_FIX, 0, 0xffffffff);
			} else {
				sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
			}
			_draw_item(item);
		}
		sceGuDepthMask(GU_FALSE);
		sceGuDisable(GU_BLEND);
	}
}
