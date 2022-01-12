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

#ifndef MDR_IMPORT_PLUGIN_BASE
#define MDR_IMPORT_PLUGIN_BASE

#include "core/version.h"

#if VERSION_MAJOR > 3
#include "core/string/ustring.h"
#include "core/variant/array.h"
#else
#include "core/array.h"
#include "core/ustring.h"
#endif

#include "../mesh_data_resource_collection.h"
#include "core/io/resource_saver.h"
#include "core/math/basis.h"
#include "core/math/transform.h"
#include "editor/import/editor_import_plugin.h"
#include "scene/main/node.h"
#include "scene/resources/mesh.h"

#include "../mesh_data_resource.h"

#include "core/math/transform.h"

#include "core/version.h"

#if VERSION_MAJOR < 4
#include "scene/3d/mesh_instance.h"

#if VERSION_MINOR < 4
#include "editor/import/editor_scene_importer_gltf.h"
#else
#include "../../gltf/editor_scene_importer_gltf.h"
#endif

#else
#include "../../gltf/editor_scene_importer_gltf.h"
#include "scene/3d/mesh_instance_3d.h"

#define MeshInstance MeshInstance3D
#endif

class MDRImportPluginBase : public EditorImportPlugin {
	GDCLASS(MDRImportPluginBase, EditorImportPlugin);

public:
	static const String BINDING_MDR_IMPORT_TYPE;
	static const String BINDING_MDR_OPTIMIZATION_TYPE;

	enum MDRImportType {
		MDR_IMPORT_TIME_SINGLE = 0,
		//MDR_IMPORT_TIME_SINGLE_MERGED,
		MDR_IMPORT_TIME_MULTIPLE,
		//MDR_IMPORT_TIME_SINGLE_WITH_SEPARATED_BONES,
	};

	enum MDROptimizationType {
		MDR_OPTIMIZATION_OFF = 0,
#if MESH_UTILS_PRESENT
		MDR_OPTIMIZATION_REMOVE_DOUBLES,
		MDR_OPTIMIZATION_REMOVE_DOUBLES_INTERPOLATE_NORMALS,
#endif
	};

public:
	virtual void get_import_options(List<ImportOption> *r_options, int p_preset = 0) const;
	virtual bool get_option_visibility(const String &p_option, const Map<StringName, Variant> &p_options) const;

	Error process_node(Node *n, const String &p_source_file, const String &p_save_path, const Map<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files = NULL, Variant *r_metadata = NULL);

	int get_mesh_count(Node *n);
	Error process_node_single(Node *n, const String &p_source_file, const String &p_save_path, const Map<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata);
	Error process_node_single_separated_bones(Node *n, const String &p_source_file, const String &p_save_path, const Map<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata);
	Error process_node_multi(Node *n, const String &p_source_file, const String &p_save_path, const Map<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata, Ref<MeshDataResourceCollection> coll);
	Ref<MeshDataResource> get_mesh(MeshInstance *mi, const Map<StringName, Variant> &p_options, MeshDataResource::ColliderType collider_type, Vector3 scale);
	Ref<MeshDataResource> get_mesh_arrays(Array &arrs, const Map<StringName, Variant> &p_options, MeshDataResource::ColliderType collider_type, Vector3 scale);

	Vector<Array> split_mesh_bones(Ref<ArrayMesh> mesh);
	Array slice_mesh_bone(const Array &array, int bone_idx);
	Array apply_transforms(Array &array, const Map<StringName, Variant> &p_options);
	Ref<Shape> scale_shape(Ref<Shape> shape, const Vector3 &scale);

	MDRImportPluginBase();
	~MDRImportPluginBase();
};

#endif
