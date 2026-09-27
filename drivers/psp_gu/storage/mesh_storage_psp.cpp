#include "mesh_storage_psp.h"

#include "servers/rendering/rendering_server.h"

#include <malloc.h>
#include <pspgu.h>
#include <pspkernel.h>

using namespace RendererPSP;

static inline int16_t _to_s16(float p_v) {
	// GE 16-bit pozisyonu /32768 ile çözer.
	int v = (int)Math::round(p_v * 32768.0f);
	return (int16_t)CLAMP(v, -32768, 32767);
}

static inline int8_t _to_s8(float p_v) {
	int v = (int)Math::round(p_v * 127.0f);
	return (int8_t)CLAMP(v, -127, 127);
}

static inline uint16_t _to_u16(float p_v) {
	int v = (int)Math::round(p_v * 65535.0f);
	return (uint16_t)CLAMP(v, 0, 65535);
}

static inline uint32_t _align(uint32_t p_offset, uint32_t p_align) {
	return (p_offset + p_align - 1) & ~(p_align - 1);
}

int RendererPSP::draw_chunk_size(int p_primitive, int p_count) {
	const int max_count = 65535;
	if (p_count <= max_count) {
		return MAX(p_count, 1);
	}
	switch (p_primitive) {
		case RSE::PRIMITIVE_POINTS:
			return max_count;
		case RSE::PRIMITIVE_LINES:
			return max_count - (max_count % 2);
		case RSE::PRIMITIVE_TRIANGLES:
			return max_count - (max_count % 3);
		default:
			return 0;
	}
}

void MeshStorage::_free_surface(Surface &p_surface) {
	if (p_surface.vertices) {
		free(p_surface.vertices);
		p_surface.vertices = nullptr;
	}
	if (p_surface.indices) {
		free(p_surface.indices);
		p_surface.indices = nullptr;
	}
}

void MeshStorage::_update_aabb(Mesh *p_mesh) {
	AABB aabb;
	for (uint32_t i = 0; i < p_mesh->surfaces.size(); i++) {
		if (i == 0) {
			aabb = p_mesh->surfaces[i].aabb;
		} else {
			aabb.merge_with(p_mesh->surfaces[i].aabb);
		}
	}
	p_mesh->aabb = aabb;
}

