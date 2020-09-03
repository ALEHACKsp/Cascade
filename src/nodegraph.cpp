#include "nodegraph.h"

#include <iostream>

#include <QMouseEvent>
#include <QScrollBar>
#include <QGraphicsProxyWidget>

#include "nodeinput.h"
#include "nodeoutput.h"

NodeGraph::NodeGraph(QWidget* parent)
    : QGraphicsView(parent)
{
    scene = new QGraphicsScene();
    this->setScene(scene);

    this->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    this->setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    this->setRenderHints(QPainter::Antialiasing |
                         QPainter::SmoothPixmapTransform);
    this->setSceneRect(0, 0, viewWidth, viewHeight);
    this->centerOn(viewWidth / 2, viewHeight / 2);

    contextMenu = new NodeGraphContextMenu(this);
}

void NodeGraph::createNode(const NodeType type)
{
    NodeBase* n = new NodeBase(type, this);
    scene->addWidget(n);
    n->move(mapToScene(lastMousePos).x(),
            mapToScene(lastMousePos).y());
    nodes.push_back(n);

    connect(n, &NodeBase::nodeWasLeftClicked,
                this, &NodeGraph::handleNodeLeftMouseClicked);
    connect(n, &NodeBase::nodeWasDoubleClicked,
            this, &NodeGraph::handleNodeMouseDoubleClicked);
}

float NodeGraph::getViewScale() const
{
    return viewScale;
}

void NodeGraph::showContextMenu(const QPoint &pos)
{
    contextMenu->exec(mapToGlobal(pos));
}

QGraphicsItem* NodeGraph::getObjectUnderCursor()
{
    auto item = scene->itemAt(mapToScene(mapFromGlobal(QCursor::pos())), QTransform());

    return item;
}

QWidget* NodeGraph::getWidgetFromGraphicsitem(QGraphicsItem *item)
{
    QGraphicsProxyWidget* pWidget = qgraphicsitem_cast<QGraphicsProxyWidget*>(item);

    if(pWidget)
    {
        return pWidget->widget();
    }
    return nullptr;
}

void NodeGraph::handleNodeLeftMouseClicked(NodeBase* node)
{
    selectedNode = node;
    foreach(NodeBase* n, nodes)
    {
        n->setIsSelected(false);
    }
    node->setIsSelected(true);
}

void NodeGraph::handleNodeMouseDoubleClicked(NodeBase* node)
{
    if (node)
    {
        activeNode = node;
        node->setIsActive(true);
        //m_properties->loadNodeProperties(node); // USE A SIGNAL
    }
    else
    {
        activeNode = nullptr;
        //m_properties->clear(); // USE A SIGNAL
    }
}

void NodeGraph::handleNodeOutputLeftMouseClicked(NodeOutput* nodeOut)
{
    leftMouseIsDragging = true;

    createOpenConnection(nodeOut);
}

void NodeGraph::createOpenConnection(NodeOutput* nodeOut)
{
    Connection* c = new Connection(nodeOut);
    openConnection = c;
    scene->addItem(openConnection);
}

void NodeGraph::destroyOpenConnection()
{
    if(openConnection)
    {
        scene->removeItem(openConnection);
        delete openConnection;
        openConnection = nullptr;
    }
}

void NodeGraph::establishConnection(NodeInput *nodeIn)
{
    nodeIn->addInConnection(openConnection);
    openConnection->targetInput = nodeIn;
    openConnection->sourceOutput->addConnection(openConnection);
    connections.push_back(openConnection);
    openConnection = nullptr;
}

void NodeGraph::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton)
    {
        showContextMenu(event->pos());
    }
    else if (event->button() == Qt::MiddleButton)
    {
        middleMouseIsDragging = true;
    }
    lastMousePos = event->pos();
    QGraphicsView::mousePressEvent(event);
}

void NodeGraph::mouseMoveEvent(QMouseEvent* event)
{
    if (middleMouseIsDragging)
    // Scroll the whole scene
    {
        auto t = event->pos() - lastMousePos;
        this->horizontalScrollBar()->setValue(this->horizontalScrollBar()->value() - t.x());
        this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() - t.y());
    }
    else if(leftMouseIsDragging)
    {
        if(openConnection)
        {
            openConnection->updatePosition(QPoint(
                                           mapToScene(mapFromGlobal(QCursor::pos())).x(),
                                           mapToScene(mapFromGlobal(QCursor::pos())).y()));
        }
    }
    lastMousePos = event->pos();

    QGraphicsView::mouseMoveEvent(event);
}

void NodeGraph::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        leftMouseIsDragging = false;

        auto item = getObjectUnderCursor();

        NodeBase* node = qobject_cast<NodeBase*>(getWidgetFromGraphicsitem(item));
        if(node)
        {
            auto nodeIn = node->getNodeInputAtPosition(event->screenPos().toPoint());
            if(nodeIn)
            // Open connection was released on a NodeInput
            {
                if(!nodeIn->hasConnection())
                {
                    establishConnection(nodeIn);
                    return;
                }
            }
        }
        destroyOpenConnection();
    }
    if (event->button() == Qt::MiddleButton)
    {
        middleMouseIsDragging = false;
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void NodeGraph::wheelEvent(QWheelEvent* event)
{
    QPoint scrollAmount = event->angleDelta();
    double factor = (scrollAmount.y() > 0) ? 1.2 : 1 / 1.2;
    this->scale(factor, factor);
    viewScale *= factor;
}
