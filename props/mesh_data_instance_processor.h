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

#ifndef PROP_MESH_DATA_INSTANCE_PROCESSOR_H
#define PROP_MESH_DATA_INSTANCE_PROCESSOR_H

#if PROPS_PRESENT

#include "../../props/processor/prop_data_processor.h"

#include "core/version.h"
#include "scene/resources/texture.h"

#include "core/math/vector3.h"

#include "../mesh_data_resource.h"

class PropInstance;

class MeshDataInstanceProcessor : public PropDataProcessor {
	GDCLASS(MeshDataInstanceProcessor, PropDataProcessor);

public:
	bool _handles(Node *node);
	void _process(Ref<PropData> prop_data, Node *node, const Transform &transform);

	MeshDataInstanceProcessor();
	~MeshDataInstanceProcessor();

protected:
	static void _bind_methods();

private:
};

#endif

#endif
