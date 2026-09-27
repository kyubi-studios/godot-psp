#pragma once

#include "core/templates/rid_owner.h"
#include "servers/rendering/dummy/storage/light_storage.h"
#include "servers/rendering/storage/utilities.h"

namespace RendererPSP {

struct Light {
	RSE::LightType type = RSE::LIGHT_DIRECTIONAL;
	Color color = Color(1, 1, 1);
	float param[RSE::LIGHT_PARAM_MAX] = {};
	bool negative = false;
	uint32_t cull_mask = 0xFFFFFFFF;
	uint64_t version = 0;
	Dependency dependency;
};

struct LightInstance {
	RID light;
	Transform3D transform;
};

// GE ışıkları için: directional/omni/spot parametreleri ve dünya dönüşümleri. Gölge/projeksiyon yok.
// Probe, lightmap ve shadow atlas API'leri dummy'den (stub).
class LightStorage : public RendererDummy::LightStorage {
	static inline LightStorage *psp_singleton = nullptr;
	mutable RID_Owner<Light, true> light_owner;
	mutable RID_Owner<LightInstance> light_instance_owner;

	RID _light_allocate() { return light_owner.allocate_rid(); }
	void _light_initialize(RID p_rid, RSE::LightType p_type);

public:
	static LightStorage *get_psp_singleton() { return psp_singleton; }

	bool owns_light(RID p_rid) const { return light_owner.owns(p_rid); }
	Light *get_light(RID p_rid) const { return light_owner.get_or_null(p_rid); }
	LightInstance *get_light_instance(RID p_rid) const { return light_instance_owner.get_or_null(p_rid); }

	RID directional_light_allocate() override { return _light_allocate(); }
	void directional_light_initialize(RID p_rid) override { _light_initialize(p_rid, RSE::LIGHT_DIRECTIONAL); }
	RID omni_light_allocate() override { return _light_allocate(); }
	void omni_light_initialize(RID p_rid) override { _light_initialize(p_rid, RSE::LIGHT_OMNI); }
	RID spot_light_allocate() override { return _light_allocate(); }
	void spot_light_initialize(RID p_rid) override { _light_initialize(p_rid, RSE::LIGHT_SPOT); }
	void light_free(RID p_rid) override;

	void light_set_color(RID p_light, const Color &p_color) override;
	void light_set_param(RID p_light, RSE::LightParam p_param, float p_value) override;
	void light_set_negative(RID p_light, bool p_enable) override;
	void light_set_cull_mask(RID p_light, uint32_t p_mask) override;

	RSE::LightType light_get_type(RID p_light) const override;
	AABB light_get_aabb(RID p_light) const override;
	float light_get_param(RID p_light, RSE::LightParam p_param) override;
	Color light_get_color(RID p_light) override;
	uint64_t light_get_version(RID p_light) const override;
	uint32_t light_get_cull_mask(RID p_light) const override;

	RID light_instance_create(RID p_light) override;
	void light_instance_free(RID p_light_instance) override;
	void light_instance_set_transform(RID p_light_instance, const Transform3D &p_transform) override;

	LightStorage();
	~LightStorage();
};

} // namespace RendererPSP
