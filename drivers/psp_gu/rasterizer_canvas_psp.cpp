#include "rasterizer_canvas_psp.h"

#include "psp_gu.h"
#include "storage/mesh_storage_psp.h"
#include "storage/texture_storage_psp.h"

#include "servers/rendering/rendering_server_globals.h"

#include <pspgu.h>

using namespace RendererPSP;

static constexpr int VERTEX_2D_FORMAT = GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D;

static inline uint32_t _abgr(const Color &p_c) {
	return PSPGU::color_to_abgr(p_c.r, p_c.g, p_c.b, p_c.a);
}

/* POLYGONS */

RasterizerCanvasPSP::PolygonID RasterizerCanvasPSP::request_polygon(const Vector<int> &p_indices, const Vector<Point2> &p_points, const Vector<Color> &p_colors, const Vector<Point2> &p_uvs, const Vector<int> &p_bones, const Vector<float> &p_weights, int p_count) {
	PolygonData *pd = memnew(PolygonData);
	pd->indices = p_indices;
	pd->points = p_points;
	pd->colors = p_colors;
	pd->uvs = p_uvs;
	pd->count = p_count < 0 ? p_indices.size() : p_count;
	const PolygonID id = polygon_id_counter++;
	polygons.insert(id, pd);
	return id;
}

void RasterizerCanvasPSP::free_polygon(PolygonID p_polygon) {
	PolygonData **pd = polygons.getptr(p_polygon);
	ERR_FAIL_NULL(pd);
	memdelete(*pd);
	polygons.erase(p_polygon);
}

RasterizerCanvasPSP::~RasterizerCanvasPSP() {
	for (KeyValue<PolygonID, PolygonData *> &E : polygons) {
		memdelete(E.value);
	}
}

/* DRAWING */

bool RasterizerCanvasPSP::_bind_texture(RID p_texture, DrawState &r_state) {
	const TextureStorage::Texture *tex = p_texture.is_valid() ? static_cast<TextureStorage *>(RSG::texture_storage)->get_psp_texture(p_texture) : nullptr;
	if (!tex || !tex->data.pixels) {
		if (r_state.textured) {
			sceGuDisable(GU_TEXTURE_2D);
			r_state.textured = false;
		}
		return false;
	}
	const PSPTextureData &td = tex->data;
	if (r_state.bound_texture != td.pixels) {
		sceGuTexMode(td.psm, 0, 0, td.swizzled ? GU_TRUE : GU_FALSE);
		sceGuTexImage(0, td.width, td.height, td.width, PSPGU::texture_address(td.pixels, td.bytes));
		sceGuTexFlush();
		r_state.bound_texture = td.pixels;
	}
	if (r_state.sent_filter != r_state.filter) {
		sceGuTexFilter(r_state.filter, r_state.filter);
		r_state.sent_filter = r_state.filter;
	}
	if (r_state.sent_repeat != (int)r_state.repeat) {
		sceGuTexWrap(r_state.repeat ? GU_REPEAT : GU_CLAMP, r_state.repeat ? GU_REPEAT : GU_CLAMP);
		r_state.sent_repeat = (int)r_state.repeat;
	}
	r_state.texel_scale = Vector2((float)td.width / MAX(td.source_width, 1), (float)td.height / MAX(td.source_height, 1));
	r_state.source_size = Vector2(td.source_width, td.source_height);
	if (!r_state.textured) {
		sceGuEnable(GU_TEXTURE_2D);
		r_state.textured = true;
	}
	return true;
}

// p_pos: yerel koordinat (xform uygulanır); p_uv: kaynak texture pikseli. Sıra: sol üst, sağ üst, sol alt, sağ alt.
void RasterizerCanvasPSP::_draw_quad(const DrawState &p_state, const Vector2 p_pos[4], const Vector2 p_uv[4], uint32_t p_color) {
	Vertex2D *v = (Vertex2D *)PSPGU::frame_alloc(sizeof(Vertex2D) * 4);
	for (int i = 0; i < 4; i++) {
		const Vector2 p = p_state.xform.xform(p_pos[i]);
		v[i].x = p.x;
		v[i].y = p.y;
		v[i].z = 0.0f;
		v[i].u = p_uv[i].x * p_state.texel_scale.x;
		v[i].v = p_uv[i].y * p_state.texel_scale.y;
		v[i].color = p_color;
	}
	sceGuDrawArray(GU_TRIANGLE_STRIP, VERTEX_2D_FORMAT, 4, nullptr, v);
	PSPGU::stats.draws_accum++;
}

