#include "material_storage_psp.h"

using namespace RendererPSP;

uint32_t RendererPSP::parse_shader_flags(const String &p_code) {
	uint32_t flags = 0;
	const int rm = p_code.find("render_mode");
	if (rm >= 0) {
		const int end = p_code.find(";", rm);
		const String modes = p_code.substr(rm + 11, end > rm ? end - rm - 11 : -1);
		const Vector<String> list = modes.split(",");
		for (const String &m_raw : list) {
			const String m = m_raw.strip_edges();
			if (m == "unshaded") {
				flags |= SHADER_UNSHADED;
			} else if (m == "cull_front") {
				flags |= SHADER_CULL_FRONT;
			} else if (m == "cull_disabled") {
				flags |= SHADER_CULL_DISABLED;
			} else if (m == "blend_add") {
				flags |= SHADER_BLEND_ADD;
			} else if (m == "depth_draw_never") {
				flags |= SHADER_DEPTH_DRAW_NEVER;
			}
		}
	}
	if (p_code.contains("ALPHA_SCISSOR_THRESHOLD")) {
		flags |= SHADER_ALPHA_SCISSOR;
	} else if (p_code.contains("ALPHA *=") || p_code.contains("ALPHA =")) {
		flags |= SHADER_BLEND_ALPHA;
	}
	if (p_code.contains("albedo_tex *= COLOR")) {
		flags |= SHADER_VERTEX_COLOR;
	}
	const int ta = p_code.find("uniform sampler2D texture_albedo");
	if (ta >= 0) {
		const int line_end = p_code.find("\n", ta);
		const String line = p_code.substr(ta, line_end > ta ? line_end - ta : -1);
		if (line.contains("filter_nearest")) {
			flags |= SHADER_FILTER_NEAREST;
		}
	}
	if (!p_code.contains("ALBEDO = albedo.rgb")) {
		flags |= SHADER_CUSTOM;
	}
	return flags;
}

uint32_t MaterialStorage::get_material_flags(const Material *p_material) const {
	Shader *shader = shader_owner.get_or_null(p_material->shader);
	return shader ? shader->flags : 0;
}

RID MaterialStorage::shader_allocate() {
	return shader_owner.allocate_rid();
}

void MaterialStorage::shader_initialize(RID p_rid, bool p_embedded) {
	shader_owner.initialize_rid(p_rid, Shader());
}

void MaterialStorage::shader_free(RID p_rid) {
	if (shader_owner.owns(p_rid)) {
		shader_owner.free(p_rid);
	}
}

void MaterialStorage::shader_set_code(RID p_shader, const String &p_code) {
	Shader *shader = shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL(shader);
	shader->flags = parse_shader_flags(p_code);
	if (shader->flags & SHADER_CUSTOM) {
		WARN_PRINT_ONCE("PSP: custom shaders are not supported; such materials are drawn with a white albedo.");
	}
}

RID MaterialStorage::material_allocate() {
	return material_owner.allocate_rid();
}

void MaterialStorage::material_initialize(RID p_rid) {
	material_owner.initialize_rid(p_rid, Material());
}

void MaterialStorage::material_free(RID p_rid) {
	if (material_owner.owns(p_rid)) {
		material_owner.free(p_rid);
	}
}

void MaterialStorage::material_set_render_priority(RID p_material, int p_priority) {
	Material *m = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL(m);
	m->render_priority = p_priority;
}

void MaterialStorage::material_set_shader(RID p_material, RID p_shader) {
	Material *m = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL(m);
	m->shader = p_shader;
}

void MaterialStorage::material_set_param(RID p_material, const StringName &p_param, const Variant &p_value) {
	Material *m = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL(m);
	// Yalnızca GE'nin kullanabildiği parametreler tutulur (bellek).
	static const StringName albedo = "albedo";
	static const StringName texture_albedo = "texture_albedo";
	static const StringName uv1_scale = "uv1_scale";
	static const StringName uv1_offset = "uv1_offset";
	static const StringName alpha_scissor_threshold = "alpha_scissor_threshold";
	if (p_param == albedo) {
		m->albedo = p_value;
	} else if (p_param == texture_albedo) {
		m->albedo_texture = p_value;
	} else if (p_param == uv1_scale) {
		Vector3 v = p_value;
		m->uv_scale = Vector2(v.x, v.y);
	} else if (p_param == uv1_offset) {
		Vector3 v = p_value;
		m->uv_offset = Vector2(v.x, v.y);
	} else if (p_param == alpha_scissor_threshold) {
		m->alpha_scissor = p_value;
	}
}

Variant MaterialStorage::material_get_param(RID p_material, const StringName &p_param) const {
	Material *m = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL_V(m, Variant());
	if (p_param == StringName("albedo")) {
		return m->albedo;
	} else if (p_param == StringName("texture_albedo")) {
		return m->albedo_texture;
	}
	return Variant();
}

RSE::CullMode MaterialStorage::material_get_cull_mode(RID p_material) const {
	Material *m = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL_V(m, RSE::CULL_MODE_BACK);
	const uint32_t f = get_material_flags(m);
	if (f & SHADER_CULL_DISABLED) {
		return RSE::CULL_MODE_DISABLED;
	}
	return (f & SHADER_CULL_FRONT) ? RSE::CULL_MODE_FRONT : RSE::CULL_MODE_BACK;
}

MaterialStorage::MaterialStorage() {
	psp_singleton = this;
}

MaterialStorage::~MaterialStorage() {
	psp_singleton = nullptr;
}
