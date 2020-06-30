/*
Copyright (c) 2019-2020 Péter Magyar

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

#include "core/version.h"

const String MeshDataResource::BINDING_STRING_COLLIDER_TYPE = "None,Trimesh Collision Shape,Single Convex Collision Shape,Multiple Convex Collision Shapes,Approximated Box,Approximated Capsule,Approximated Cylinder,Approximated Sphere";

Array MeshDataResource::get_array() {
	return _arrays;
}
void MeshDataResource::set_array(const Array &p_arrays) {
	_arrays.clear();

	_arrays = p_arrays.duplicate(true);
}

void MeshDataResource::add_collision_shape(const Vector3 &offset, const Ref<Shape> &shape) {
	MDRData d;

	d.offset = offset;
	d.shape = shape;

	_collision_shapes.push_back(d);
}
Ref<Shape> MeshDataResource::get_collision_shape(const int index) {
	ERR_FAIL_INDEX_V(index, _collision_shapes.size(), Ref<Shape>());

	return _collision_shapes[index].shape;
}
Vector3 MeshDataResource::get_collision_shape_offset(const int index) {
	ERR_FAIL_INDEX_V(index, _collision_shapes.size(), Vector3());

	return _collision_shapes[index].offset;
}
int MeshDataResource::get_collision_shape_count() const {
	return _collision_shapes.size();
}

Vector<Variant> MeshDataResource::get_collision_shapes() {
	Vector<Variant> r;
	for (int i = 0; i < _collision_shapes.size(); i++) {
#if VERSION_MAJOR < 4
		r.push_back(_collision_shapes[i].offset);
		r.push_back(_collision_shapes[i].shape.get_ref_ptr());
#else
		r.push_back(_collision_shapes[i].offset);
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

		d.offset = p_arrays[i];
		d.shape = Ref<Shape>(p_arrays[i + 1]);

		_collision_shapes.push_back(d);
	}
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

	ClassDB::bind_method(D_METHOD("get_collision_shapes"), &MeshDataResource::get_collision_shapes);
	ClassDB::bind_method(D_METHOD("set_collision_shapes", "array"), &MeshDataResource::set_collision_shapes);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "collision_shapes"), "set_collision_shapes", "get_collision_shapes");

	ClassDB::bind_method(D_METHOD("add_collision_shape", "shape"), &MeshDataResource::add_collision_shape);
	ClassDB::bind_method(D_METHOD("get_collision_shape", "index"), &MeshDataResource::get_collision_shape);
	ClassDB::bind_method(D_METHOD("get_collision_shape_count"), &MeshDataResource::get_collision_shape_count);
}
