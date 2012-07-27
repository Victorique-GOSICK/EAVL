// Copyright 2010-2012 UT-Battelle, LLC.  See LICENSE.txt for more information.
#ifndef EAVL_CELL_SET_ALL_EDGES_OF_EXPLICIT_H
#define EAVL_CELL_SET_ALL_EDGES_OF_EXPLICIT_H

#include "eavlCellComponents.h"
#include "eavlCellSetExplicit.h"
#include "eavlException.h"

// ****************************************************************************
// Class:  eavlCellSetAllEdgesOfExplicit
//
// Purpose:
///   A set of all edges on an explicit set of cells.
///   \todo: question: do we create edges for a 1D line mesh?
//
// Programmer:  Jeremy Meredith, Dave Pugmire, Sean Ahern
// Creation:    January  3, 2012
//
// ****************************************************************************

class eavlCellSetAllEdgesOfExplicit : public eavlCellSet
{
  protected:
    eavlCellSetExplicit *parent; ///<\todo: should this really be a pointer, or index, or what?
  public:
    virtual void PrintSummary(ostream &out)
    {
        out << "    eavlCellSetAllEdgesOfExplicit:\n";
        out << "        name = "<<name<<endl;
        out << "        dimensionality = "<<dimensionality<<endl;
        out << "        nCells = "<<GetNumCells()<<endl;
        out << "        parent = "<<parent<<endl;
    }
    eavlCellSetAllEdgesOfExplicit(eavlCellSetExplicit *p)
        : eavlCellSet(string("edges_of_")+p->GetName(), 1), parent(p)
    {
        if (!p)
            THROW(eavlException,"parent was null in eavlCellSetExplicit constructor")
        if (parent->GetNumCells() <= 0)
            THROW(eavlException,"parent had no cells in eavlCellSetExplicit constructor");
    }
    virtual int GetNumCells()
    {
        return parent->GetNumEdges();
    }
    virtual eavlCell GetCellNodes(int edgeindex)
    {
        const eavlExplicitConnectivity &conn = 
            parent->GetConnectivity(EAVL_NODES_OF_EDGES);
        eavlCell cell;
        cell.type = eavlCellShape(conn.GetElementComponents(edgeindex,
                                                            cell.numIndices,
                                                            cell.indices));
        return cell;
    }
};

#endif
