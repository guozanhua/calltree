/*
 *
 * Copyright (C) 2009, Joacim Jacobsson ( j dot jacobsson at gmail dot com )
 * All rights reserved.
 *
 * See LICENSE file for details
 *
 */

#ifndef NODETONODEARROW_H_INCLUDED
#define NODETONODEARROW_H_INCLUDED

#include <QtGui/QGraphicsLineItem>

#include "GraphicsItemTypes.h"

// Qt Forward declares
class QGraphicsPolygonItem;
class QGraphicsLineItem;
class QGraphicsScene;
class QRectF;
class QGraphicsSceneMouseEvent;
class QPainterPath;

// BT Forward declares
class BehaviorTreeNode;

class NodeToNodeArrow: public QGraphicsLineItem
{
public:
	enum
	{
		Type = NodeToNodeArrowType
	};

	NodeToNodeArrow(
		BehaviorTreeNode *start,
		BehaviorTreeNode *end,
		QGraphicsScene *scene = 0x0
	);

	int type() const
	{
		return Type;
	}

	BehaviorTreeNode *startItem() const
	{
		return m_Start;
	}

	BehaviorTreeNode *endItem() const
	{
		return m_End;
	}

	void setStartAndEnd( BehaviorTreeNode *start, BehaviorTreeNode *end );
	void setDashed( bool dashed );

public slots:

	void updatePosition();

protected:

	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
			QWidget *widget = 0);

private:
	BehaviorTreeNode *m_Start;
	BehaviorTreeNode *m_End;
};

#endif