bool MeshStorage::build_surface(const Array &p_arrays, RSE::PrimitiveType p_primitive, const AABB &p_aabb, Surface &r_surface) {
	ERR_FAIL_COND_V(p_arrays.size() != RSE::ARRAY_MAX, false);
	PackedVector3Array positions;
	if (p_arrays[RSE::ARRAY_VERTEX].get_type() == Variant::PACKED_VECTOR2_ARRAY) {
		// 2D mesh (Polygon2D, MeshInstance2D): z = 0 düzleminde 3D pozisyon.
		const PackedVector2Array p2 = p_arrays[RSE::ARRAY_VERTEX];
		positions.resize(p2.size());
		for (int i = 0; i < p2.size(); i++) {
			positions.write[i] = Vector3(p2[i].x, p2[i].y, 0.0f);
		}
	} else if (p_arrays[RSE::ARRAY_VERTEX].get_type() == Variant::PACKED_VECTOR3_ARRAY) {
		positions = p_arrays[RSE::ARRAY_VERTEX];
	} else {
		WARN_PRINT_ONCE("PSP: unsupported vertex array type.");
		return false;
	}
	const PackedVector3Array normals = p_arrays[RSE::ARRAY_NORMAL];
	const PackedVector2Array uvs = p_arrays[RSE::ARRAY_TEX_UV];
	const PackedColorArray colors = p_arrays[RSE::ARRAY_COLOR];
	const PackedInt32Array indices = p_arrays[RSE::ARRAY_INDEX];

	const int vcount = positions.size();
	ERR_FAIL_COND_V(vcount == 0, false);
	if (vcount > 65535 && indices.size() > 0) {
		WARN_PRINT_ONCE("PSP: surfaces with more than 65535 indexed vertices are not supported.");
		return false;
	}

	const bool has_uv = uvs.size() == vcount;
	const bool has_color = colors.size() == vcount;
	const bool has_normal = normals.size() == vcount;

	// Düzen: bileşen ofsetleri ve vertex boyutu (en büyük bileşen hizasına yuvarlanır).
	uint32_t off = 0, max_align = 2;
	uint32_t uv_off = 0, color_off = 0, normal_off = 0, pos_off = 0;
	// Normalli (ışıklanabilir) yüzeyler: float pozisyon, model matrisinde ölçek yok → GE normali normalize etsin
	// etmesin ışık doğru. Normalsiz yüzeyler (unshaded/2D): AABB'ye göre ölçekli 16-bit pozisyon (yarı bellek).
	const bool float_pos = has_normal;
	uint32_t vtype = float_pos ? GU_VERTEX_32BITF : GU_VERTEX_16BIT;
	if (has_uv) {
		uv_off = off;
		off += 4;
		vtype |= GU_TEXTURE_16BIT;
	}
	if (has_color) {
		off = _align(off, 4);
		color_off = off;
		off += 4;
		max_align = 4;
		vtype |= GU_COLOR_8888;
	}
	if (has_normal) {
		normal_off = off;
		off += 3;
		vtype |= GU_NORMAL_8BIT;
	}
	if (float_pos) {
		off = _align(off, 4);
		pos_off = off;
		off += 12;
		max_align = 4;
	} else {
		off = _align(off, 2);
		pos_off = off;
		off += 6;
	}
	const uint32_t stride = _align(off, max_align);

	AABB aabb = p_aabb;
	if (aabb.size == Vector3()) {
		aabb.position = positions[0];
		for (int i = 1; i < vcount; i++) {
			aabb.expand_to(positions[i]);
		}
	}
	Vector3 center = aabb.get_center();
	Vector3 half = aabb.size * 0.5f;
	half.x = MAX(half.x, 1e-4f);
	half.y = MAX(half.y, 1e-4f);
	half.z = MAX(half.z, 1e-4f);
	if (float_pos) {
		center = Vector3();
		half = Vector3(1, 1, 1); // model matrisine ölçek eklenmez
	}

	Vector2 uv_min, uv_max;
	if (has_uv) {
		uv_min = uv_max = uvs[0];
		for (int i = 1; i < vcount; i++) {
			uv_min = uv_min.min(uvs[i]);
			uv_max = uv_max.max(uvs[i]);
		}
	}
	Vector2 uv_range = uv_max - uv_min;
	uv_range.x = MAX(uv_range.x, 1e-6f);
	uv_range.y = MAX(uv_range.y, 1e-6f);

	uint8_t *vbuf = (uint8_t *)memalign(16, stride * vcount);
	ERR_FAIL_NULL_V(vbuf, false);
	memset(vbuf, 0, stride * vcount);
	for (int i = 0; i < vcount; i++) {
		uint8_t *v = vbuf + stride * i;
		if (has_uv) {
			uint16_t *t = (uint16_t *)(v + uv_off);
			t[0] = _to_u16((uvs[i].x - uv_min.x) / uv_range.x);
			t[1] = _to_u16((uvs[i].y - uv_min.y) / uv_range.y);
		}
		if (has_color) {
			const Color &c = colors[i];
			*(uint32_t *)(v + color_off) = c.to_abgr32();
		}
		if (has_normal) {
			// Model matrisi pozisyon ölçeğini (pos_half) içerir ve GE normali bu matrisle dönüştürüp normalize eder;
			// doğru yönü korumak için ölçeğin tersini önceden uygula.
			Vector3 nn = normals[i] / half;
			const float len = nn.length();
			nn = len > 0.0f ? nn / len : Vector3(0, 1, 0);
			int8_t *n = (int8_t *)(v + normal_off);
			n[0] = _to_s8(nn.x);
			n[1] = _to_s8(nn.y);
			n[2] = _to_s8(nn.z);
		}
		if (float_pos) {
			float *p = (float *)(v + pos_off);
			p[0] = positions[i].x;
			p[1] = positions[i].y;
			p[2] = positions[i].z;
		} else {
			int16_t *p = (int16_t *)(v + pos_off);
			p[0] = _to_s16((positions[i].x - center.x) / half.x);
			p[1] = _to_s16((positions[i].y - center.y) / half.y);
			p[2] = _to_s16((positions[i].z - center.z) / half.z);
		}
	}

	uint16_t *ibuf = nullptr;
	const int icount = indices.size();
	if (icount > 0) {
		ibuf = (uint16_t *)memalign(16, sizeof(uint16_t) * icount);
		if (!ibuf) {
			free(vbuf);
			ERR_FAIL_V(false);
		}
		for (int i = 0; i < icount; i++) {
			ibuf[i] = (uint16_t)indices[i];
		}
		vtype |= GU_INDEX_16BIT;
	}

	// GE önbellekten değil RAM'den okur.
	sceKernelDcacheWritebackRange(vbuf, stride * vcount);
	if (ibuf) {
		sceKernelDcacheWritebackRange(ibuf, sizeof(uint16_t) * icount);
	}

	r_surface.vertices = vbuf;
	r_surface.indices = ibuf;
	r_surface.vertex_type = vtype;
	r_surface.vertex_count = vcount;
	r_surface.index_count = icount;
	r_surface.primitive = (int)p_primitive;
	r_surface.stride = stride;
	r_surface.bytes = stride * vcount + sizeof(uint16_t) * icount;
	r_surface.aabb = aabb;
	r_surface.pos_center = center;
	r_surface.pos_half = half;
	r_surface.uv_min = uv_min;
	r_surface.uv_range = uv_range;
	return true;
}

RID MeshStorage::mesh_allocate() {
	return mesh_owner.allocate_rid();
}

void MeshStorage::mesh_initialize(RID p_rid) {
	mesh_owner.initialize_rid(p_rid, Mesh());
}

