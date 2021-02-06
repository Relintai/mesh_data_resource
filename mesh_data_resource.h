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

#ifndef MESH_DATA_REOURCE_H
#define MESH_DATA_REOURCE_H

#include "core/version.h"

#if VERSION_MAJOR > 3
#include "core/io/resource.h"
#include "core/variant/array.h"
#else
#include "core/resource.h"
#include "core/array.h"
#endif

#include "core/version.h"
#include "scene/resources/mesh.h"

#if VERSION_MAJOR < 4
#include "scene/resources/shape.h"
#else
#include "scene/resources/shape_3d.h"

#define Shape Shape3D
#endif

class MeshDataResource : public Resource {
	GDCLASS(MeshDataResource, Resource);
	RES_BASE_EXTENSION("mdres");

public:
	static const String BINDING_STRING_COLLIDER_TYPE;

	enum ColliderType {
		COLLIDER_TYPE_NONE = 0,
		COLLIDER_TYPE_TRIMESH_COLLISION_SHAPE,
		COLLIDER_TYPE_SINGLE_CONVEX_COLLISION_SHAPE,
		COLLIDER_TYPE_MULTIPLE_CONVEX_COLLISION_SHAPES,
		COLLIDER_TYPE_APPROXIMATED_BOX,
		COLLIDER_TYPE_APPROXIMATED_CAPSULE,
		COLLIDER_TYPE_APPROXIMATED_CYLINDER,
		COLLIDER_TYPE_APPROXIMATED_SPHERE,
	};

public:
	Array get_array();
	void set_array(const Array &p_arrays);

	AABB get_aabb() const;
	void set_aabb(const AABB &aabb);

	void add_collision_shape(const Transform &transform, const Ref<Shape> &shape);
	Ref<Shape> get_collision_shape(const int index);
	Transform get_collision_shape_offset(const int index);
	int get_collision_shape_count() const;

	Vector<Variant> get_collision_shapes();
	void set_collision_shapes(const Vector<Variant> &p_arrays);

	void recompute_aabb();

	MeshDataResource();
	~MeshDataResource();

protected:
	struct MDRData {
		Ref<Shape> shape;
		Transform transform;
	};

protected:
	static void _bind_methods();

private:
	Array _arrays;
	AABB _aabb;
	Vector<MDRData> _collision_shapes;
};

VARIANT_ENUM_CAST(MeshDataResource::ColliderType);

#endif
