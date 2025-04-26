/**************************************************************************
This file is part of JahshakaVR, VR Authoring Toolkit
http://www.jahshaka.com
Copyright (c) 2016  GPLv3 Jahshaka LLC <coders@jahshaka.com>

This is free software: you may copy, redistribute
and/or modify it under the terms of the GPLv3 License

For more information see the LICENSE file
*************************************************************************/
#include "graphnodescene.h"
#include "../nodes/test.h"
#include "core/project.h"

#include <QMimeData>
#include <QListWidgetItem>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QShortcut>

#include <shadergraph/dialogs/searchdialog.h>




NodeGraph *GraphNodeScene::getNodeGraph() const
{
	return nodeGraph;
}

GraphNode * GraphNodeScene::getNodeByPropertyId(QString id)
{
	return nullptr;
}

void GraphNodeScene::refreshNodeTitle(QString id)
{
	auto node = getNodeById(id);
	if (node) {
		
	}
}

void GraphNodeScene::setNodeGraph(NodeGraph *graph)
{
	// clear previous nodegraph

	// recreate nodes
	nodeGraph = graph;

	//auto masterNode = nodeGraph->getMasterNode();
	//addNodeModel(masterNode, 0, 0, false);

	// add nodes
	for (auto node : graph->nodes.values()) {
		this->addNodeModel(node, false);
	}

	// add connections
	for (auto con : graph->connections.values()) {
		auto leftNode = con->leftSocket->node;
		auto rightNode = con->rightSocket->node;
		auto conn = this->addConnection(leftNode->id,
			leftNode->outSockets.indexOf(con->leftSocket),
			rightNode->id,
			rightNode->inSockets.indexOf(con->rightSocket));

		conn->connectionId = con->id;// very important!
	}
}

void GraphNodeScene::addNodeModel(NodeModel* model, bool addToGraph)
{
	if (model->title != "Surface Material") {
		auto addNodeCommand = new AddNodeCommand(model, this);
		stack->push(addNodeCommand);
	}
	else {
		//add surface node to the scene
		//other nodes get added to the scene from the add node command above
		//on AddNodeCommand, redo gets called - stupid qt
			addNodeModel(model, model->getX(), model->getY(), addToGraph);
	}
}

// add
GraphNode* GraphNodeScene::addNodeModel(NodeModel *model, float x, float y, bool addToGraph)
{
	auto nodeView = this->createNode<GraphNode>();
	nodeView->setNodeGraph(this->nodeGraph);
	nodeView->setModel(model);
	nodeView->setTitle(model->title);
	nodeView->setTitleColor(model->setNodeTitleColor());

	for (auto sock : model->inSockets)
		nodeView->addInSocket(sock);
	for (auto sock : model->outSockets)
		nodeView->addOutSocket(sock);

	if (model->widget != nullptr) {
		nodeView->setWidget(model->widget);
	}

	
	/*nodeView->setTitle(model->title);
	nodeView->setTitleColor(model->setNodeTitleColor());*/
	
	nodeView->setPos(model->getX(), model->getY());
	nodeView->nodeId = model->id;
	nodeView->layout();
//	if (model->title == "Color Node") nodeView->resetPositionForColorWidget();

	if (model->isPreviewEnabled()) {
		nodeView->enablePreviewWidget();
	}

	if (addToGraph) {
		Q_ASSERT_X(nodeGraph != nullptr, "GraphNodeScene::addNodeModel", "Cant add node to null scene");
		nodeGraph->addNode(model);
	}

	connect(model, &NodeModel::valueChanged, [this](NodeModel* nodeModel, int sockedIndex) {
		emit nodeValueChanged(nodeModel, sockedIndex);
		emit graphInvalidated();
	});

	return nodeView;

	//connect(nodeView, &GraphNode::positionUpdated, [=](QPointF one, QPointF two) {
	////	auto moveCommand = new MoveNodeCommand(nodeView,this,one,two);
	////	stack->push(moveCommand);

	//});
}

