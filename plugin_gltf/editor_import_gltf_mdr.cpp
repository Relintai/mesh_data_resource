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

#include "editor_import_gltf_mdr.h"

String EditorImportGLTFMdr::get_importer_name() const {
	return "gltf_mdr";
}

String EditorImportGLTFMdr::get_visible_name() const {
	return "GLTF MDR";
}

void EditorImportGLTFMdr::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("gltf");
	p_extensions->push_back("glb");
}

String EditorImportGLTFMdr::get_save_extension() const {
	return "res";
}

String EditorImportGLTFMdr::get_resource_type() const {
	return "MeshDataResource";
}

float EditorImportGLTFMdr::get_priority() const {
	return 1.0;
}

int EditorImportGLTFMdr::get_preset_count() const {
	return 0;
}

String EditorImportGLTFMdr::get_preset_name(int p_idx) const {
	return "";
}

void EditorImportGLTFMdr::get_import_options(List<ImportOption> *r_options, int p_preset) const {
	r_options->push_back(ImportOption(PropertyInfo(Variant::VECTOR3, "offset"), Vector3(0, 0, 0)));
	r_options->push_back(ImportOption(PropertyInfo(Variant::VECTOR3, "rotation"), Vector3(0, 0, -(3.141592 / 2.0))));
	r_options->push_back(ImportOption(PropertyInfo(Variant::VECTOR3, "scale"), Vector3(0.011, 0.011, 0.011)));
}

bool EditorImportGLTFMdr::get_option_visibility(const String &p_option, const Map<StringName, Variant> &p_options) const {
	return true;
}

Error EditorImportGLTFMdr::import(const String &p_source_file, const String &p_save_path, const Map<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata) {
	Node *n = _importer->import_scene(p_source_file, 0, 15);

	if (n == NULL) {
		n->queue_delete();
		return Error::ERR_PARSE_ERROR;
	}

	for (int i = 0; i < n->get_child_count(); ++i) {
		Node *c = n->get_child(i);
		print_error(String::num(i));

		if (c == NULL) {
			continue;
		}

		if (Object::cast_to<MeshInstance>(c)) {
			MeshInstance *mi = Object::cast_to<MeshInstance>(c);

			Ref<ArrayMesh> mesh = mi->get_mesh();

			if (mesh.is_valid()) {
				Ref<MeshDataResource> mdr;
				mdr.instance();

				Array arrays = mesh->surface_get_arrays(0);

				mdr->set_array(apply_transforms(arrays, p_options));

				n->queue_delete();

				return ResourceSaver::save(p_save_path + "." + get_save_extension(), mdr);
			}
		}
	}

	n->queue_delete();
	return Error::ERR_PARSE_ERROR;
}

Array EditorImportGLTFMdr::apply_transforms(Array &array, const Map<StringName, Variant> &p_options) {
	Vector3 offset = p_options["offset"];
	Vector3 rotation = p_options["rotation"];
	Vector3 scale = p_options["scale"];

	Transform transform = Transform(Basis(rotation).scaled(scale), offset);

	Array verts = array.get(Mesh::ARRAY_VERTEX);

	for (int i = 0; i < verts.size(); ++i) {
		Vector3 vert = verts[i];

		vert = transform.xform(vert);

		verts.set(i, (vert));
	}

	Array normals = array.get(Mesh::ARRAY_NORMAL);

	for (int i = 0; i < normals.size(); ++i) {
		Vector3 normal = normals[i];

		normal = transform.basis.xform(normal);

		normals.set(i, normal);
	}

	Array tangents = array.get(Mesh::ARRAY_TANGENT);

	if (tangents.size() == verts.size() * 4) {

		for (int i = 0; i < verts.size(); ++i) {

			Plane p(tangents[i * 4 + 0], tangents[i * 4 + 1], tangents[i * 4 + 2], tangents[i * 4 + 3]);

			Vector3 tangent = p.normal;

			tangent = transform.basis.xform(tangent);

			tangents.set(i, tangent);
		}
	}

	array.set(Mesh::ARRAY_VERTEX, verts);
	array.set(Mesh::ARRAY_NORMAL, normals);
	array.set(Mesh::ARRAY_TANGENT, tangents);

	return array;
}

EditorImportGLTFMdr::EditorImportGLTFMdr() {
	_importer.instance();
}

EditorImportGLTFMdr::~EditorImportGLTFMdr() {
	_importer.unref();
}