void RasterizerCanvasPSP::_draw_rect(DrawState &p_state, const Item::CommandRect *p_rect) {
	Rect2 rect = p_rect->rect;
	uint16_t flags = p_rect->flags;
	// Negatif boyut = çevirme.
	if (rect.size.x < 0) {
		rect.position.x += rect.size.x;
		rect.size.x = -rect.size.x;
		flags ^= RendererCanvasRender::CANVAS_RECT_FLIP_H;
	}
	if (rect.size.y < 0) {
		rect.position.y += rect.size.y;
		rect.size.y = -rect.size.y;
		flags ^= RendererCanvasRender::CANVAS_RECT_FLIP_V;
	}
	const bool textured = _bind_texture(p_rect->texture, p_state);
	Rect2 src;
	if (textured) {
		src = (flags & RendererCanvasRender::CANVAS_RECT_REGION) ? p_rect->source : Rect2(Vector2(), p_state.source_size);
		if (flags & RendererCanvasRender::CANVAS_RECT_TILE) {
			// Tile: kaynak bölge yerine hedef boyut kadar tekrar (repeat açık olmalı).
			src.size = rect.size;
		}
	}
	Vector2 uv0 = src.position, uv1 = src.position + src.size;
	if (flags & RendererCanvasRender::CANVAS_RECT_FLIP_H) {
		SWAP(uv0.x, uv1.x);
	}
	if (flags & RendererCanvasRender::CANVAS_RECT_FLIP_V) {
		SWAP(uv0.y, uv1.y);
	}
	const Vector2 pos[4] = { rect.position, Vector2(rect.position.x + rect.size.x, rect.position.y), Vector2(rect.position.x, rect.position.y + rect.size.y), rect.position + rect.size };
	Vector2 uv[4] = { uv0, Vector2(uv1.x, uv0.y), Vector2(uv0.x, uv1.y), uv1 };
	if (flags & RendererCanvasRender::CANVAS_RECT_TRANSPOSE) {
		SWAP(uv[1], uv[2]);
	}
	_draw_quad(p_state, pos, uv, _abgr(p_state.modulate * p_rect->modulate));
}

void RasterizerCanvasPSP::_draw_ninepatch(DrawState &p_state, const Item::CommandNinePatch *p_np) {
	const bool textured = _bind_texture(p_np->texture, p_state);
	const Rect2 src = textured ? (p_np->source != Rect2() ? p_np->source : Rect2(Vector2(), p_state.source_size)) : Rect2();
	const Rect2 &r = p_np->rect;
	// margin: sol, üst, sağ, alt (kaynak ve hedefte aynı piksel).
	const float ml = p_np->margin[SIDE_LEFT], mt = p_np->margin[SIDE_TOP], mr = p_np->margin[SIDE_RIGHT], mb = p_np->margin[SIDE_BOTTOM];
	const float dx[4] = { r.position.x, r.position.x + ml, r.position.x + r.size.x - mr, r.position.x + r.size.x };
	const float dy[4] = { r.position.y, r.position.y + mt, r.position.y + r.size.y - mb, r.position.y + r.size.y };
	const float sx[4] = { src.position.x, src.position.x + ml, src.position.x + src.size.x - mr, src.position.x + src.size.x };
	const float sy[4] = { src.position.y, src.position.y + mt, src.position.y + src.size.y - mb, src.position.y + src.size.y };
	const uint32_t color = _abgr(p_state.modulate * p_np->color);
	for (int y = 0; y < 3; y++) {
		for (int x = 0; x < 3; x++) {
			if (x == 1 && y == 1 && !p_np->draw_center) {
				continue;
			}
			if (dx[x + 1] <= dx[x] || dy[y + 1] <= dy[y]) {
				continue;
			}
			const Vector2 pos[4] = { Vector2(dx[x], dy[y]), Vector2(dx[x + 1], dy[y]), Vector2(dx[x], dy[y + 1]), Vector2(dx[x + 1], dy[y + 1]) };
			const Vector2 uv[4] = { Vector2(sx[x], sy[y]), Vector2(sx[x + 1], sy[y]), Vector2(sx[x], sy[y + 1]), Vector2(sx[x + 1], sy[y + 1]) };
			_draw_quad(p_state, pos, uv, color);
		}
	}
}

