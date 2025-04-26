/**************************************************************************
This file is part of JahshakaVR, VR Authoring Toolkit
http://www.jahshaka.com
Copyright (c) 2016  GPLv3 Jahshaka LLC <coders@jahshaka.com>

This is free software: you may copy, redistribute
and/or modify it under the terms of the GPLv3 License

For more information see the LICENSE file
*************************************************************************/
#include "shadergraph.h"
#include "graph/nodegraph.h"
#include "models/nodemodel.h"
#include "nodes/test.h"


ShaderGraph* ShaderGraph::createDefaultShaderGraph()
{
	auto nodeGraph = new ShaderGraph();
	auto masterNode = new SurfaceMasterNode();
	nodeGraph->addNode(masterNode);
	nodeGraph->setMasterNode(masterNode);

	return nodeGraph;
}

// to be implemented
ShaderGraph* ShaderGraph::createParticleShaderGraph()
{
	return nullptr;
}

// to be implemented
ShaderGraph* ShaderGraph::createPBRShaderGraph()
{
	return nullptr;
}