QMenu *GraphNodeScene::createContextMenu(float x, float y)
{
	auto menu = new QMenu();
    menu->setStyleSheet(
        "QMenu { background-color: #1A1A1A; color: #EEE; padding: 0; margin: 0; }"
        "QMenu::item { background-color: #1A1A1A; padding: 6px 8px; margin: 0; }"
        "QMenu::item:selected { background-color: #3498db; color: #EEE; padding: 6px 8px; margin: 0; }"
        "QMenu::item : disabled { color: #555; }"
    );

	/*
	for(auto key : nodeGraph->modelFactories.keys()) {
	auto factory = nodeGraph->modelFactories[key];
	connect(menu->addAction(key), &QAction::triggered, [this,x, y,factory](){

	auto node = factory();
	this->addNodeModel(node, x, y);

	});
	}
	*/
	for (auto item : nodeGraph->library->getItems()) {
		auto factory = item->factoryFunction;
		connect(menu->addAction(item->displayName), &QAction::triggered, [this, x, y, factory]() {

			auto node = factory();
			node->setX(x);
			node->setY(y);
			this->addNodeModel(node);

		});
	}

	// create properties
	auto propMenu = menu->addMenu("Properties");
	for (auto prop : nodeGraph->properties) {
		connect(propMenu->addAction(prop->displayName), &QAction::triggered, [this, x, y, prop]() {
			auto propNode = new PropertyNode();
			propNode->setProperty(prop);
			this->addNodeModel(propNode, x, y);
		});
	}

	return menu;
}

QMenu * GraphNodeScene::removeConnectionContextMenu(float x, float y)
{
	auto menu = new QMenu();
    menu->setStyleSheet(
        "QMenu { background-color: #1A1A1A; color: #EEE; padding: 0; margin: 0; }"
        "QMenu::item { background-color: #1A1A1A; padding: 6px 8px; margin: 0; }"
        "QMenu::item:selected { background-color: #3498db; color: #EEE; padding: 6px 8px; margin: 0; }"
        "QMenu::item : disabled { color: #555; }"
    );
	auto sock = getSocketAt(x, y);

	auto getAppropriateText = [&](SocketConnection *conn, int i) {
		if (i == 2) return conn->socket2->text->toPlainText();
		if (i == 1) return conn->socket1->text->toPlainText();
	};

	QString string;

	for (SocketConnection* connection : sock->connections) {
		string = QString("remove %1 - %2 connection").arg(getAppropriateText(connection, 1)).arg(getAppropriateText(connection, 2));
		connect(menu->addAction(string), &QAction::triggered, [this, connection]() {
			removeConnection(connection);
		});
	}

	return menu;
}

QJsonObject GraphNodeScene::serialize()
{
	QJsonObject data;

	data["graph"] = this->nodeGraph->serialize();
	QJsonObject scene;

	QJsonArray nodesJson;

	// save nodes

	for (auto item : this->items()) {
		if (item && item->type() == (int)GraphicsItemType::Node) {
			auto node = (GraphNode*)item;
			QJsonObject nodeObj;
			nodeObj["id"] = node->nodeId;
			nodeObj["x"] = node->x();
			nodeObj["y"] = node->y();
			nodesJson.append(nodeObj);
		}
	}

	scene.insert("nodes", nodesJson);
	data["scene"] = scene;

	return data;
}

void GraphNodeScene::updatePropertyNodeTitle(QString title, QString propId)
{

	auto propList = nodeGraph->getNodesByTypeName("property");

	for (auto node : propList) {
		auto propNode = static_cast<PropertyNode *>(node);
		if (propNode->getProperty()->id == propId) {
			auto node = getNodeById(propNode->id);
			node->setTitle(title);
		}
	}

	
}