void RasterizerCanvasPSP::_draw_polygon(DrawState &p_state, const Item::CommandPolygon *p_poly) {
	PolygonData **pdp = polygons.getptr(p_poly->polygon.polygon_id);
	if (!pdp) {
		return;
	}
	const PolygonData *pd = *pdp;
	const bool textured = _bind_texture(p_poly->texture, p_state);
	const int count = pd->count;
	if (count <= 0) {
		return;
	}
	const int npoints = pd->points.size();
	const bool per_vertex_color = pd->colors.size() == npoints;
	const Color single = pd->colors.size() == 1 ? pd->colors[0] : Color(1, 1, 1, 1);
	const bool has_uv = textured && pd->uvs.size() == npoints;
	// UV'ler normalize (0..1) → kaynak piksel → texel.
	const Vector2 uv_scale = p_state.source_size;

	int prim = GU_TRIANGLES;
	switch (p_poly->primitive) {
		case RSE::PRIMITIVE_POINTS:
			prim = GU_POINTS;
			break;
		case RSE::PRIMITIVE_LINES:
			prim = GU_LINES;
			break;
		case RSE::PRIMITIVE_LINE_STRIP:
			prim = GU_LINE_STRIP;
			break;
		case RSE::PRIMITIVE_TRIANGLE_STRIP:
			prim = GU_TRIANGLE_STRIP;
			break;
		default:
			break;
	}
	// Vertex'ler parça parça üretilip display list'e yazılır (büyük poligonlar tek seferde sığmaz).
	const int *idx = pd->indices.ptr();
	const int max_chunk = 1200;
	static LocalVector<Vertex2D> tmp; // kareler arası yeniden kullanılır (her çizimde ayırma yok)
	if (tmp.size() < (uint32_t)MIN(count, max_chunk)) {
		tmp.resize(MIN(count, max_chunk));
	}
	int start = 0;
	while (start < count) {
		int n = MIN(max_chunk, count - start);
		if (prim == GU_TRIANGLES && n < count - start) {
			n -= n % 3;
		} else if (prim == GU_LINES && n < count - start) {
			n -= n % 2;
		}
		for (int i = 0; i < n; i++) {
			const int k = idx[start + i];
			Vertex2D &vv = tmp[i];
			if (k < 0 || k >= npoints) {
				vv = Vertex2D{ 0, 0, 0, 0, 0, 0 };
				continue;
			}
			const Vector2 p = p_state.xform.xform(pd->points[k]);
			vv.x = p.x;
			vv.y = p.y;
			vv.z = 0.0f;
			if (has_uv) {
				vv.u = pd->uvs[k].x * uv_scale.x * p_state.texel_scale.x;
				vv.v = pd->uvs[k].y * uv_scale.y * p_state.texel_scale.y;
			} else {
				vv.u = vv.v = 0.0f;
			}
			vv.color = _abgr(p_state.modulate * (per_vertex_color ? pd->colors[k] : single));
		}
		_draw_vertices(prim, tmp.ptr(), n);
		if (start + n >= count) {
			break;
		}
		// Şeritler: süreklilik için son vertex(ler) tekrar edilir (üçgen şeridinde parite korunur).
		const int overlap = prim == GU_TRIANGLE_STRIP ? 2 : (prim == GU_LINE_STRIP ? 1 : 0);
		if (prim == GU_TRIANGLE_STRIP && (n % 2) != 0) {
			n--; // çift sayıda ilerle: sarım yönü değişmesin
		}
		start += n - overlap;
	}
	PSPGU::stats.draws_accum++;
}

