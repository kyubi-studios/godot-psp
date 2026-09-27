#include "light_storage_psp.h"

using namespace RendererPSP;

void LightStorage::_light_initialize(RID p_rid, RSE::LightType p_type) {
	Light light;
	light.type = p_type;
	light.param[RSE::LIGHT_PARAM_ENERGY] = 1.0f;
	light.param[RSE::LIGHT_PARAM_INDIRECT_ENERGY] = 1.0f;
	light.param[RSE::LIGHT_PARAM_RANGE] = 1.0f;
	light.param[RSE::LIGHT_PARAM_ATTENUATION] = 1.0f;
	light.param[RSE::LIGHT_PARAM_SPOT_ANGLE] = 45.0f;
	light.param[RSE::LIGHT_PARAM_SPOT_ATTENUATION] = 1.0f;
	light_owner.initialize_rid(p_rid, light);
}

void LightStorage::light_free(RID p_rid) {
	Light *light = light_owner.get_or_null(p_rid);
	ERR_FAIL_NULL(light);
	light->dependency.deleted_notify(p_rid);
	light_owner.free(p_rid);
}

void LightStorage::light_set_color(RID p_light, const Color &p_color) {
	Light *light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);
	light->color = p_color;
}

void LightStorage::light_set_param(RID p_light, RSE::LightParam p_param, float p_value) {
	Light *light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);
	ERR_FAIL_INDEX(p_param, RSE::LIGHT_PARAM_MAX);
	light->param[p_param] = p_value;
	if (p_param == RSE::LIGHT_PARAM_RANGE || p_param == RSE::LIGHT_PARAM_SPOT_ANGLE) {
		light->version++;
		light->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_LIGHT);
	}
}

void LightStorage::light_set_negative(RID p_light, bool p_enable) {
	Light *light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);
	light->negative = p_enable;
}

void LightStorage::light_set_cull_mask(RID p_light, uint32_t p_mask) {
	Light *light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);
	light->cull_mask = p_mask;
	light->version++;
	light->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_LIGHT);
}

RSE::LightType LightStorage::light_get_type(RID p_light) const {
	Light *light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL_V(light, RSE::LIGHT_DIRECTIONAL);
	return light->type;
}

AABB LightStorage::light_get_aabb(RID p_light) const {
	Light *light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL_V(light, AABB());
	const float range = light->param[RSE::LIGHT_PARAM_RANGE];
	switch (light->type) {
		case RSE::LIGHT_OMNI:
			return AABB(-Vector3(range, range, range), Vector3(range, range, range) * 2.0f);
		case RSE::LIGHT_SPOT: {
			const float size = Math::tan(Math::deg_to_rad(light->param[RSE::LIGHT_PARAM_SPOT_ANGLE])) * range;
			return AABB(Vector3(-size, -size, -range), Vector3(size * 2.0f, size * 2.0f, range));
		}
		default:
			return AABB();
	}
}

float LightStorage::light_get_param(RID p_light, RSE::LightParam p_param) {
	Light *light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL_V(light, 0.0f);
	ERR_FAIL_INDEX_V(p_param, RSE::LIGHT_PARAM_MAX, 0.0f);
	return light->param[p_param];
}

Color LightStorage::light_get_color(RID p_light) {
	Light *light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL_V(light, Color());
	return light->color;
}

uint64_t LightStorage::light_get_version(RID p_light) const {
	Light *light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL_V(light, 0);
	return light->version;
}

uint32_t LightStorage::light_get_cull_mask(RID p_light) const {
	Light *light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL_V(light, 0);
	return light->cull_mask;
}

RID LightStorage::light_instance_create(RID p_light) {
	LightInstance li;
	li.light = p_light;
	return light_instance_owner.make_rid(li);
}

void LightStorage::light_instance_free(RID p_light_instance) {
	if (light_instance_owner.owns(p_light_instance)) {
		light_instance_owner.free(p_light_instance);
	}
}

void LightStorage::light_instance_set_transform(RID p_light_instance, const Transform3D &p_transform) {
	LightInstance *li = light_instance_owner.get_or_null(p_light_instance);
	ERR_FAIL_NULL(li);
	li->transform = p_transform;
}

LightStorage::LightStorage() {
	psp_singleton = this;
}

LightStorage::~LightStorage() {
	psp_singleton = nullptr;
}