void GraphNodeScene::addNodeFromSearchDialog(QTreeWidgetItem * item, const QPoint &point)
{
	auto view = this->views().first();
	auto viewPoint = view->viewport()->mapFromGlobal(point);
	auto scenePoint = view->mapToScene(viewPoint);

	auto p = scenePoint;


	if (!item->data(0, MODEL_EXT_ROLE).isNull()) {
		auto prop = nodeGraph->properties.at(item->data(0, MODEL_EXT_ROLE).toInt());
		if (prop) {
			auto propNode = new PropertyNode();
			propNode->setProperty(prop);
			propNode->setX(p.x());
			propNode->setY(p.y());
			this->addNodeModel(propNode);
		}
	}

	if (item->data(0, MODEL_TYPE_ROLE).toString() == "node") {
		auto node = nodeGraph->library->createNode(item->data(0, Qt::UserRole).toString());

		//	auto factory = nodeGraph->modelFactories[event->mimeData()->text()];
		if (node) {
			node->setX(p.x());
			node->setY(p.y());
			this->addNodeModel(node);
			return;
		}
	}
}

void GraphNodeScene::deleteSelectedNodes()
{
	auto items = selectedItems();
	QList<GraphNode*> nodes;
	auto masterNodeId = nodeGraph->getMasterNode()->id;

	// gather all nodes
	for (auto item : items) {
		if (item->type() == (int)GraphicsItemType::Node) {
			auto node = static_cast<GraphNode*>(item);
			if (node->nodeId != masterNodeId)
				nodes.append(node);
		}
	}
	if (nodes.length() > 0)
	{
		auto deleteCommand = new DeleteNodeCommand(nodes, this);
		stack->push(deleteCommand);
	}
	
	// remove each node's connections then remove the nodes
	for (auto node : nodes) {
	//	deleteNode(node);
	}



	emit graphInvalidated();
}

void GraphNodeScene::deleteNode(GraphNode* node)
{
	// remove in and out connections
	auto conns = nodeGraph->getNodeConnections(node->nodeId);

	// remove node from scenegraph
	nodeGraph->removeNode(node->nodeId);

	// remove them from scene
	for (auto con : conns) {
		// false, because the connection has already been removed
		// in nodeGraph->removeNode(node->nodeId);
		this->removeConnection(con->id, false, true);
	}

	this->removeItem(node);

	emit nodeRemoved(node);
}

bool GraphNodeScene::areSocketsComptible(Socket* sock1, Socket* sock2)
{
	Socket* inSock;
	Socket* outSock;
	determineOutAndInSockets(sock1, sock2, &outSock, &inSock);

	// get the socket models and check if they're compatible
	auto outNode = nodeGraph->getNode(outSock->node->nodeId);
	auto outSockModel = outNode->outSockets[outSock->socketIndex];
	auto inNode = nodeGraph->getNode(inSock->node->nodeId);
	auto inSockModel = inNode->inSockets[inSock->socketIndex];

	return outSockModel->canConvertTo(inSockModel);
}

void GraphNodeScene::emitGraphInvalidated()
{
	emit graphInvalidated();
}

void GraphNodeScene::dropEvent(QGraphicsSceneDragDropEvent * event)
{

	if (!event->mimeData()->data("index").isNull()) {
		event->accept();
		auto prop = nodeGraph->properties.at(event->mimeData()->data("index").toInt());
		if (prop) {
			auto propNode = new PropertyNode();
			propNode->setProperty(prop);
			propNode->setX(event->scenePos().x());
			propNode->setY(event->scenePos().y());
			this->addNodeModel(propNode);
			auto nodeView = this->getNodeById(propNode->id);
			nodeView->setPos(event->scenePos().x() - nodeView->boundingRect().width() / 2.0, event->scenePos().y() - nodeView->boundingRect().height() / 4.0);
		}
	}

	if ("node" == event->mimeData()->data("MODEL_TYPE_ROLE").toStdString()) {
		event->accept();

		auto node = nodeGraph->library->createNode(event->mimeData()->html());
			if (node) {
				node->setX(event->scenePos().x());
				node->setY(event->scenePos().y());
				this->addNodeModel(node);
				auto nodeView = this->getNodeById(node->id);
				nodeView->setPos(event->scenePos().x() - nodeView->boundingRect().width() / 2.0, event->scenePos().y() - nodeView->boundingRect().height() / 4.0);
			}
	}

	if (QVariant(event->mimeData()->data("MODEL_TYPE_ROLE")).toInt() == static_cast<int>(ModelTypes::Shader) ) {
		event->accept();

		QListWidgetItem *item = new QListWidgetItem;

		item->setData(Qt::DisplayRole, event->mimeData()->text());
		item->setData(MODEL_GUID_ROLE, event->mimeData()->data("MODEL_GUID_ROLE"));
		currentlyEditing = item;

		emit loadGraph(item);
		return;
	}

	if (event->mimeData()->data("MODEL_TYPE_ROLE").toStdString() == "presets") {
		emit loadGraphFromPreset(event->mimeData()->text());
	}

	if (event->mimeData()->data("MODEL_TYPE_ROLE").toStdString() == "presets2") {
		qDebug() << "preset 2";
		emit loadGraphFromPreset2(event->mimeData()->text());
	}
}

