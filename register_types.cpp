#include "register_types.h"

#include "mesh_data_resource.h"

#ifdef TOOLS_ENABLED
#include "editor/editor_plugin.h"

#include "plugin_collada/editor_plugin_collada_mdr.h"

#include "plugin_gltf/editor_plugin_gltf_mdr.h"
#endif

void register_mesh_data_resource_types() {
	ClassDB::register_class<MeshDataResource>();

#ifdef TOOLS_ENABLED
	EditorPlugins::add_by_type<EditorPluginColladaMdr>();

	EditorPlugins::add_by_type<EditorPluginGLTFMdr>();
#endif
}

void unregister_mesh_data_resource_types() {
}
