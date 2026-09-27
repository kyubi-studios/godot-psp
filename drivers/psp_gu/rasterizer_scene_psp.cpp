#include "rasterizer_scene_psp.h"

#include "psp_gu.h"
#include "storage/light_storage_psp.h"
#include "storage/material_storage_psp.h"
#include "storage/mesh_storage_psp.h"
#include "storage/texture_storage_psp.h"

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
	if (p_placement_idx >= (uint32_t)MAX_LIGHTS) {
		return;
	}
	if (p_light_type == RSE::LIGHT_SPOT) {
		spot_lights[p_placement_idx] = p_light_instance;
		spot_count = MAX(spot_count, (int)p_placement_idx + 1);
	} else if (p_light_type == RSE::LIGHT_OMNI) {
		omni_lights[p_placement_idx] = p_light_instance;
		omni_count = MAX(omni_count, (int)p_placement_idx + 1);
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

/* LIGHTS / ENVIRONMENT */

// Godot'un ön yüz sarımı (ekran uzayında) GE'nin GU_CW/GU_CCW ayarına karşılığı.
static constexpr int GODOT_FRONT_FACE = GU_CW;

bool RasterizerScenePSP::_make_gu_light(RID p_light_instance, GULight &r_light) const {
	LightStorage *lights = LightStorage::get_psp_singleton();
	const LightInstance *li = lights->get_light_instance(p_light_instance);
	if (!li) {
		return false;
	}
	const Light *light = lights->get_light(li->light);
	if (!light) {
		return false;
	}
	float energy = light->param[RSE::LIGHT_PARAM_ENERGY];
	if (light->negative) {
		energy = 0.0f; // GE eksi ışık desteklemez.
	}
	const Color c = light->color * energy;
	r_light.color = PSPGU::color_to_abgr(c.r, c.g, c.b, 1.0f);
	const Vector3 origin = li->transform.origin;
	const Vector3 forward = -li->transform.basis.get_column(2).normalized();
	const float range = MAX(light->param[RSE::LIGHT_PARAM_RANGE], 0.001f);
	r_light.att[0] = 1.0f;
	r_light.att[1] = 0.0f;
	r_light.att[2] = 4.0f / (range * range); // menzilin sonunda ~%20
	r_light.dir[0] = forward.x;
	r_light.dir[1] = forward.y;
	r_light.dir[2] = forward.z;
	r_light.spot_exponent = 0.0f;
	r_light.spot_cutoff = -1.0f;
	switch (light->type) {
		case RSE::LIGHT_DIRECTIONAL:
			r_light.type = GU_DIRECTIONAL;
			r_light.pos[0] = -forward.x;
			r_light.pos[1] = -forward.y;
			r_light.pos[2] = -forward.z;
			break;
		case RSE::LIGHT_SPOT:
			r_light.type = GU_SPOTLIGHT;
			r_light.pos[0] = origin.x;
			r_light.pos[1] = origin.y;
			r_light.pos[2] = origin.z;
			r_light.spot_exponent = light->param[RSE::LIGHT_PARAM_SPOT_ATTENUATION];
			r_light.spot_cutoff = Math::cos(Math::deg_to_rad(light->param[RSE::LIGHT_PARAM_SPOT_ANGLE]));
			break;
		default:
			r_light.type = GU_POINTLIGHT;
			r_light.pos[0] = origin.x;
			r_light.pos[1] = origin.y;
			r_light.pos[2] = origin.z;
			break;
	}
	return true;
}

void RasterizerScenePSP::_apply_lights(const GeometryInstancePSP *p_instance) {
	int count = 0;
	for (int i = 0; i < directional_light_count && count < MAX_LIGHTS; i++) {
		const GULight &l = directional_lights[i];
		ScePspFVector3 pos = { l.pos[0], l.pos[1], l.pos[2] };
		sceGuLight(count, l.type, GU_DIFFUSE, &pos);
		sceGuLightColor(count, GU_DIFFUSE, l.color);
		sceGuLightColor(count, GU_AMBIENT, 0);
		sceGuLightAtt(count, 1.0f, 0.0f, 0.0f);
		sceGuEnable(GU_LIGHT0 + count);
		count++;
	}
	// Omni + spot adayları (her biri RendererSceneCull'ın en iyi 4'ü): nesne merkezindeki yaklaşık katkıya göre
	// en iyi (4 - directional) tanesi seçilir.
	struct Candidate {
		GULight light;
		float score;
	};
	Candidate candidates[MAX_LIGHTS * 2];
	int ncand = 0;
	const Vector3 center = p_instance->transformed_aabb.get_center();
	const RID *lists[2] = { p_instance->omni_lights, p_instance->spot_lights };
	const int counts[2] = { p_instance->omni_count, p_instance->spot_count };
	for (int k = 0; k < 2; k++) {
		for (int i = 0; i < counts[k]; i++) {
			Candidate &c = candidates[ncand];
			if (!_make_gu_light(lists[k][i], c.light) || c.light.type == GU_DIRECTIONAL) {
				continue;
			}
			const float d2 = center.distance_squared_to(Vector3(c.light.pos[0], c.light.pos[1], c.light.pos[2]));
			const uint32_t col = c.light.color;
			const float intensity = ((col & 0xff) + ((col >> 8) & 0xff) + ((col >> 16) & 0xff)) / 765.0f;
			c.score = intensity / (c.light.att[0] + c.light.att[2] * d2);
			ncand++;
		}
	}
	while (count < MAX_LIGHTS && ncand > 0) {
		int best = 0;
		for (int i = 1; i < ncand; i++) {
			if (candidates[i].score > candidates[best].score) {
				best = i;
			}
		}
		const GULight &l = candidates[best].light;
		ScePspFVector3 pos = { l.pos[0], l.pos[1], l.pos[2] };
		sceGuLight(count, l.type, GU_DIFFUSE, &pos);
		sceGuLightColor(count, GU_DIFFUSE, l.color);
		sceGuLightColor(count, GU_AMBIENT, 0);
		sceGuLightAtt(count, l.att[0], l.att[1], l.att[2]);
		if (l.type == GU_SPOTLIGHT) {
			ScePspFVector3 dir = { l.dir[0], l.dir[1], l.dir[2] };
			sceGuLightSpot(count, &dir, l.spot_exponent, l.spot_cutoff);
		}
		sceGuEnable(GU_LIGHT0 + count);
		count++;
		candidates[best] = candidates[--ncand];
	}
	for (int i = count; i < MAX_LIGHTS; i++) {
		sceGuDisable(GU_LIGHT0 + i);
	}
}

void RasterizerScenePSP::_setup_environment(RID p_environment, const CameraData *p_camera_data) {
	Color ambient(0.2f, 0.2f, 0.2f);
	bool fog = false;
	float fog_near = 0.0f, fog_far = 1.0f;
	Color fog_color;
	if (p_environment.is_valid() && is_environment(p_environment)) {
		const float energy = environment_get_ambient_light_energy(p_environment);
		switch (environment_get_ambient_source(p_environment)) {
			case RSE::ENV_AMBIENT_SOURCE_COLOR:
				ambient = environment_get_ambient_light(p_environment) * energy;
				break;
			case RSE::ENV_AMBIENT_SOURCE_DISABLED:
				ambient = Color(0, 0, 0);
				break;
			case RSE::ENV_AMBIENT_SOURCE_BG:
				ambient = (environment_get_background(p_environment) == RSE::ENV_BG_COLOR ? environment_get_bg_color(p_environment) : ambient) * energy;
				break;
			default:
				break;
		}
		if (environment_get_fog_enabled(p_environment)) {
			fog = true;
			fog_color = environment_get_fog_light_color(p_environment);
			const float zfar = p_camera_data->main_projection.get_z_far();
			if (environment_get_fog_mode(p_environment) == RSE::ENV_FOG_MODE_DEPTH) {
				fog_near = environment_get_fog_depth_begin(p_environment);
				const float end = environment_get_fog_depth_end(p_environment);
				fog_far = end > 0.0f ? end : zfar;
			} else {
				// Üstel fog'un doğrusal yaklaşımı: yoğunluğun %95'e ulaştığı mesafe.
				const float density = MAX(environment_get_fog_density(p_environment), 0.0001f);
				fog_near = 0.0f;
				fog_far = MIN(3.0f / density, zfar);
			}
			fog_far = MAX(fog_far, fog_near + 0.01f);
		}
	}
	ambient_abgr = PSPGU::color_to_abgr(ambient.r, ambient.g, ambient.b, 1.0f);
	sceGuAmbient(ambient_abgr);
	sceGuLightMode(0);
	if (fog) {
		sceGuFog(fog_near, fog_far, PSPGU::color_to_abgr(fog_color.r, fog_color.g, fog_color.b, 1.0f));
		sceGuEnable(GU_FOG);
	} else {
		sceGuDisable(GU_FOG);
	}
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

	// En kötü durum: matris + ışıklar + texture + birkaç çizim komutu (~1 KB).
	PSPGU::ensure_list_space(1024);

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

	const bool has_vertex_color = (surf.vertex_type & GU_COLOR_8888) != 0;
	bool unshaded = (flags & SHADER_UNSHADED) != 0;
	if (!unshaded && !(surf.vertex_type & GU_NORMAL_8BIT)) {
		// Normali olmayan yüzey GE'de tanımsız ışıklanır; ışıksız (albedo) çiz.
		WARN_PRINT_ONCE("PSP: a lit surface has no normals; it is drawn unshaded.");
		unshaded = true;
	}
	if (unshaded) {
		if (has_vertex_color && !(flags & SHADER_VERTEX_COLOR)) {
			// Işıksız modda GE rengi vertex renginden alır; materyal vertex rengini kullanmıyorsa albedo'yu
			// "ışıklı, ışıksız, beyaz ambient" ile çıkar: renk = ambient(beyaz) * materyal ambient(albedo).
			sceGuEnable(GU_LIGHTING);
			for (int i = 0; i < MAX_LIGHTS; i++) {
				sceGuDisable(GU_LIGHT0 + i);
			}
			sceGuAmbient(0xffffffff);
			lit_instance = nullptr;
		} else {
			sceGuDisable(GU_LIGHTING);
		}
	} else {
		sceGuEnable(GU_LIGHTING);
		sceGuAmbient(ambient_abgr);
		if (lit_instance != p_item.instance) {
			_apply_lights(p_item.instance);
			lit_instance = p_item.instance;
		}
	}
	sceGuColorMaterial((flags & SHADER_VERTEX_COLOR) && has_vertex_color ? (GU_AMBIENT | GU_DIFFUSE) : 0);
	const TextureStorage::Texture *tex = nullptr;
	if (mat && mat->albedo_texture.is_valid()) {
		tex = static_cast<TextureStorage *>(RSG::texture_storage)->get_psp_texture(mat->albedo_texture);
		if (tex && !tex->data.pixels) {
			tex = nullptr;
		}
	}
	if (tex) {
		const PSPTextureData &td = tex->data;
		if (bound_texture != td.pixels) {
			sceGuTexMode(td.psm, 0, 0, td.swizzled ? GU_TRUE : GU_FALSE);
			sceGuTexImage(0, td.width, td.height, td.width, PSPGU::texture_address(td.pixels, td.bytes));
			sceGuTexFlush();
			bound_texture = td.pixels;
		}
		sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
		const int filter = (flags & SHADER_FILTER_NEAREST) ? GU_NEAREST : GU_LINEAR;
		sceGuTexFilter(filter, filter);
		sceGuTexWrap(GU_REPEAT, GU_REPEAT);
		// Vertex UV'si [0,2) aralığında (u16/32768): gerçek UV = uv_min + raw * range/2, ardından material uv1.
		const Vector2 scale = surf.uv_range * 0.5f * mat->uv_scale;
		const Vector2 offset = surf.uv_min * mat->uv_scale + mat->uv_offset;
		sceGuTexScale(scale.x, scale.y);
		sceGuTexOffset(offset.x, offset.y);
		sceGuEnable(GU_TEXTURE_2D);
	} else {
		sceGuDisable(GU_TEXTURE_2D);
	}
	if (flags & SHADER_ALPHA_SCISSOR) {
		const int ref = CLAMP((int)(mat->alpha_scissor * 255.0f), 0, 255);
		sceGuAlphaFunc(GU_GREATER, ref, 0xff);
		sceGuEnable(GU_ALPHA_TEST);
	} else {
		sceGuDisable(GU_ALPHA_TEST);
	}

	if (flags & SHADER_CULL_DISABLED) {
		sceGuDisable(GU_CULL_FACE);
	} else {
		// Arka yüz kırpma; cull_front ve yansıtılmış dönüşüm (negatif determinant) sarımı çevirir.
		const bool flip = ((flags & SHADER_CULL_FRONT) != 0) != p_item.instance->mirror;
		sceGuFrontFace(flip ? (GODOT_FRONT_FACE == GU_CW ? GU_CCW : GU_CW) : GODOT_FRONT_FACE);
		sceGuEnable(GU_CULL_FACE);
	}

	// GE prim komutunun sayı alanı 16 bit: büyük yüzeyler parçalanır (üçgen listesi için 3'ün katı).
	const int total = surf.index_count > 0 ? surf.index_count : surf.vertex_count;
	const int chunk = RendererPSP::draw_chunk_size(surf.primitive, total);
	if (chunk <= 0) {
		WARN_PRINT_ONCE("PSP: a surface is too large for a single GE draw and cannot be split (strip/fan > 65535); skipped.");
		return;
	}
	for (int start = 0; start < total; start += chunk) {
		const int n = MIN(chunk, total - start);
		if (start > 0) {
			PSPGU::ensure_list_space(64);
		}
		if (surf.index_count > 0) {
			sceGuDrawArray(surf.primitive, surf.vertex_type | GU_TRANSFORM_3D, n, surf.indices + start, surf.vertices);
		} else {
			sceGuDrawArray(surf.primitive, surf.vertex_type | GU_TRANSFORM_3D, n, nullptr, (const uint8_t *)surf.vertices + (size_t)start * surf.stride);
		}
	}
	PSPGU::stats.draws_accum++;
}

void RasterizerScenePSP::render_scene(const Ref<RenderSceneBuffers> &p_render_buffers, const CameraData *p_camera_data, const CameraData *p_prev_camera_data, const PagedArray<RenderGeometryInstance *> &p_instances, const PagedArray<RID> &p_lights, const PagedArray<RID> &p_reflection_probes, const PagedArray<RID> &p_voxel_gi_instances, const PagedArray<RID> &p_decals, const PagedArray<RID> &p_lightmaps, const PagedArray<RID> &p_fog_volumes, RID p_environment, RID p_camera_attributes, RID p_compositor, RID p_shadow_atlas, RID p_occluder_debug_tex, RID p_reflection_atlas, RID p_reflection_probe, int p_reflection_probe_pass, float p_screen_mesh_lod_threshold, const RenderShadowData *p_render_shadows, int p_render_shadow_count, const RenderSDFGIData *p_render_sdfgi_regions, int p_render_sdfgi_region_count, float p_window_output_max_value, const RenderSDFGIUpdateData *p_sdfgi_update_data, RenderingServerTypes::RenderInfo *r_render_info) {
	if (!PSPGU::in_frame()) {
		return;
	}
	PSPGU::stats.scenes++;
	// 2D çizimi derinliği/kırpmayı kapatmış olabilir.
	sceGuEnable(GU_DEPTH_TEST);
	sceGuDepthFunc(GU_GEQUAL);
	sceGuDepthMask(GU_FALSE);

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

	_setup_environment(p_environment, p_camera_data);
	directional_light_count = 0;
	LightStorage *lights = LightStorage::get_psp_singleton();
	for (uint32_t i = 0; i < p_lights.size() && directional_light_count < MAX_LIGHTS; i++) {
		const LightInstance *li = lights->get_light_instance(p_lights[i]);
		const Light *light = li ? lights->get_light(li->light) : nullptr;
		if (light && light->type == RSE::LIGHT_DIRECTIONAL && _make_gu_light(p_lights[i], directional_lights[directional_light_count])) {
			directional_light_count++;
		}
	}

	_fill_lists(p_instances, p_camera_data->main_transform);
	bound_texture = nullptr;
	lit_instance = nullptr;

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