void GraphNodeScene::drawBackground(QPainter * painter, const QRectF & rect)
{
	//does not draw background
}

GraphNodeScene::GraphNodeScene(QWidget* parent) :
	QGraphicsScene(parent)
{
	nodeGraph = nullptr;
	con = nullptr;
	this->installEventFilter(this);
	conGroup = new QGraphicsItemGroup;
	addItem(conGroup);
	stack = new QUndoStack;
	
	selectedNode = nullptr;
}

void GraphNodeScene::undo()
{
	stack->undo();
}

void GraphNodeScene::redo()
{
	stack->redo();
}


SocketConnection *GraphNodeScene::addConnection(QString leftNodeId, int leftSockIndex, QString rightNodeId, int rightSockIndex)
{
	auto leftNode = this->getNodeById(leftNodeId);
	auto rightNode = this->getNodeById(rightNodeId);

	Q_ASSERT(leftNode != nullptr);
	Q_ASSERT(rightNode != nullptr);

	auto con = new SocketConnection();
	con->socket1 = leftNode->getOutSocket(leftSockIndex);
	con->socket2 = rightNode->getInSocket(rightSockIndex);
	con->socket1->addConnection(con);
	con->socket2->addConnection(con);
	con->updatePosFromSockets();
	con->updatePath();
	this->addItem(con);
	emit graphInvalidated();

	return con;
}

SocketConnection * GraphNodeScene::removeConnection(SocketConnection * connection, bool removeFromNodeGraph, bool emitSignal)
{
	auto socket1 = connection->socket1;
	auto socket2 = connection->socket2;
	socket1->removeConnection(connection);
	socket2->removeConnection(connection);
	this->removeItem(connection);

	if (removeFromNodeGraph)
		this->nodeGraph->removeConnection(connection->connectionId);

	if (emitSignal)
		emit connectionRemoved(connection);

	connection->updatePath();
	emit graphInvalidated();

	return connection;
}

void GraphNodeScene::removeConnection(const QString& conId, bool removeFromNodeGraph, bool emitSignal)
{
	auto con = getConnection(conId);
	removeConnection(con, removeFromNodeGraph, emitSignal);
}

void GraphNodeScene::setUndoRedoStack(QUndoStack *stack)
{
	this->stack = stack;
}

