/*
Copyright (c) 2019-2021 Péter Magyar

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "mesh_data_resource.h"

#include "core/variant.h"
#include "core/version.h"

#ifdef XATLAS_PRESENT
#include "scene/resources/surface_tool.h"
#include "core/local_vector.h"
#include "thirdparty/xatlas/xatlas.h"
#endif

#if VERSION_MAJOR >= 4
#define PoolVector Vector
#endif

const String MeshDataResource::BINDING_STRING_COLLIDER_TYPE = "None,Trimesh Collision Shape,Single Convex Collision Shape,Multiple Convex Collision Shapes,Approximated Box,Approximated Capsule,Approximated Cylinder,Approximated Sphere";

Array MeshDataResource::get_array() {
	return _arrays;
}
void MeshDataResource::set_array(const Array &p_arrays) {
	_arrays = p_arrays;

	recompute_aabb();
}
Array MeshDataResource::get_array_const() const {
	return _arrays;
}

AABB MeshDataResource::get_aabb() const {
	return _aabb;
}
void MeshDataResource::set_aabb(const AABB &aabb) {
	_aabb = aabb;
}

void MeshDataResource::add_collision_shape(const Transform &transform, const Ref<Shape> &shape) {
	MDRData d;

	d.transform = transform;
	d.shape = shape;

	_collision_shapes.push_back(d);
}
Ref<Shape> MeshDataResource::get_collision_shape(const int index) {
	ERR_FAIL_INDEX_V(index, _collision_shapes.size(), Ref<Shape>());

	return _collision_shapes[index].shape;
}
Transform MeshDataResource::get_collision_shape_offset(const int index) {
	ERR_FAIL_INDEX_V(index, _collision_shapes.size(), Transform());

	return _collision_shapes[index].transform;
}
int MeshDataResource::get_collision_shape_count() const {
	return _collision_shapes.size();
}

Vector<Variant> MeshDataResource::get_collision_shapes() {
	Vector<Variant> r;
	for (int i = 0; i < _collision_shapes.size(); i++) {
#if VERSION_MAJOR < 4
		r.push_back(_collision_shapes[i].transform);
		r.push_back(_collision_shapes[i].shape.get_ref_ptr());
#else
		r.push_back(_collision_shapes[i].transform);
		r.push_back(_collision_shapes[i].shape);
#endif
	}
	return r;
}
void MeshDataResource::set_collision_shapes(const Vector<Variant> &p_arrays) {
	ERR_FAIL_COND(p_arrays.size() % 2 == 1);

	_collision_shapes.clear();
	for (int i = 0; i < p_arrays.size(); i += 2) {
		MDRData d;

		d.transform = p_arrays[i];
		d.shape = Ref<Shape>(p_arrays[i + 1]);

		_collision_shapes.push_back(d);
	}
}

PoolIntArray MeshDataResource::get_seams() {
	return _seams;
}

void MeshDataResource::set_seams(const PoolIntArray &array) {
	_seams = array;
}

void MeshDataResource::recompute_aabb() {
	if (_arrays.size() == 0) {
		return;
	}

	Variant arr = _arrays[Mesh::ARRAY_VERTEX];
	PoolVector<Vector3> vertices = arr;
	int len = vertices.size();

	if (len == 0) {
		return;
	}

#if VERSION_MAJOR < 4
	PoolVector<Vector3>::Read r = vertices.read();
	const Vector3 *vtx = r.ptr();
#else
	const Vector3 *vtx = vertices.ptr();
#endif

	AABB aabb;
	for (int i = 0; i < len; i++) {
		if (i == 0)
			aabb.position = vtx[i];
		else
			aabb.expand_to(vtx[i]);
	}

	_aabb = aabb;
}

struct MeshDataResourceUnwrapSurface {
	Vector<SurfaceTool::Vertex> vertices;
	uint32_t format;
};

bool MeshDataResource::uv_unwrap(float p_texel_size) {
	if (_arrays.size() != ArrayMesh::ARRAY_MAX) {
		return false;
	}

	PoolVector3Array pool_vertices = _arrays[Mesh::ARRAY_VERTEX];

	if (pool_vertices.size() == 0) {
		return false;
	}

	PoolVector3Array pool_normals = _arrays[Mesh::ARRAY_NORMAL];

	ERR_FAIL_COND_V_MSG(pool_vertices.size() != pool_normals.size(), false, "Normals are required for lightmap unwrap.");

	PoolIntArray pool_indices = _arrays[Mesh::ARRAY_INDEX];

	MeshDataResourceUnwrapSurface mdrsurface;

	mdrsurface.vertices = SurfaceTool::create_vertex_array_from_triangle_arrays(_arrays);

	LocalVector<Vector3> vertices;
	LocalVector<Vector3> normals;
	LocalVector<int> indices;

	vertices.resize(pool_vertices.size());
	normals.resize(pool_normals.size());

	for (int i = 0; i < pool_vertices.size(); ++i) {
		vertices[i] = pool_vertices[i];
		normals[i] = pool_normals[i];
	}

	indices.resize(pool_indices.size());

	for (int i = 0; i < pool_indices.size(); ++i) {
		indices[i] = pool_indices[i];
	}

	// set up input mesh
	xatlas::MeshDecl input_mesh;

	input_mesh.indexData = indices.ptr();
	input_mesh.indexCount = indices.size();
	input_mesh.indexFormat = xatlas::IndexFormat::UInt32;

	input_mesh.vertexCount = vertices.size();
	input_mesh.vertexPositionData = vertices.ptr();
	input_mesh.vertexPositionStride = sizeof(float) * 3;
	input_mesh.vertexNormalData = normals.ptr();
	input_mesh.vertexNormalStride = sizeof(uint32_t) * 3;
	input_mesh.vertexUvData = nullptr;
	input_mesh.vertexUvStride = 0;

	xatlas::ChartOptions chart_options;
	chart_options.fixWinding = true;

	xatlas::PackOptions pack_options;
	pack_options.padding = 1;
	pack_options.maxChartSize = 4094; // Lightmap atlassing needs 2 for padding between meshes, so 4096-2
	pack_options.blockAlign = true;
	pack_options.texelsPerUnit = 1.0 / p_texel_size;

	xatlas::Atlas *atlas = xatlas::Create();

	xatlas::AddMeshError err = xatlas::AddMesh(atlas, input_mesh, 1);
	ERR_FAIL_COND_V_MSG(err != xatlas::AddMeshError::Success, false, xatlas::StringForEnum(err));

	xatlas::Generate(atlas, chart_options, pack_options);

	int size_x = atlas->width;
	int size_y = atlas->height;

	float w = 1;
	float h = 1;

	if (w == 0 || h == 0) {
		xatlas::Destroy(atlas);
		return false; //could not bake because there is no area
	}

	const xatlas::Mesh &output = atlas->meshes[0];

	int *r_vertex = (int *)malloc(sizeof(int) * output.vertexCount);
	ERR_FAIL_NULL_V_MSG(r_vertex, false, "Out of memory.");
	float *r_uv = (float *)malloc(sizeof(float) * output.vertexCount * 2);
	ERR_FAIL_NULL_V_MSG(r_uv, false, "Out of memory.");
	int *r_index = (int *)malloc(sizeof(int) * output.indexCount);
	ERR_FAIL_NULL_V_MSG(r_index, false, "Out of memory.");

	float max_x = 0;
	float max_y = 0;
	for (uint32_t i = 0; i < output.vertexCount; i++) {
		r_vertex[i] = output.vertexArray[i].xref;
		r_uv[i * 2 + 0] = output.vertexArray[i].uv[0] / w;
		r_uv[i * 2 + 1] = output.vertexArray[i].uv[1] / h;
		max_x = MAX(max_x, output.vertexArray[i].uv[0]);
		max_y = MAX(max_y, output.vertexArray[i].uv[1]);
	}

	int r_vertex_count = output.vertexCount;

	for (uint32_t i = 0; i < output.indexCount; i++) {
		r_index[i] = output.indexArray[i];
	}

	int r_index_count = output.indexCount;

/*

	print_verbose("Mesh: Gen indices: " + itos(gen_index_count));
	//go through all indices
	for (int i = 0; i < gen_index_count; i += 3) {
		ERR_FAIL_INDEX_V(gen_vertices[gen_indices[i + 0]], (int)uv_indices.size(), ERR_BUG);
		ERR_FAIL_INDEX_V(gen_vertices[gen_indices[i + 1]], (int)uv_indices.size(), ERR_BUG);
		ERR_FAIL_INDEX_V(gen_vertices[gen_indices[i + 2]], (int)uv_indices.size(), ERR_BUG);

		ERR_FAIL_COND_V(uv_indices[gen_vertices[gen_indices[i + 0]]].first != uv_indices[gen_vertices[gen_indices[i + 1]]].first || uv_indices[gen_vertices[gen_indices[i + 0]]].first != uv_indices[gen_vertices[gen_indices[i + 2]]].first, ERR_BUG);

		int surface = uv_indices[gen_vertices[gen_indices[i + 0]]].first;

		for (int j = 0; j < 3; j++) {
			SurfaceTool::Vertex v = lightmap_surfaces[surface].vertices[uv_indices[gen_vertices[gen_indices[i + j]]].second];

			if (lightmap_surfaces[surface].format & ARRAY_FORMAT_COLOR) {
				surfaces_tools[surface]->add_color(v.color);
			}
			if (lightmap_surfaces[surface].format & ARRAY_FORMAT_TEX_UV) {
				surfaces_tools[surface]->add_uv(v.uv);
			}
			if (lightmap_surfaces[surface].format & ARRAY_FORMAT_NORMAL) {
				surfaces_tools[surface]->add_normal(v.normal);
			}
			if (lightmap_surfaces[surface].format & ARRAY_FORMAT_TANGENT) {
				Plane t;
				t.normal = v.tangent;
				t.d = v.binormal.dot(v.normal.cross(v.tangent)) < 0 ? -1 : 1;
				surfaces_tools[surface]->add_tangent(t);
			}
			if (lightmap_surfaces[surface].format & ARRAY_FORMAT_BONES) {
				surfaces_tools[surface]->add_bones(v.bones);
			}
			if (lightmap_surfaces[surface].format & ARRAY_FORMAT_WEIGHTS) {
				surfaces_tools[surface]->add_weights(v.weights);
			}

			Vector2 uv2(gen_uvs[gen_indices[i + j] * 2 + 0], gen_uvs[gen_indices[i + j] * 2 + 1]);
			surfaces_tools[surface]->add_uv2(uv2);

			surfaces_tools[surface]->add_vertex(v.vertex);
		}
	}

	//generate surfaces
	for (unsigned int i = 0; i < surfaces_tools.size(); i++) {
		surfaces_tools[i]->index();
		surfaces_tools[i]->commit(Ref<ArrayMesh>((ArrayMesh *)this), lightmap_surfaces[i].format);
	}
*/


	xatlas::Destroy(atlas);

	::free(r_vertex);
	::free(r_index);
	::free(r_uv);

	return true;

