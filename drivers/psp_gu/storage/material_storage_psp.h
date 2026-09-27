#pragma once

#include "core/templates/rid_owner.h"
#include "servers/rendering/dummy/storage/material_storage.h"

namespace RendererPSP {

// Shader'lar derlenmez (ShaderLanguage PSP'de pahalı): koddan yalnızca GE'nin karşılayabildiği bayraklar
// basit metin aramasıyla çıkarılır. BaseMaterial3D'nin ürettiği kod için tasarlanmıştır; özel shader'lar
// beyaz albedo ile (uyarı vererek) çizilir.
enum ShaderFlags : uint32_t {
	SHADER_UNSHADED = 1 << 0,
	SHADER_CULL_FRONT = 1 << 1,
	SHADER_CULL_DISABLED = 1 << 2,
	SHADER_BLEND_ALPHA = 1 << 3, // saydam: alfa karışımı, arkadan öne sıralı
	SHADER_BLEND_ADD = 1 << 4,
	SHADER_ALPHA_SCISSOR = 1 << 5, // alfa testi
	SHADER_VERTEX_COLOR = 1 << 6,
	SHADER_FILTER_NEAREST = 1 << 7,
	SHADER_DEPTH_DRAW_NEVER = 1 << 8,
	SHADER_CUSTOM = 1 << 9, // BaseMaterial3D kodu değil
};

uint32_t parse_shader_flags(const String &p_code);

struct Shader {
	uint32_t flags = 0;
};

struct Material {
	RID shader;
	Color albedo = Color(1, 1, 1, 1);
	RID albedo_texture;
	Vector2 uv_scale = Vector2(1, 1);
	Vector2 uv_offset;
	float alpha_scissor = 0.5f;
	int render_priority = 0;
};

class MaterialStorage : public RendererDummy::MaterialStorage {
	static inline MaterialStorage *psp_singleton = nullptr;
	mutable RID_Owner<Shader, true> shader_owner;
	mutable RID_Owner<Material, true> material_owner;

public:
	static MaterialStorage *get_psp_singleton() { return psp_singleton; }

	bool owns_shader(RID p_rid) const { return shader_owner.owns(p_rid); }
	bool owns_material(RID p_rid) const { return material_owner.owns(p_rid); }
	Material *get_material(RID p_rid) const { return material_owner.get_or_null(p_rid); }
	uint32_t get_material_flags(const Material *p_material) const;

	RID shader_allocate() override;
	void shader_initialize(RID p_rid, bool p_embedded) override;
	void shader_free(RID p_rid) override;
	void shader_set_code(RID p_shader, const String &p_code) override;
	void get_shader_parameter_list(RID p_shader, List<PropertyInfo> *p_param_list) const override {}

	RID material_allocate() override;
	void material_initialize(RID p_rid) override;
	void material_free(RID p_rid) override;
	void material_set_render_priority(RID p_material, int p_priority) override;
	void material_set_shader(RID p_material, RID p_shader) override;
	void material_set_param(RID p_material, const StringName &p_param, const Variant &p_value) override;
	Variant material_get_param(RID p_material, const StringName &p_param) const override;
	void material_set_next_pass(RID p_material, RID p_next_material) override {}
	RSE::CullMode material_get_cull_mode(RID p_material) const override;
	void material_get_instance_shader_parameters(RID p_material, List<InstanceShaderParam> *r_parameters) override {}

	MaterialStorage();
	~MaterialStorage();
};

} // namespace RendererPSP