void MeshStorage::mesh_free(RID p_rid) {
	Mesh *mesh = mesh_owner.get_or_null(p_rid);
	ERR_FAIL_NULL(mesh);
	mesh_clear(p_rid);
	mesh->dependency.deleted_notify(p_rid);
	mesh_owner.free(p_rid);
}

void MeshStorage::mesh_set_blend_shape_count(RID p_mesh, int p_blend_shape_count) {
	Mesh *mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL(mesh);
	mesh->blend_shape_count = p_blend_shape_count;
}

void MeshStorage::mesh_add_surface(RID p_mesh, const RenderingServerTypes::SurfaceData &p_surface) {
	Mesh *mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL(mesh);

	Surface s;
	s.format = p_surface.format;
	s.material = p_surface.material;
	s.aabb = p_surface.aabb;
	{
		// Godot'un (sıkıştırılmış olabilen) yüzey verisini dizilere aç, GE formatına çevir; geçici diziler burada ölür.
		Array arrays = RenderingServer::get_singleton()->mesh_create_arrays_from_surface_data(p_surface);
		if (!build_surface(arrays, p_surface.primitive, p_surface.aabb, s)) {
			WARN_PRINT_ONCE("PSP: a mesh surface could not be converted and will not be drawn.");
		}
	}
	total_bytes += s.bytes;
	mesh->surfaces.push_back(s);
	_update_aabb(mesh);
	mesh->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MESH);
}

int MeshStorage::mesh_get_blend_shape_count(RID p_mesh) const {
	Mesh *mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL_V(mesh, 0);
	return mesh->blend_shape_count;
}

void MeshStorage::mesh_set_blend_shape_mode(RID p_mesh, RSE::BlendShapeMode p_mode) {
	Mesh *mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL(mesh);
	mesh->blend_shape_mode = p_mode;
}

RSE::BlendShapeMode MeshStorage::mesh_get_blend_shape_mode(RID p_mesh) const {
	Mesh *mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL_V(mesh, RSE::BLEND_SHAPE_MODE_NORMALIZED);
	return mesh->blend_shape_mode;
}

void MeshStorage::mesh_surface_set_material(RID p_mesh, int p_surface, RID p_material) {
	Mesh *mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL(mesh);
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_surface, mesh->surfaces.size());
	mesh->surfaces[p_surface].material = p_material;
	mesh->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MATERIAL);
}

RID MeshStorage::mesh_surface_get_material(RID p_mesh, int p_surface) const {
	Mesh *mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL_V(mesh, RID());
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_surface, mesh->surfaces.size(), RID());
	return mesh->surfaces[p_surface].material;
}

RenderingServerTypes::SurfaceData MeshStorage::mesh_get_surface(RID p_mesh, int p_surface) const {
	// Bellek için ham vertex verisi saklanmaz: yalnızca meta veri döner (surface_get_arrays boş gelir).
	RenderingServerTypes::SurfaceData sd;
	Mesh *mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL_V(mesh, sd);
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_surface, mesh->surfaces.size(), sd);
	const Surface &s = mesh->surfaces[p_surface];
	sd.primitive = (RSE::PrimitiveType)s.primitive;
	sd.material = s.material;
	sd.aabb = s.aabb;
	return sd;
}

int MeshStorage::mesh_get_surface_count(RID p_mesh) const {
	Mesh *mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL_V(mesh, 0);
	return mesh->surfaces.size();
}

void MeshStorage::mesh_set_custom_aabb(RID p_mesh, const AABB &p_aabb) {
	Mesh *mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL(mesh);
	mesh->custom_aabb = p_aabb;
	mesh->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_AABB);
}

AABB MeshStorage::mesh_get_custom_aabb(RID p_mesh) const {
	Mesh *mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL_V(mesh, AABB());
	return mesh->custom_aabb;
}

AABB MeshStorage::mesh_get_aabb(RID p_mesh, RID p_skeleton) {
	Mesh *mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL_V(mesh, AABB());
	if (mesh->custom_aabb != AABB()) {
		return mesh->custom_aabb;
	}
	return mesh->aabb;
}

void MeshStorage::mesh_surface_remove(RID p_mesh, int p_surface) {
	Mesh *mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL(mesh);
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_surface, mesh->surfaces.size());
	total_bytes -= mesh->surfaces[p_surface].bytes;
	_free_surface(mesh->surfaces[p_surface]);
	mesh->surfaces.remove_at(p_surface);
	_update_aabb(mesh);
	mesh->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MESH);
}

void MeshStorage::mesh_clear(RID p_mesh) {
	Mesh *mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL(mesh);
	for (Surface &s : mesh->surfaces) {
		total_bytes -= s.bytes;
		_free_surface(s);
	}
	mesh->surfaces.clear();
	mesh->aabb = AABB();
	mesh->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MESH);
}

MeshStorage::MeshStorage() {
	psp_singleton = this;
}

MeshStorage::~MeshStorage() {
	psp_singleton = nullptr;
}