#ifdef XATLAS_PRESENT
#else
	return false;
#endif
}

MeshDataResource::MeshDataResource() {
}

MeshDataResource::~MeshDataResource() {
	_arrays.clear();
	_collision_shapes.clear();
}

void MeshDataResource::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_array"), &MeshDataResource::get_array);
	ClassDB::bind_method(D_METHOD("set_array", "array"), &MeshDataResource::set_array);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "array"), "set_array", "get_array");

	ClassDB::bind_method(D_METHOD("get_aabb"), &MeshDataResource::get_aabb);
	ClassDB::bind_method(D_METHOD("set_aabb", "array"), &MeshDataResource::set_aabb);
	ADD_PROPERTY(PropertyInfo(Variant::AABB, "aabb"), "set_aabb", "get_aabb");

	ClassDB::bind_method(D_METHOD("get_collision_shapes"), &MeshDataResource::get_collision_shapes);
	ClassDB::bind_method(D_METHOD("set_collision_shapes", "array"), &MeshDataResource::set_collision_shapes);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "collision_shapes"), "set_collision_shapes", "get_collision_shapes");

	ClassDB::bind_method(D_METHOD("get_seams"), &MeshDataResource::get_seams);
	ClassDB::bind_method(D_METHOD("set_seams", "array"), &MeshDataResource::set_seams);
	ADD_PROPERTY(PropertyInfo(Variant::POOL_INT_ARRAY, "seams"), "set_seams", "get_seams");

	ClassDB::bind_method(D_METHOD("add_collision_shape", "shape"), &MeshDataResource::add_collision_shape);
	ClassDB::bind_method(D_METHOD("get_collision_shape", "index"), &MeshDataResource::get_collision_shape);
	ClassDB::bind_method(D_METHOD("get_collision_shape_count"), &MeshDataResource::get_collision_shape_count);

	ClassDB::bind_method(D_METHOD("recompute_aabb"), &MeshDataResource::recompute_aabb);
	ClassDB::bind_method(D_METHOD("uv_unwrap", "texel_size"), &MeshDataResource::uv_unwrap, 0.1);
}
