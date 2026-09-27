#include "utilities_psp.h"

#include "light_storage_psp.h"
#include "material_storage_psp.h"
#include "mesh_storage_psp.h"
#include "texture_storage_psp.h"

#include "servers/rendering/rendering_server_globals.h"

using namespace RendererPSP;

RSE::InstanceType Utilities::get_base_type(RID p_rid) const {
	if (MeshStorage::get_psp_singleton()->owns_mesh(p_rid)) {
		return RSE::INSTANCE_MESH;
	} else if (LightStorage::get_psp_singleton()->owns_light(p_rid)) {
		return RSE::INSTANCE_LIGHT;
	}
	return RendererDummy::Utilities::get_base_type(p_rid);
}

bool Utilities::free(RID p_rid) {
	MeshStorage *meshes = MeshStorage::get_psp_singleton();
	MaterialStorage *materials = MaterialStorage::get_psp_singleton();
	TextureStorage *textures = static_cast<TextureStorage *>(RSG::texture_storage);
	LightStorage *lights = LightStorage::get_psp_singleton();
	if (meshes->owns_mesh(p_rid)) {
		meshes->mesh_free(p_rid);
		return true;
	} else if (lights->owns_light(p_rid)) {
		lights->light_free(p_rid);
		return true;
	} else if (materials->owns_material(p_rid)) {
		materials->material_free(p_rid);
		return true;
	} else if (materials->owns_shader(p_rid)) {
		materials->shader_free(p_rid);
		return true;
	} else if (textures->owns_render_target(p_rid)) {
		textures->render_target_free(p_rid);
		return true;
	}
	return RendererDummy::Utilities::free(p_rid);
}

void Utilities::base_update_dependency(RID p_base, DependencyTracker *p_instance) {
	Mesh *mesh = MeshStorage::get_psp_singleton()->get_mesh(p_base);
	if (mesh) {
		p_instance->update_dependency(&mesh->dependency);
		return;
	}
	Light *light = LightStorage::get_psp_singleton()->get_light(p_base);
	if (light) {
		p_instance->update_dependency(&light->dependency);
		return;
	}
	RendererDummy::Utilities::base_update_dependency(p_base, p_instance);
}
