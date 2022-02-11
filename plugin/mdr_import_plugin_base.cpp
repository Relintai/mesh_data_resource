/*
Copyright (c) 2019-2022 Péter Magyar

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

#include "mdr_import_plugin_base.h"

#include "core/version.h"

#if VERSION_MAJOR < 4

#include "scene/resources/box_shape.h"
#include "scene/resources/capsule_shape.h"
#include "scene/resources/concave_polygon_shape.h"
#include "scene/resources/convex_polygon_shape.h"
#include "scene/resources/cylinder_shape.h"
#include "scene/resources/shape.h"
#include "scene/resources/sphere_shape.h"

#else

#include "scene/resources/box_shape_3d.h"
#include "scene/resources/capsule_shape_3d.h"
#include "scene/resources/concave_polygon_shape_3d.h"
#include "scene/resources/convex_polygon_shape_3d.h"
#include "scene/resources/cylinder_shape_3d.h"
#include "scene/resources/shape_3d.h"
#include "scene/resources/sphere_shape_3d.h"

#define BoxShape BoxShape3D
#define CapsuleShape CapsuleShape3D
#define ConcavePolygonShape ConcavePolygonShape3D
#define ConvexPolygonShape ConvexPolygonShape3D
#define CylinderShape CylinderShape3D
#define Shape Shape3D
#define SphereShape SphereShape3D

#define PoolVector3Array PackedVector3Array
#define PoolVector2Array PackedVector2Array
#define PoolColorArray PackedColorArray
#define PoolIntArray PackedInt64Array
#define PoolRealArray PackedFloat32Array
#define PoolByteArray PackedByteArray

typedef class RenderingServer VisualServer;
typedef class RenderingServer VS;

template <class N>
class Vector;
template <class N>
using PoolVector = Vector<N>;

#endif

#if MESH_UTILS_PRESENT
#include "../../mesh_utils/mesh_utils.h"
#endif

const String MDRImportPluginBase::BINDING_MDR_IMPORT_TYPE = "Single,Multiple";
const String MDRImportPluginBase::BINDING_MDR_OPTIMIZATION_TYPE = "Off"
#if MESH_UTILS_PRESENT
																  ",Remove Doubles,Remove Doubles Interpolate Normals"
#endif
		;

void MDRImportPluginBase::get_import_options(List<ImportOption> *r_options, int p_preset) const {
	r_options->push_back(ImportOption(PropertyInfo(Variant::INT, "import_type", PROPERTY_HINT_ENUM, BINDING_MDR_IMPORT_TYPE), MDRImportPluginBase::MDR_IMPORT_TIME_SINGLE));

#if MESH_UTILS_PRESENT
	//Normal remove doubles should be the default if mesh utils present as it shouldn't visibly change the mesh
	r_options->push_back(ImportOption(PropertyInfo(Variant::INT, "optimization_type", PROPERTY_HINT_ENUM, BINDING_MDR_OPTIMIZATION_TYPE), MDRImportPluginBase::MDR_OPTIMIZATION_REMOVE_DOUBLES));
#else
	r_options->push_back(ImportOption(PropertyInfo(Variant::INT, "optimization_type", PROPERTY_HINT_ENUM, BINDING_MDR_OPTIMIZATION_TYPE), MDRImportPluginBase::MDR_OPTIMIZATION_OFF));
#endif

	r_options->push_back(ImportOption(PropertyInfo(Variant::INT, "collider_type", PROPERTY_HINT_ENUM, MeshDataResource::BINDING_STRING_COLLIDER_TYPE), MeshDataResource::COLLIDER_TYPE_NONE));

	r_options->push_back(ImportOption(PropertyInfo(Variant::VECTOR3, "offset"), Vector3(0, 0, 0)));
	r_options->push_back(ImportOption(PropertyInfo(Variant::VECTOR3, "rotation"), Vector3(0, 0, 0)));
	r_options->push_back(ImportOption(PropertyInfo(Variant::VECTOR3, "scale"), Vector3(1, 1, 1)));

	r_options->push_back(ImportOption(PropertyInfo(Variant::BOOL, "save_copy_as_resource"), false));
}

bool MDRImportPluginBase::get_option_visibility(const String &p_option, const Map<StringName, Variant> &p_options) const {
	return true;
}

Error MDRImportPluginBase::process_node(Node *n, const String &p_source_file, const String &p_save_path, const Map<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata) {
	MDRImportPluginBase::MDRImportType type = static_cast<MDRImportPluginBase::MDRImportType>(static_cast<int>(p_options["import_type"]));

	switch (type) {
		case MDR_IMPORT_TIME_SINGLE: {
			return process_node_single(n, p_source_file, p_save_path, p_options, r_platform_variants, r_gen_files, r_metadata);
		}

			//case MDR_IMPORT_TIME_SINGLE_MERGED: {
			//	ERR_FAIL_V_MSG(Error::ERR_UNAVAILABLE, "import type Single Merged is not yet implemented! " + p_source_file);
			//}

		case MDR_IMPORT_TIME_MULTIPLE: {
			bool save_copy_as_resource = static_cast<bool>(p_options["save_copy_as_resource"]);

			Ref<MeshDataResourceCollection> coll;
			coll.instance();

			process_node_multi(n, p_source_file, p_save_path, p_options, r_platform_variants, r_gen_files, r_metadata, coll);

			if (save_copy_as_resource) {
				save_mdrcoll_copy_as_tres(p_source_file, coll);
			}

			return ResourceSaver::save(p_save_path + "." + get_save_extension(), coll);
		}

			//case MDR_IMPORT_TIME_SINGLE_WITH_SEPARATED_BONES: {
			//	return process_node_single_separated_bones(n, p_source_file, p_save_path, p_options, r_platform_variants, r_gen_files, r_metadata);
			//}
	}

	return Error::ERR_PARSE_ERROR;
}

int MDRImportPluginBase::get_mesh_count(Node *n) {
	int count = 0;

	for (int i = 0; i < n->get_child_count(); ++i) {
		Node *c = n->get_child(i);

		if (Object::cast_to<MeshInstance>(c)) {
			++count;
		}

		count += get_mesh_count(c);
	}

	return count;
}

Error MDRImportPluginBase::process_node_single(Node *n, const String &p_source_file, const String &p_save_path, const Map<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata) {
#if MESH_UTILS_PRESENT
	MDRImportPluginBase::MDROptimizationType optimization_type = static_cast<MDRImportPluginBase::MDROptimizationType>(static_cast<int>(p_options["optimization_type"]));
#endif

	bool save_copy_as_resource = static_cast<bool>(p_options["save_copy_as_resource"]);
	MeshDataResource::ColliderType collider_type = static_cast<MeshDataResource::ColliderType>(static_cast<int>(p_options["collider_type"]));

	Vector3 scale = p_options["scale"];

	ERR_FAIL_COND_V(n == NULL, Error::ERR_PARSE_ERROR);

	for (int i = 0; i < n->get_child_count(); ++i) {
		Node *c = n->get_child(i);

		if (Object::cast_to<MeshInstance>(c)) {
			MeshInstance *mi = Object::cast_to<MeshInstance>(c);

			Ref<MeshDataResource> mdr = get_mesh(mi, p_options, collider_type, scale);

#if MESH_UTILS_PRESENT
			switch (optimization_type) {
				case MDR_OPTIMIZATION_OFF:
					break;
				case MDR_OPTIMIZATION_REMOVE_DOUBLES:
					mdr->set_array(MeshUtils::get_singleton()->remove_doubles(mdr->get_array()));
					break;
				case MDR_OPTIMIZATION_REMOVE_DOUBLES_INTERPOLATE_NORMALS:
					mdr->set_array(MeshUtils::get_singleton()->remove_doubles_interpolate_normals(mdr->get_array()));
					break;
			}
#endif

			ERR_FAIL_COND_V(!mdr.is_valid(), Error::ERR_PARSE_ERROR);

			if (save_copy_as_resource) {
				save_mdr_copy_as_tres(p_source_file, mdr);
			}

			return ResourceSaver::save(p_save_path + "." + get_save_extension(), mdr);
		}

		if (process_node_single(c, p_source_file, p_save_path, p_options, r_platform_variants, r_gen_files, r_metadata) == Error::OK) {
			return Error::OK;
		}
	}

	return Error::ERR_PARSE_ERROR;
}

Error MDRImportPluginBase::process_node_single_separated_bones(Node *n, const String &p_source_file, const String &p_save_path, const Map<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata) {
#if MESH_UTILS_PRESENT
	MDRImportPluginBase::MDROptimizationType optimization_type = static_cast<MDRImportPluginBase::MDROptimizationType>(static_cast<int>(p_options["optimization_type"]));
#endif

	MeshDataResource::ColliderType collider_type = static_cast<MeshDataResource::ColliderType>(static_cast<int>(p_options["collider_type"]));
	bool save_copy_as_resource = static_cast<bool>(p_options["save_copy_as_resource"]);

	Vector3 scale = p_options["scale"];

	ERR_FAIL_COND_V(n == NULL, Error::ERR_PARSE_ERROR);

	for (int i = 0; i < n->get_child_count(); ++i) {
		Node *c = n->get_child(i);

		if (Object::cast_to<MeshInstance>(c)) {
			Ref<MeshDataResourceCollection> coll;
			coll.instance();

			MeshInstance *mi = Object::cast_to<MeshInstance>(c);

			Ref<ArrayMesh> mesh = mi->get_mesh();

			if (!mesh.is_valid())
				continue;

			Vector<Array> arrays = split_mesh_bones(mesh);

			for (int j = 0; j < arrays.size(); ++j) {
				Array arr = arrays[j];

				Ref<MeshDataResource> mdr = get_mesh_arrays(arr, p_options, collider_type, scale);

				if (!mdr.is_valid())
					continue;

#if MESH_UTILS_PRESENT
				switch (optimization_type) {
					case MDR_OPTIMIZATION_OFF:
						break;
					case MDR_OPTIMIZATION_REMOVE_DOUBLES:
						mdr->set_array(MeshUtils::get_singleton()->remove_doubles(mdr->get_array()));
						break;
					case MDR_OPTIMIZATION_REMOVE_DOUBLES_INTERPOLATE_NORMALS:
						mdr->set_array(MeshUtils::get_singleton()->remove_doubles_interpolate_normals(mdr->get_array()));
						break;
				}
#endif

				String node_name = c->get_name();
				node_name = node_name.to_lower();
				String filename = p_source_file.get_basename() + "_" + node_name + "_" + String::num(j) + "." + get_save_extension();

				Error err = ResourceSaver::save(filename, mdr);

				ERR_CONTINUE(err != Error::OK);

				Ref<MeshDataResource> mdrl = ResourceLoader::load(filename);

				coll->add_mdr(mdrl);
			}

			if (save_copy_as_resource) {
				save_mdrcoll_copy_as_tres(p_source_file, coll);
			}

			return ResourceSaver::save(p_save_path + "." + get_save_extension(), coll);
		}

		if (process_node_single_separated_bones(c, p_source_file, p_save_path, p_options, r_platform_variants, r_gen_files, r_metadata) == Error::OK) {
			return Error::OK;
		}
	}

	return Error::ERR_PARSE_ERROR;
}

Error MDRImportPluginBase::process_node_multi(Node *n, const String &p_source_file, const String &p_save_path, const Map<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata, Ref<MeshDataResourceCollection> coll) {
#if MESH_UTILS_PRESENT
	MDRImportPluginBase::MDROptimizationType optimization_type = static_cast<MDRImportPluginBase::MDROptimizationType>(static_cast<int>(p_options["optimization_type"]));
#endif

	MeshDataResource::ColliderType collider_type = static_cast<MeshDataResource::ColliderType>(static_cast<int>(p_options["collider_type"]));

	Vector3 scale = p_options["scale"];

	ERR_FAIL_COND_V(n == NULL, Error::ERR_PARSE_ERROR);

	for (int i = 0; i < n->get_child_count(); ++i) {
		Node *c = n->get_child(i);

		if (Object::cast_to<MeshInstance>(c)) {
			MeshInstance *mi = Object::cast_to<MeshInstance>(c);

			Ref<MeshDataResource> mdr = get_mesh(mi, p_options, collider_type, scale);

#if MESH_UTILS_PRESENT
			switch (optimization_type) {
				case MDR_OPTIMIZATION_OFF:
					break;
				case MDR_OPTIMIZATION_REMOVE_DOUBLES:
					mdr->set_array(MeshUtils::get_singleton()->remove_doubles(mdr->get_array()));
					break;
				case MDR_OPTIMIZATION_REMOVE_DOUBLES_INTERPOLATE_NORMALS:
					mdr->set_array(MeshUtils::get_singleton()->remove_doubles_interpolate_normals(mdr->get_array()));
					break;
			}
#endif

			String node_name = c->get_name();
			node_name = node_name.to_lower();
			String filename = p_source_file.get_basename() + "_" + node_name + "." + get_save_extension();

			Error err = ResourceSaver::save(filename, mdr);

			Ref<MeshDataResource> mdrl = ResourceLoader::load(filename);

			coll->add_mdr(mdrl);

			if (err != Error::OK) {
				return err;
			}
		}

		process_node_multi(c, p_source_file, p_save_path, p_options, r_platform_variants, r_gen_files, r_metadata, coll);
	}

	return Error::OK;
}

Ref<MeshDataResource> MDRImportPluginBase::get_mesh(MeshInstance *mi, const Map<StringName, Variant> &p_options, MeshDataResource::ColliderType collider_type, Vector3 scale) {
	Ref<ArrayMesh> mesh = mi->get_mesh();

	if (mesh.is_valid()) {
		Ref<MeshDataResource> mdr;
		mdr.instance();

		Array arrays = mesh->surface_get_arrays(0);

		mdr->set_array(apply_transforms(arrays, p_options));

		if (collider_type == MeshDataResource::COLLIDER_TYPE_TRIMESH_COLLISION_SHAPE) {
			Ref<ArrayMesh> m;
			m.instance();
			m->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, mdr->get_array());

			Ref<Shape> shape = m->create_trimesh_shape();

			if (!shape.is_null()) {
				mdr->add_collision_shape(Transform(), scale_shape(shape, scale));
			}
		} else if (collider_type == MeshDataResource::COLLIDER_TYPE_SINGLE_CONVEX_COLLISION_SHAPE) {
			Ref<ArrayMesh> m;
			m.instance();
			m->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, mdr->get_array());

			Ref<Shape> shape = mesh->create_convex_shape();

			if (!shape.is_null()) {
				mdr->add_collision_shape(Transform(), scale_shape(shape, scale));
			}
		} else if (collider_type == MeshDataResource::COLLIDER_TYPE_MULTIPLE_CONVEX_COLLISION_SHAPES) {
			Ref<ArrayMesh> m;
			m.instance();
			m->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, mdr->get_array());

			Vector<Ref<Shape>> shapes = mesh->convex_decompose();

			for (int j = 0; j < shapes.size(); ++j) {
				scale_shape(shapes[j], scale);
			}

			for (int j = 0; j < shapes.size(); ++j) {
				mdr->add_collision_shape(Transform(), shapes[j]);
			}
		} else if (collider_type == MeshDataResource::COLLIDER_TYPE_APPROXIMATED_BOX) {
			Ref<ArrayMesh> m;
			m.instance();
			m->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, mdr->get_array());

			Ref<BoxShape> shape;
			shape.instance();

			AABB aabb = m->get_aabb();
			Vector3 size = aabb.get_size();

#if VERSION_MAJOR > 3
			shape->set_size(size * 0.5);
#else
			shape->set_extents(size * 0.5);
#endif

			Vector3 pos = aabb.position;
			pos += size / 2.0;

			Transform t;
			t.origin = pos;

			mdr->add_collision_shape(t, shape);
		} else if (collider_type == MeshDataResource::COLLIDER_TYPE_APPROXIMATED_CAPSULE) {
			Ref<ArrayMesh> m;
			m.instance();
			m->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, mdr->get_array());

			Ref<CapsuleShape> shape;
			shape.instance();

			AABB aabb = m->get_aabb();
			Vector3 size = aabb.get_size();

			shape->set_height(size.y * 0.5);
			shape->set_radius(MIN(size.x, size.z) * 0.5);

			Vector3 pos = aabb.position;
			pos += size / 2.0;

			Transform t = Transform(Basis().rotated(Vector3(1, 0, 0), M_PI_2));
			t.origin = pos;

			mdr->add_collision_shape(t, shape);
		} else if (collider_type == MeshDataResource::COLLIDER_TYPE_APPROXIMATED_CYLINDER) {
			Ref<ArrayMesh> m;
			m.instance();
			m->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, mdr->get_array());

			Ref<CylinderShape> shape;
			shape.instance();

			AABB aabb = m->get_aabb();
			Vector3 size = aabb.get_size();

			shape->set_height(size.y * 0.5);
			shape->set_radius(MIN(size.x, size.z) * 0.5);

			Vector3 pos = aabb.position;
			pos += size / 2.0;

			Transform t;
			t.origin = pos;

			mdr->add_collision_shape(t, shape);
		} else if (collider_type == MeshDataResource::COLLIDER_TYPE_APPROXIMATED_SPHERE) {
			Ref<ArrayMesh> m;
			m.instance();
			m->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, mdr->get_array());

			Ref<SphereShape> shape;
			shape.instance();

			AABB aabb = m->get_aabb();
			Vector3 size = aabb.get_size();

			shape->set_radius(MIN(size.x, MIN(size.y, size.z)) * 0.5);

			Vector3 mid = aabb.get_size() / 2.0;

			Transform t;
			t.origin = aabb.position + mid;

			mdr->add_collision_shape(t, shape);
		}

		return mdr;
	}

	return Ref<MeshDataResource>();
}

Ref<MeshDataResource> MDRImportPluginBase::get_mesh_arrays(Array &arrs, const Map<StringName, Variant> &p_options, MeshDataResource::ColliderType collider_type, Vector3 scale) {
	ERR_FAIL_COND_V(arrs.size() < VS::ARRAY_MAX, Ref<MeshDataResource>());

	Ref<ArrayMesh> mesh;
	mesh.instance();
	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrs);

	ERR_FAIL_COND_V(mesh->get_surface_count() == 0, Ref<MeshDataResource>());

	Ref<MeshDataResource> mdr;
	mdr.instance();

	Array arrays = mesh->surface_get_arrays(0);

	mdr->set_array(apply_transforms(arrays, p_options));

	if (collider_type == MeshDataResource::COLLIDER_TYPE_TRIMESH_COLLISION_SHAPE) {
		Ref<ArrayMesh> m;
		m.instance();
		m->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, mdr->get_array());

		Ref<Shape> shape = m->create_trimesh_shape();

		if (!shape.is_null()) {
			mdr->add_collision_shape(Transform(), scale_shape(shape, scale));
		}
	} else if (collider_type == MeshDataResource::COLLIDER_TYPE_SINGLE_CONVEX_COLLISION_SHAPE) {
		Ref<ArrayMesh> m;
		m.instance();
		m->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, mdr->get_array());

		Ref<Shape> shape = mesh->create_convex_shape();

		if (!shape.is_null()) {
			mdr->add_collision_shape(Transform(), scale_shape(shape, scale));
		}
	} else if (collider_type == MeshDataResource::COLLIDER_TYPE_MULTIPLE_CONVEX_COLLISION_SHAPES) {
		Ref<ArrayMesh> m;
		m.instance();
		m->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, mdr->get_array());

		Vector<Ref<Shape>> shapes = mesh->convex_decompose();

		for (int j = 0; j < shapes.size(); ++j) {
			scale_shape(shapes[j], scale);
		}

		for (int j = 0; j < shapes.size(); ++j) {
			mdr->add_collision_shape(Transform(), shapes[j]);
		}
	} else if (collider_type == MeshDataResource::COLLIDER_TYPE_APPROXIMATED_BOX) {
		Ref<ArrayMesh> m;
		m.instance();
		m->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, mdr->get_array());

		Ref<BoxShape> shape;
		shape.instance();

		AABB aabb = m->get_aabb();
		Vector3 size = aabb.get_size();

#if VERSION_MAJOR > 3
		shape->set_size(size * 0.5);
#else
		shape->set_extents(size * 0.5);
#endif

		Vector3 pos = aabb.position;
		pos += size / 2.0;

		Transform t;
		t.origin = pos;

		mdr->add_collision_shape(t, shape);
	} else if (collider_type == MeshDataResource::COLLIDER_TYPE_APPROXIMATED_CAPSULE) {
		Ref<ArrayMesh> m;
		m.instance();
		m->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, mdr->get_array());

		Ref<CapsuleShape> shape;
		shape.instance();

		AABB aabb = m->get_aabb();
		Vector3 size = aabb.get_size();

		shape->set_height(size.y * 0.5);
		shape->set_radius(MIN(size.x, size.z) * 0.5);

		Vector3 pos = aabb.position;
		pos += size / 2.0;

		Transform t = Transform(Basis().rotated(Vector3(1, 0, 0), M_PI_2));
		t.origin = pos;

		mdr->add_collision_shape(t, shape);
	} else if (collider_type == MeshDataResource::COLLIDER_TYPE_APPROXIMATED_CYLINDER) {
		Ref<ArrayMesh> m;
		m.instance();
		m->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, mdr->get_array());

		Ref<CylinderShape> shape;
		shape.instance();

		AABB aabb = m->get_aabb();
		Vector3 size = aabb.get_size();

		shape->set_height(size.y * 0.5);
		shape->set_radius(MIN(size.x, size.z) * 0.5);

		Vector3 pos = aabb.position;
		pos += size / 2.0;

		Transform t;
		t.origin = pos;

		mdr->add_collision_shape(t, shape);
	} else if (collider_type == MeshDataResource::COLLIDER_TYPE_APPROXIMATED_SPHERE) {
		Ref<ArrayMesh> m;
		m.instance();
		m->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, mdr->get_array());

		Ref<SphereShape> shape;
		shape.instance();

		AABB aabb = m->get_aabb();
		Vector3 size = aabb.get_size();

		shape->set_radius(MIN(size.x, MIN(size.y, size.z)) * 0.5);

		Vector3 mid = aabb.get_size() / 2.0;

		Transform t;
		t.origin = aabb.position + mid;

		mdr->add_collision_shape(t, shape);
	}

	return mdr;
}

Vector<Array> MDRImportPluginBase::split_mesh_bones(Ref<ArrayMesh> mesh) {
	Array arr = mesh->surface_get_arrays(0);

	PoolVector<int> barr = arr[VS::ARRAY_BONES];

	Vector<Array> resarrs;

	Vector<int> bone_ids;

	for (int i = 0; i < barr.size(); ++i) {
		int bi = barr[i];

		if (bone_ids.find(bi) == -1) {
			bone_ids.push_back(bi);
		}
	}

	for (int i = 0; i < bone_ids.size(); ++i) {
		int bi = bone_ids[i];

		Array carr = slice_mesh_bone(arr, bi);

		if (carr.size() == VS::ARRAY_MAX) {
			PoolVector<Vector3> vcarr = carr[VS::ARRAY_VERTEX];

			if (vcarr.size() != 0)
				resarrs.push_back(slice_mesh_bone(carr, bi));
		}
	}

	return resarrs;
}

Array MDRImportPluginBase::slice_mesh_bone(const Array &arr, int bone_idx) {
	Array resarrs;
	resarrs.resize(VS::ARRAY_MAX);

	PoolVector<Vector3> varr = arr[VS::ARRAY_VERTEX];
	PoolVector<Vector3> narr = arr[VS::ARRAY_NORMAL];
	PoolVector<float> tarr = arr[VS::ARRAY_TANGENT];
	PoolVector<Color> carr = arr[VS::ARRAY_COLOR];
	PoolVector<Vector2> uvarr = arr[VS::ARRAY_TEX_UV];
	PoolVector<Vector2> uv2arr = arr[VS::ARRAY_TEX_UV2];
	PoolVector<int> barr = arr[VS::ARRAY_BONES];
	PoolVector<float> warr = arr[VS::ARRAY_WEIGHTS];

	PoolVector<int> iarr = arr[VS::ARRAY_INDEX];

	PoolVector<Vector3> rvarr;
	PoolVector<Vector3> rnarr;
	PoolVector<float> rtarr;
	PoolVector<Color> rcarr;
	PoolVector<Vector2> ruvarr;
	PoolVector<Vector2> ruv2arr;
	PoolVector<int> rbarr;
	PoolVector<float> rwarr;
	PoolVector<int> riarr;

	Vector<int> curr_bone_indices;

	for (int i = 0; i < barr.size(); i += 4) {
		int b0 = barr[i + 0];
		//int b1 = barr[i + 1];
		//int b2 = barr[i + 2];
		//int b3 = barr[i + 3];

		if (b0 == bone_idx) { // || b1 == bone_idx || b2 == bone_idx || b3 == bone_idx) {
			curr_bone_indices.push_back(i / 4);
		}
	}

	for (int i = 0; i < curr_bone_indices.size(); ++i) {
		int indx = curr_bone_indices[i];

		rvarr.push_back(varr[indx]);

		if (narr.size() > 0)
			rnarr.push_back(narr[indx]);

		if (tarr.size() > 0) {
			int ii = indx * 4;

			tarr.push_back(tarr[ii]);
			tarr.push_back(tarr[ii + 1]);
			tarr.push_back(tarr[ii + 2]);
			tarr.push_back(tarr[ii + 3]);
		}

		if (carr.size() > 0)
			rcarr.push_back(carr[indx]);

		if (uvarr.size() > 0)
			ruvarr.push_back(uvarr[indx]);

		if (uv2arr.size() > 0)
			ruv2arr.push_back(uv2arr[indx]);

		if (barr.size() > 0) {
			int ii = indx * 4;

			rbarr.push_back(barr[ii]);
			rbarr.push_back(0);
			rbarr.push_back(0);
			rbarr.push_back(0);

			//rbarr.set(ii + 1, barr[indx + 1]);
			//rbarr.set(ii + 2, barr[indx + 2]);
			//rbarr.set(ii + 3, barr[indx + 3]);
		}

		if (warr.size() > 0) {
			int ii = indx * 4;

			rwarr.push_back(warr[ii]);
			rwarr.push_back(0);
			rwarr.push_back(0);
			rwarr.push_back(0);

			//rwarr.set(ii + 1, warr[indx + 1]);
			//rwarr.set(ii + 2, warr[indx + 2]);
			//rwarr.set(ii + 3, warr[indx + 3]);
		}
	}

	for (int i = 0; i < curr_bone_indices.size(); ++i) {
		int old_indx = curr_bone_indices[i];
		int new_indx = i;

		for (int j = 0; j < i; ++j) {
			if (curr_bone_indices[j] == old_indx) {
				new_indx = j;
				break;
			}
		}

		riarr.push_back(new_indx);
	}

	resarrs[VS::ARRAY_VERTEX] = rvarr;

	if (rnarr.size() > 0)
		resarrs[VS::ARRAY_NORMAL] = rnarr;

	if (rtarr.size() > 0)
		resarrs[VS::ARRAY_TANGENT] = rtarr;

	if (rcarr.size() > 0)
		resarrs[VS::ARRAY_COLOR] = rcarr;

	if (ruvarr.size() > 0)
		resarrs[VS::ARRAY_TEX_UV] = ruvarr;

	if (ruv2arr.size() > 0)
		resarrs[VS::ARRAY_TEX_UV2] = ruv2arr;

	if (rbarr.size() > 0)
		resarrs[VS::ARRAY_BONES] = rbarr;

	if (rwarr.size() > 0)
		resarrs[VS::ARRAY_WEIGHTS] = rwarr;

	resarrs[VS::ARRAY_INDEX] = riarr;

	return resarrs;
}

Array MDRImportPluginBase::apply_transforms(Array &array, const Map<StringName, Variant> &p_options) {
	Vector3 offset = p_options["offset"];
	Vector3 rotation = p_options["rotation"];
	Vector3 scale = p_options["scale"];

	ERR_FAIL_COND_V(array.size() < VS::ARRAY_MAX, array);

	Transform transform = Transform(Basis(rotation).scaled(scale), offset);

	PoolVector3Array verts = array.get(Mesh::ARRAY_VERTEX);

	for (int i = 0; i < verts.size(); ++i) {
		Vector3 vert = verts[i];

		vert = transform.xform(vert);

		verts.set(i, (vert));
	}

	PoolVector3Array normals = array.get(Mesh::ARRAY_NORMAL);

	for (int i = 0; i < normals.size(); ++i) {
		Vector3 normal = normals[i];

		normal = transform.basis.xform(normal);

		normals.set(i, normal);
	}
	/*
	Array tangents = array.get(Mesh::ARRAY_TANGENT);

	if (tangents.size() == verts.size() * 4) {

		for (int i = 0; i < verts.size(); ++i) {

			Plane p(tangents[i * 4 + 0], tangents[i * 4 + 1], tangents[i * 4 + 2], tangents[i * 4 + 3]);

			Vector3 tangent = p.normal;

			tangent = transform.basis.xform(tangent);

			tangents.set(i, tangent);
		}
	}
*/
	array.set(Mesh::ARRAY_VERTEX, verts);
	array.set(Mesh::ARRAY_NORMAL, normals);
	//array.set(Mesh::ARRAY_TANGENT, tangents);

	return array;
}