void RasterizerCanvasPSP::_draw_vertices(int p_prim, const Vertex2D *p_src, int p_count) {
	Vertex2D *v = (Vertex2D *)PSPGU::frame_alloc(sizeof(Vertex2D) * p_count);
	memcpy(v, p_src, sizeof(Vertex2D) * p_count);
	sceGuDrawArray(p_prim, VERTEX_2D_FORMAT, p_count, nullptr, v);
}

void RasterizerCanvasPSP::_draw_primitive(DrawState &p_state, const Item::CommandPrimitive *p_prim) {
	const bool textured = _bind_texture(p_prim->texture, p_state);
	const int n = CLAMP((int)p_prim->point_count, 1, 4);
	// Quad: 0,1,3,2 sırasıyla strip.
	static const int quad_order[4] = { 0, 1, 3, 2 };
	Vertex2D *v = (Vertex2D *)PSPGU::frame_alloc(sizeof(Vertex2D) * n);
	for (int i = 0; i < n; i++) {
		const int k = n == 4 ? quad_order[i] : i;
		const Vector2 p = p_state.xform.xform(p_prim->points[k]);
		v[i].x = p.x;
		v[i].y = p.y;
		v[i].z = 0.0f;
		v[i].u = textured ? p_prim->uvs[k].x * p_state.source_size.x * p_state.texel_scale.x : 0.0f;
		v[i].v = textured ? p_prim->uvs[k].y * p_state.source_size.y * p_state.texel_scale.y : 0.0f;
		v[i].color = _abgr(p_state.modulate * p_prim->colors[k]);
	}
	static const int prims[5] = { 0, GU_POINTS, GU_LINES, GU_TRIANGLES, GU_TRIANGLE_STRIP };
	sceGuDrawArray(prims[n], VERTEX_2D_FORMAT, n, nullptr, v);
	PSPGU::stats.draws_accum++;
}

void RasterizerCanvasPSP::_draw_mesh(DrawState &p_state, const Item::CommandMesh *p_mesh) {
	Mesh *mesh = MeshStorage::get_psp_singleton()->get_mesh(p_mesh->mesh);
	if (!mesh) {
		return;
	}
	if (!mesh_mode) {
		// Piksel koordinatlı ortografik projeksiyon (y aşağı), görünüm birim; derinlik testi kapalı.
		ScePspFMatrix4 proj = {
			{ 2.0f / PSPGU::SCREEN_W, 0, 0, 0 },
			{ 0, -2.0f / PSPGU::SCREEN_H, 0, 0 },
			{ 0, 0, 0, 0 },
			{ -1.0f, 1.0f, 0, 1.0f },
		};
		sceGuSetMatrix(GU_PROJECTION, &proj);
		ScePspFMatrix4 ident = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } };
		sceGuSetMatrix(GU_VIEW, &ident);
		mesh_mode = true;
	}
	const Transform2D xf = p_state.xform * p_mesh->transform;
	const Color color = p_state.modulate * p_mesh->modulate;
	for (uint32_t s = 0; s < mesh->surfaces.size(); s++) {
		const Surface &surf = mesh->surfaces[s];
		if (!surf.vertices) {
			continue;
		}
		PSPGU::ensure_list_space(512);
		// 2D dönüşüm → 3D model matrisi, ardından yüzeyin 16-bit pozisyon ölçeği.
		const Transform3D world(Basis(Vector3(xf.columns[0].x, xf.columns[0].y, 0), Vector3(xf.columns[1].x, xf.columns[1].y, 0), Vector3(0, 0, 1)), Vector3(xf.columns[2].x, xf.columns[2].y, 0));
		const Transform3D model = world * Transform3D(Basis::from_scale(surf.pos_half), surf.pos_center);
		const Basis &b = model.basis;
		ScePspFMatrix4 m = {
			{ b.rows[0][0], b.rows[1][0], b.rows[2][0], 0 },
			{ b.rows[0][1], b.rows[1][1], b.rows[2][1], 0 },
			{ b.rows[0][2], b.rows[1][2], b.rows[2][2], 0 },
			{ model.origin.x, model.origin.y, model.origin.z, 1 },
		};
		sceGuSetMatrix(GU_MODEL, &m);
		// Vertex rengi yoksa GE rengi materyal renginden (sceGuColor) alır. Vertex rengi varsa ışıksız modda
		// sceGuColor yok sayılır: modulate için "ışıklı, ışıksız, ambient = modulate, materyal ambient = vertex rengi".
		sceGuColor(_abgr(color));
		const bool modulate_vertex_color = (surf.vertex_type & GU_COLOR_8888) && color != Color(1, 1, 1, 1);
		if (modulate_vertex_color) {
			sceGuEnable(GU_LIGHTING);
			for (int l = 0; l < 4; l++) {
				sceGuDisable(GU_LIGHT0 + l);
			}
			sceGuAmbient(_abgr(color));
			sceGuColorMaterial(GU_AMBIENT);
		}
		const bool textured = (surf.vertex_type & GU_TEXTURE_16BIT) && _bind_texture(p_mesh->texture, p_state);
		if (textured) {
			sceGuTexScale(surf.uv_range.x * 0.5f, surf.uv_range.y * 0.5f);
			sceGuTexOffset(surf.uv_min.x, surf.uv_min.y);
		} else if (p_state.textured) {
			sceGuDisable(GU_TEXTURE_2D);
			p_state.textured = false;
		}
		const int total = surf.index_count > 0 ? surf.index_count : surf.vertex_count;
		const int chunk = draw_chunk_size(surf.primitive, total);
		for (int start = 0; chunk > 0 && start < total; start += chunk) {
			const int n = MIN(chunk, total - start);
			if (surf.index_count > 0) {
				sceGuDrawArray(surf.primitive, surf.vertex_type | GU_TRANSFORM_3D, n, surf.indices + start, surf.vertices);
			} else {
				sceGuDrawArray(surf.primitive, surf.vertex_type | GU_TRANSFORM_3D, n, nullptr, (const uint8_t *)surf.vertices + (size_t)start * surf.stride);
			}
		}
		if (modulate_vertex_color) {
			sceGuDisable(GU_LIGHTING);
			sceGuColorMaterial(0);
		}
		PSPGU::stats.draws_accum++;
	}
}

