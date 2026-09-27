#pragma once

#include "core/templates/hash_map.h"
#include "servers/rendering/dummy/rasterizer_canvas_dummy.h"

// 2D canvas çizimi (PSP GE, "through" modu: vertex'ler CPU'da ekran koordinatına dönüştürülür,
// vertex belleği display list'ten ayrılır). Desteklenen komutlar: RECT (texture/bölge/çevirme/tile),
// NINEPATCH (stretch), POLYGON, PRIMITIVE, TRANSFORM, CLIP_IGNORE. Canvas ışıkları, gölgeler, canvas
// shader'ları, mesh/multimesh/particles desteklenmez (atlanır).
class RasterizerCanvasPSP : public RasterizerCanvasDummy {
	struct PolygonData {
		Vector<int> indices;
		Vector<Point2> points;
		Vector<Color> colors;
		Vector<Point2> uvs;
		int count = 0;
	};
	HashMap<PolygonID, PolygonData *> polygons;
	PolygonID polygon_id_counter = 1;

	struct Vertex2D {
		float u, v;
		uint32_t color;
		float x, y, z;
	};

	struct DrawState {
		Transform2D xform;
		Color modulate;
		const void *bound_texture = nullptr;
		int filter = 0; // GU_NEAREST / GU_LINEAR
		bool repeat = false;
		// Bağlı texture'ın texel ölçeği (kaynak piksel → dönüştürülmüş texture texel'i) ve kaynak boyutu.
		Vector2 texel_scale;
		Vector2 source_size;
		bool textured = false;
	};

	bool _bind_texture(RID p_texture, DrawState &r_state);
	void _draw_quad(const DrawState &p_state, const Vector2 p_pos[4], const Vector2 p_uv[4], uint32_t p_color);
	void _draw_rect(DrawState &p_state, const Item::CommandRect *p_rect);
	void _draw_ninepatch(DrawState &p_state, const Item::CommandNinePatch *p_np);
	void _draw_polygon(DrawState &p_state, const Item::CommandPolygon *p_poly);
	void _draw_primitive(DrawState &p_state, const Item::CommandPrimitive *p_prim);
	void _draw_mesh(DrawState &p_state, const Item::CommandMesh *p_mesh);
	bool mesh_mode = false; // GE 3D yolu (ortografik) ayarlandı mı

public:
	PolygonID request_polygon(const Vector<int> &p_indices, const Vector<Point2> &p_points, const Vector<Color> &p_colors, const Vector<Point2> &p_uvs = Vector<Point2>(), const Vector<int> &p_bones = Vector<int>(), const Vector<float> &p_weights = Vector<float>(), int p_count = -1) override;
	void free_polygon(PolygonID p_polygon) override;

	void canvas_render_items(RID p_to_render_target, Item *p_item_list, const Color &p_modulate, Light *p_light_list, Light *p_directional_list, const Transform2D &p_canvas_transform, RSE::CanvasItemTextureFilter p_default_filter, RSE::CanvasItemTextureRepeat p_default_repeat, bool p_snap_2d_vertices_to_pixel, bool &r_sdf_used, RenderingServerTypes::RenderInfo *r_render_info = nullptr) override;

	~RasterizerCanvasPSP();
};