bool GraphNodeScene::eventFilter(QObject *o, QEvent *e)
{
	QGraphicsSceneMouseEvent *me = (QGraphicsSceneMouseEvent*)e;

	switch ((int)e->type())
	{
	case QEvent::GraphicsSceneMousePress:
	{
		auto sock = getSocketAt(me->scenePos().x(), me->scenePos().y());
		if (sock != nullptr) {
			if (me->button() == Qt::LeftButton) {

				// if it's an insocket with a connection then we're removing the connection
				if (sock->socketType == SocketType::In && sock->connections.size() == 1) {
					// insockets only have one connection
					con = sock->connections[0];

					// remove connection from nodegraph model
					this->nodeGraph->removeConnection(con->connectionId);

					// should be socket2 of the connection
					con->socket1->removeConnection(con);
					con->socket2->removeConnection(con);

					// emit event before modifying the nodes
					emit connectionRemoved(con);

					con->socket2 = nullptr;
					con->status = SocketConnectionStatus::Started;

					con->pos2 = me->scenePos();
					con->updatePath();
					//socketConnections.removeOne(con);

					views().at(0)->setDragMode(QGraphicsView::NoDrag);

					emit graphInvalidated();

				}
				else {
					con = new SocketConnection();
					con->socket1 = sock;
					con->pos1 = me->scenePos();
					con->pos2 = me->scenePos();
					con->status = SocketConnectionStatus::Started;
					con->updatePath();
					//conGroup->addToGroup(con);
					this->addItem(con);
					views().at(0)->setDragMode(QGraphicsView::NoDrag);
				}

			}
			if (me->button() == Qt::RightButton) {
                auto x = me->scenePos().x();
                auto y = me->scenePos().y();
                auto menu = removeConnectionContextMenu(x, y);
                auto view = this->views().first();
                auto scenePoint = view->mapFromScene(me->scenePos());
                auto p = view->viewport()->mapToGlobal(scenePoint);

                menu->exec(p);
			}

			return true;
		}
		else if (me->button() == Qt::RightButton)
		{
			auto view = this->views().first();
			auto scenePoint = view->mapFromScene(me->scenePos());
			auto p = view->viewport()->mapToGlobal(scenePoint);

//			menu->exec(p);

            auto dialog = new SearchDialog(this->nodeGraph, this, p);
            dialog->exec();
		}
		else if (me->button() == Qt::MiddleButton)
		{
			views().at(0)->setDragMode(QGraphicsView::NoDrag);
		}


		auto nodes = this->selectedItems();
		for (auto node : nodes) {
			auto nod = static_cast<GraphNode*> (node);
			nod->initialPoint = nod->pos();
		}
	}
	break;
	case QEvent::GraphicsSceneMouseMove:
	{
		if (con) {
			con->pos2 = me->scenePos();
			con->updatePath();

			auto sock = getSocketAt(me->scenePos().x(), me->scenePos().y());
			if (sock != nullptr && con->socket1 != sock) {
				//"connection entered";
			}
			return true;
		}
	}
	break;
	case QEvent::GraphicsSceneMouseRelease:
	{
		views().at(0)->setDragMode(QGraphicsView::RubberBandDrag);

		auto nodes = this->selectedItems();
		QList<GraphNode*> list;
		for ( auto node : nodes){
			auto nod = static_cast<GraphNode*> (node);
			nod->movedPoint = nod->pos();
			//if the distance between the old point and the new point is greater than 50, then add move command to the stack
			if (qSqrt(qPow(nod->initialPoint.x() - nod->movedPoint.x(), 2) + qPow(nod->initialPoint.y() - nod->movedPoint.y(), 2)) > 50) {
				list.append(nod);
			}
		}
		if (list.length() > 0) {
			auto moveCommand = new MoveMultipleCommand(list, this);
			stack->push(moveCommand);
		}


		if (con) {
			// make it an official connection
			auto sock = getSocketAt(me->scenePos().x(), me->scenePos().y());

			// from here on out the direction of the socket is important
			Socket* outSock;
			Socket* inSock;

			determineOutAndInSockets(con->socket1, sock, &outSock, &inSock);
			if (canSocketConnect(con->socket1, sock)) {

				con->socket2 = sock;
				con->status = SocketConnectionStatus::Finished;
				con->socket1->addConnection(con);
				con->socket2->addConnection(con);
				con->updatePosFromSockets();
				con->updatePath();

				// connect models too
				if (nodeGraph != nullptr) {
					AddConnectionCommand *addConnectionCommand;
					addConnectionCommand = new AddConnectionCommand(outSock->node->nodeId, inSock->node->nodeId, this, outSock->socketIndex, inSock->socketIndex);
					con->connectionId = addConnectionCommand->connectionID;
					stack->push(addConnectionCommand);
				}

				this->removeConnection(con->connectionId, false, false);

				emit newConnection(con);
				con = nullptr;
				emit graphInvalidated();

				return true;
			}

			this->removeItem(con);
			delete con;
			con = nullptr;

			return true;
		}
	}
	break;

	case QEvent::GraphicsSceneDrop: {
		auto event = (QDropEvent*)e;
		event->acceptProposedAction();
	}
	break;

	}

	return QObject::eventFilter(o, e);
}

