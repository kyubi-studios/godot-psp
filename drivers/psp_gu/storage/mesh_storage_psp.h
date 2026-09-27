#pragma once

#include "core/templates/local_vector.h"
#include "core/templates/rid_owner.h"
#include "servers/rendering/dummy/storage/mesh_storage.h"

namespace RendererPSP {

// GE'ye hazır bir mesh yüzeyi. Yükleme anında bir kez dönüştürülür; Godot'un dizileri saklanmaz.
// Vertex düzeni (GE sırası, her bileşen kendi boyutuna hizalı):
//   [u16 u, v] [u32 ABGR renk] [s8 nx, ny, nz, pad] [float x, y, z | s16 x, y, z]  (olmayan bileşenler atlanır)
// Normalli yüzeylerde pozisyon float'tır (ışık için model matrisinde ölçek olmaz). Normalsiz yüzeylerde
// pozisyon yüzey AABB'sinin merkezine/yarı boyutuna göre 16-bit normalize edilir (model matrisine eklenir);
// UV [uv_min, uv_min + uv_range] aralığına göre 0..65535'e (TexScale/TexOffset ile geri açılır).
struct Surface {
	void *vertices = nullptr; // memalign(16)
	uint16_t *indices = nullptr; // memalign(16), yoksa DrawArray indekssiz
	uint32_t vertex_type = 0; // GU_* vertex format bayrakları (GU_TRANSFORM_3D hariç)
	int vertex_count = 0;
	int index_count = 0;
	int primitive = 3; // RSE::PrimitiveType == GU primitive numarası
	uint32_t stride = 0; // vertex boyutu (byte)
	uint32_t bytes = 0;
	RID material;
	AABB aabb;
	Vector3 pos_center;
	Vector3 pos_half;
	Vector2 uv_min;
	Vector2 uv_range = Vector2(1, 1);
	uint64_t format = 0; // Godot yüzey formatı (mesh_get_surface için)
};

struct Mesh {
	LocalVector<Surface> surfaces;
	AABB aabb;
	AABB custom_aabb;
	int blend_shape_count = 0;
	RSE::BlendShapeMode blend_shape_mode = RSE::BLEND_SHAPE_MODE_NORMALIZED;
	Dependency dependency;
};

// GE prim komutu en fazla 65535 eleman çizer. Parça boyutu: listeler için primitive boyutunun katı;
// strip/fan bölünemez (0 = çizilemez).
int draw_chunk_size(int p_primitive, int p_count);

class MeshStorage : public RendererDummy::MeshStorage {
	static inline MeshStorage *psp_singleton = nullptr;
	mutable RID_Owner<Mesh, true> mesh_owner;
	uint32_t total_bytes = 0;

	static void _free_surface(Surface &p_surface);
	void _update_aabb(Mesh *p_mesh);

public:
	static MeshStorage *get_psp_singleton() { return psp_singleton; }

	bool owns_mesh(RID p_rid) const { return mesh_owner.owns(p_rid); }
	Mesh *get_mesh(RID p_rid) const { return mesh_owner.get_or_null(p_rid); }
	uint32_t get_total_bytes() const { return total_bytes; }

	// Test/araç: Godot dizilerinden GE yüzeyi kur (mesh_add_surface bunu kullanır).
	static bool build_surface(const Array &p_arrays, RSE::PrimitiveType p_primitive, const AABB &p_aabb, Surface &r_surface);

	RID mesh_allocate() override;
	void mesh_initialize(RID p_rid) override;
	void mesh_free(RID p_rid) override;
	void mesh_set_blend_shape_count(RID p_mesh, int p_blend_shape_count) override;
	bool mesh_needs_instance(RID p_mesh, bool p_has_skeleton) override { return false; }
	void mesh_add_surface(RID p_mesh, const RenderingServerTypes::SurfaceData &p_surface) override;
	int mesh_get_blend_shape_count(RID p_mesh) const override;
	void mesh_set_blend_shape_mode(RID p_mesh, RSE::BlendShapeMode p_mode) override;
	RSE::BlendShapeMode mesh_get_blend_shape_mode(RID p_mesh) const override;
	void mesh_surface_update_vertex_region(RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t> &p_data) override {}
	void mesh_surface_update_attribute_region(RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t> &p_data) override {}
	void mesh_surface_update_skin_region(RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t> &p_data) override {}
	void mesh_surface_update_index_region(RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t> &p_data) override {}
	void mesh_surface_set_material(RID p_mesh, int p_surface, RID p_material) override;
	RID mesh_surface_get_material(RID p_mesh, int p_surface) const override;
	RenderingServerTypes::SurfaceData mesh_get_surface(RID p_mesh, int p_surface) const override;
	int mesh_get_surface_count(RID p_mesh) const override;
	void mesh_set_custom_aabb(RID p_mesh, const AABB &p_aabb) override;
	AABB mesh_get_custom_aabb(RID p_mesh) const override;
	AABB mesh_get_aabb(RID p_mesh, RID p_skeleton = RID()) override;
	void mesh_surface_remove(RID p_mesh, int p_surface) override;
	void mesh_clear(RID p_mesh) override;

	MeshStorage();
	~MeshStorage();
};

} // namespace RendererPSP