void RasterizerCanvasPSP::canvas_render_items(RID p_to_render_target, Item *p_item_list, const Color &p_modulate, Light *p_light_list, Light *p_directional_list, const Transform2D &p_canvas_transform, RSE::CanvasItemTextureFilter p_default_filter, RSE::CanvasItemTextureRepeat p_default_repeat, bool p_snap_2d_vertices_to_pixel, bool &r_sdf_used, RenderingServerTypes::RenderInfo *r_render_info) {
	r_sdf_used = false;
	if (!PSPGU::in_frame()) {
		return;
	}
	// 3D çizilmediyse render target'ın temizleme isteği henüz uygulanmadı: 2D'den önce temizle.
	if (RSG::texture_storage->render_target_is_clear_requested(p_to_render_target)) {
		RSG::texture_storage->render_target_do_clear_request(p_to_render_target);
	}

	// 2D durumu: derinlik/ışık/fog/kırpma yok, alfa karışımı açık.
	sceGuDisable(GU_DEPTH_TEST);
	sceGuDepthMask(GU_TRUE);
	sceGuDisable(GU_LIGHTING);
	sceGuDisable(GU_FOG);
	sceGuDisable(GU_CULL_FACE);
	sceGuDisable(GU_ALPHA_TEST);
	sceGuEnable(GU_BLEND);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
	sceGuShadeModel(GU_SMOOTH);
	sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	sceGuDisable(GU_TEXTURE_2D);

	DrawState state;
	bool scissor_active = false;
	mesh_mode = false;
	for (Item *ci = p_item_list; ci; ci = ci->next) {
		state.modulate = p_modulate * ci->final_modulate;
		// final_transform canvas dönüşümünü zaten içerir (render target pikseli); tekrar çarpılmaz.
		const Transform2D base_xform = ci->final_transform;
		state.xform = base_xform;
		const RSE::CanvasItemTextureFilter filter = ci->texture_filter == RSE::CANVAS_ITEM_TEXTURE_FILTER_DEFAULT ? p_default_filter : ci->texture_filter;
		state.filter = (filter == RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST || filter == RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST_WITH_MIPMAPS) ? GU_NEAREST : GU_LINEAR;
		const RSE::CanvasItemTextureRepeat repeat = ci->texture_repeat == RSE::CANVAS_ITEM_TEXTURE_REPEAT_DEFAULT ? p_default_repeat : ci->texture_repeat;
		state.repeat = repeat == RSE::CANVAS_ITEM_TEXTURE_REPEAT_ENABLED || repeat == RSE::CANVAS_ITEM_TEXTURE_REPEAT_MIRROR;

		// Kırpma (Control clip_contents vb.). final_clip_rect render target pikselindedir.
		// sceGuScissor(x, y, genişlik, yükseklik) alır.
		int clip[4] = { 0, 0, PSPGU::SCREEN_W, PSPGU::SCREEN_H };
		const bool want_clip = ci->final_clip_owner != nullptr;
		if (want_clip) {
			const Rect2 r = ci->final_clip_owner->final_clip_rect;
			const int x0 = CLAMP((int)Math::floor(r.position.x), 0, PSPGU::SCREEN_W);
			const int y0 = CLAMP((int)Math::floor(r.position.y), 0, PSPGU::SCREEN_H);
			const int x1 = CLAMP((int)Math::ceil(r.position.x + r.size.x), 0, PSPGU::SCREEN_W);
			const int y1 = CLAMP((int)Math::ceil(r.position.y + r.size.y), 0, PSPGU::SCREEN_H);
			if (x1 <= x0 || y1 <= y0) {
				continue; // tamamen kırpılmış
			}
			clip[0] = x0;
			clip[1] = y0;
			clip[2] = x1 - x0;
			clip[3] = y1 - y0;
			sceGuScissor(clip[0], clip[1], clip[2], clip[3]);
			scissor_active = true;
		} else if (scissor_active) {
			sceGuScissor(0, 0, PSPGU::SCREEN_W, PSPGU::SCREEN_H);
			scissor_active = false;
		}

		for (const Item::Command *c = ci->commands; c; c = c->next) {
			switch (c->type) {
				case Item::Command::TYPE_RECT:
					_draw_rect(state, static_cast<const Item::CommandRect *>(c));
					break;
				case Item::Command::TYPE_NINEPATCH:
					_draw_ninepatch(state, static_cast<const Item::CommandNinePatch *>(c));
					break;
				case Item::Command::TYPE_POLYGON:
					_draw_polygon(state, static_cast<const Item::CommandPolygon *>(c));
					break;
				case Item::Command::TYPE_PRIMITIVE:
					_draw_primitive(state, static_cast<const Item::CommandPrimitive *>(c));
					break;
				case Item::Command::TYPE_MESH:
					_draw_mesh(state, static_cast<const Item::CommandMesh *>(c));
					break;
				case Item::Command::TYPE_TRANSFORM:
					state.xform = base_xform * static_cast<const Item::CommandTransform *>(c)->xform;
					break;
				case Item::Command::TYPE_CLIP_IGNORE:
					// ignore=true: kırpmayı kaldır; ignore=false: item'ın kendi kırpmasını geri yükle.
					if (static_cast<const Item::CommandClipIgnore *>(c)->ignore) {
						sceGuScissor(0, 0, PSPGU::SCREEN_W, PSPGU::SCREEN_H);
					} else {
						sceGuScissor(clip[0], clip[1], clip[2], clip[3]);
					}
					break;
				default:
					WARN_PRINT_ONCE("PSP: 2D multimesh/particles are not supported and are skipped.");
					break;
			}
		}
	}

	if (scissor_active) {
		sceGuScissor(0, 0, PSPGU::SCREEN_W, PSPGU::SCREEN_H);
	}
	sceGuDisable(GU_BLEND);
	sceGuDisable(GU_TEXTURE_2D);
	sceGuEnable(GU_DEPTH_TEST);
	sceGuDepthMask(GU_FALSE);
}