bool GraphNodeScene::canSocketConnect(Socket* outSock, Socket* inSock)
{
	if (inSock == nullptr)
		return false;

	// ensure only outs can connect to ins
	if (outSock->socketType == inSock->socketType)
		return false;

	// prevent it connecting to the same node
	if (outSock->node == inSock->node)
		return false;

	// ensure they're compatible (controlled by the model)
	if (!areSocketsComptible(outSock, inSock))
		return false;

	// prevent recursive connection
	if (willConnectionBeALoop(outSock, inSock))
		return false;

	// prevent in sockets from having multiple connections
	if (inSock->connections.count() != 0)
		return false;

	return true;
}

bool GraphNodeScene::willConnectionBeALoop(Socket* sock1, Socket* sock2)
{
	Socket* leftSock;
	Socket* rightSock;
	/*
	if (sock1->socketType == SocketType::Out) {
		leftSock = sock1;
		rightSock = sock2;
	}
	else {
		leftSock = sock2;
		rightSock = sock1;
	}
	*/
	determineOutAndInSockets(sock1, sock2, &leftSock, &rightSock);

	// recursively gather nodes from left side of tree
	QSet<QString> nodeIds;
	std::function<void(GraphNode* node)> gatherNodes;
	gatherNodes = [&gatherNodes, &nodeIds](GraphNode* node)
	{
		nodeIds.insert(node->nodeId);
		for (int i = 0; i < node->getInSocketCount(); i++) {
			auto sock = node->getInSocket(i);

			// find connecting node and recurse
			for (auto con : sock->connections) {
				gatherNodes(con->getOutSocket()->node);
			}
		}
	};

	gatherNodes(leftSock->node);

	if (nodeIds.contains(rightSock->node->nodeId))
		return true;
	return false;
}

void GraphNodeScene::determineOutAndInSockets(Socket* sock1, Socket* sock2, Socket** outSock, Socket** inSock)
{
	Socket* leftSock;
	Socket* rightSock;
	if (sock1->socketType == SocketType::Out) {
		*outSock = sock1;
		*inSock = sock2;
	}
	else {
		*outSock = sock2;
		*inSock = sock1;
	}
}

Socket* GraphNodeScene::getSocketAt(float x, float y)
{
	auto items = this->items(QPointF(x, y));
	//auto items = this->items();
	for (auto item : items) {
		if (item && item->type() == (int)GraphicsItemType::Socket)
			return (Socket*)item;
	}

	return nullptr;
}

SocketConnection* GraphNodeScene::getConnectionAt(float x, float y)
{
	auto items = this->items(QPointF(x, y));
	//auto items = this->items();
	for (auto item : items) {
		if (item && item->type() == (int)GraphicsItemType::Connection)
			return (SocketConnection*)item;
	}

	return nullptr;
}

SocketConnection* GraphNodeScene::getConnection(const QString& conId)
{
	auto items = this->items();
	for (auto item : items) {
		if (item && item->type() == (int)GraphicsItemType::Connection)
			if (((SocketConnection*)item)->connectionId == conId)
				return (SocketConnection*)item;
	}

	return nullptr;
}

GraphNode *GraphNodeScene::getNodeById(QString id)
{
	auto items = this->items();
	for (auto item : items) {
		if (item && item->type() == (int)GraphicsItemType::Node)
			if (((GraphNode*)item)->nodeId == id)
				return (GraphNode*)item;
	}

	return nullptr;
}

QVector<GraphNode*> GraphNodeScene::getNodes()
{
	QVector<GraphNode*> nodes;
	auto items = this->items();
	for (auto item : items) {
		if (item && item->type() == (int)GraphicsItemType::Node)
				nodes.append((GraphNode*)item);
	}

	return nodes;
}

GraphNode *GraphNodeScene::getNodeByPos(QPointF point)
{
	auto items = this->items();
	//auto items = this->items();
	for (auto item : items) {
		if (item && item->boundingRect().contains(point)) {
			return (GraphNode*)item;
		}
	}

	return nullptr;
}