Ref<Shape> MDRImportPluginBase::scale_shape(Ref<Shape> shape, const Vector3 &scale) {
	if (shape.is_null())
		return shape;

	if (Object::cast_to<SphereShape>(*shape)) {
		Ref<SphereShape> ss = shape;

		ss->set_radius(ss->get_radius() * MAX(scale.x, MAX(scale.y, scale.z)));
	}

	if (Object::cast_to<BoxShape>(*shape)) {
		Ref<BoxShape> bs = shape;

#if VERSION_MAJOR > 3
		bs->set_size(bs->get_size() * scale);
#else
		bs->set_extents(bs->get_extents() * scale);
#endif
	}

	if (Object::cast_to<CapsuleShape>(*shape)) {
		Ref<CapsuleShape> cs = shape;

		float sc = MAX(scale.x, MAX(scale.y, scale.z));

		cs->set_radius(cs->get_radius() * sc);
		cs->set_height(cs->get_height() * sc);
	}

	if (Object::cast_to<CylinderShape>(*shape)) {
		Ref<CylinderShape> cs = shape;

		float sc = MAX(scale.x, MAX(scale.y, scale.z));

		cs->set_radius(cs->get_radius() * sc);
		cs->set_height(cs->get_height() * sc);
	}

	if (Object::cast_to<ConcavePolygonShape>(*shape)) {
		Ref<ConcavePolygonShape> cps = shape;

		PoolVector3Array arr = cps->get_faces();

		Basis b = Basis().scaled(scale);

		for (int i = 0; i < arr.size(); ++i) {
			arr.set(i, b.xform(arr[i]));
		}

		cps->set_faces(arr);
	}

	if (Object::cast_to<ConvexPolygonShape>(*shape)) {
		Ref<ConvexPolygonShape> cps = shape;

		PoolVector3Array arr = cps->get_points();

		Basis b = Basis().scaled(scale);

		for (int i = 0; i < arr.size(); ++i) {
			arr.set(i, b.xform(arr[i]));
		}

		cps->set_points(arr);
	}

	return shape;
}

void MDRImportPluginBase::save_mdr_copy_as_tres(const String &p_source_file, const Ref<MeshDataResource> &res) {
	String sp = p_source_file;
	String ext = p_source_file.get_extension();
	sp.resize(sp.size() - ext.size());
	sp += ".tres";

	ResourceSaver::save(sp, res);
}
void MDRImportPluginBase::save_mdrcoll_copy_as_tres(const String &p_source_file, const Ref<MeshDataResource> &res) {
	String sp = p_source_file;
	String ext = p_source_file.get_extension();
	sp.resize(sp.size() - ext.size());
	sp += ".tres";

	ResourceSaver::save(sp, res);
}

MDRImportPluginBase::MDRImportPluginBase() {
}

MDRImportPluginBase::~MDRImportPluginBase() {
}
