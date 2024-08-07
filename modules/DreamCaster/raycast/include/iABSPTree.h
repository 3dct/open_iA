// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAScene.h"
#include "../../iADreamCaster.h"

#include <algorithm>
#include <cassert>
#include <vector>

extern iADreamCaster * dcast;

//! Class representing a BSP-tree node. AABB specified BSP-tree node.
class iABSPNode
{
public:
	//! Splits current node's AABB by maximum dimension. Creates two child-nodes with derived AABBs.
	//! @param[in]  p_aabb parent's aabb
	//! @param[out] l_aabb left child's aabb
	//! @param[out] r_aabb right child's aabb
	//! @return 1 if successful, 0 otherwise
	int Split(iAaabb const &p_aabb, iAaabb &l_aabb, iAaabb &r_aabb)
	{
		float bound;
		int mainDim = p_aabb.mainDim();
		this->setAxisInd(mainDim);
		switch(mainDim)
		{
		case 0://X
			bound = p_aabb.center().x();
			l_aabb.setData(p_aabb.x1, bound, p_aabb.y1, p_aabb.y2, p_aabb.z1, p_aabb.z2);
			r_aabb.setData(bound, p_aabb.x2, p_aabb.y1, p_aabb.y2, p_aabb.z1, p_aabb.z2);
			break;
		case 1://Y
			bound = p_aabb.center().y();
			l_aabb.setData(p_aabb.x1, p_aabb.x2, p_aabb.y1, bound, p_aabb.z1, p_aabb.z2);
			r_aabb.setData(p_aabb.x1, p_aabb.x2, bound, p_aabb.y2, p_aabb.z1, p_aabb.z2);
			break;
		case 2://Z
			bound = p_aabb.center().z();
			l_aabb.setData(p_aabb.x1, p_aabb.x2, p_aabb.y1, p_aabb.y2, p_aabb.z1, bound);
			r_aabb.setData(p_aabb.x1, p_aabb.x2, p_aabb.y1, p_aabb.y2, bound, p_aabb.z2);
			break;
		default:
			assert(false);
			return 0;
		    break;
		}
		set_splitCoord(bound);
		return 1;
	}
	//! Splits current node's AABB. Creates two child-nodes with derived AABBs and distributes primitives from
	//! current node primitives vector among two child-nodes. Recursively called for child-nodes. Stops if
	//! maximum level is reached. If current node does not have any primitives, recursion is stopped.
	//! @param level level of current node.
	//! @param max_level nodes with max_level are leafs.
	//! @param m_aabb node's aabb
	//! @param nodes vector of all tree nodes
	//! @param tri_ind vector of index-triangle mapping array
	//! @param parent_tris
	//! @param tri_start_ind
	//! @return 1 if successful
	int DistributePrims(int &level, int &max_level, iAaabb &m_aabb, std::vector<iABSPNode*> &nodes,
						std::vector<unsigned int> &tri_ind,
						std::vector<iATriPrim*> &parent_tris, unsigned int tri_start_ind)
	{
		unsigned int trisSz = (unsigned int) parent_tris.size();
		if (level == max_level)
			setLeaf(true);
		if(isLeaf())
		{
			set_tri_start(tri_start_ind);
			set_tri_count(trisSz);
			for (unsigned int i=0; i<trisSz; i++)
				tri_ind.push_back(parent_tris[i]->GetIndex());
			return 1;
		}
		if(trisSz<=dcast->stngs.MIN_TRI_PER_NODE)//if number of primitives in node small, node is leaf
		{
			setLeaf(true);
			set_tri_start(tri_start_ind);
			set_tri_count(trisSz);
			for (unsigned int i=0; i<trisSz; i++)
				tri_ind.push_back(parent_tris[i]->GetIndex());
			return 1;
		}
		iAaabb l_aabb, r_aabb;
		Split(m_aabb, l_aabb, r_aabb);
		set_offset( (unsigned int) nodes.size() );
		nodes.push_back(new iABSPNode());
		set_has_left(true);
		nodes.push_back(new iABSPNode());
		set_has_right(true);

		iAVec3f center = m_aabb.center(), h_size = m_aabb.half_size();
		unsigned int l_tri_start_ind = (unsigned int) tri_ind.size();
		std::vector<iATriPrim*> l_tris, r_tris;
		for (unsigned int i=0; i<trisSz; i++)
		{
			if(parent_tris[i]->Intersect( l_aabb, center, h_size))
			{
				l_tris.push_back(parent_tris[i]);
			}
		}
		unsigned int r_tri_start_ind = (unsigned int) tri_ind.size();
		for (unsigned int i=0; i<trisSz; i++)
		{
			if(parent_tris[i]->Intersect( r_aabb, center, h_size))
			{
				r_tris.push_back(parent_tris[i]);
			}
		}
		int nxtLvl = level+1;

		// order matters, else if left before right will try to access empty element
		if(r_tris.size()==0)
		{
			delete get_right(nodes);
			nodes.erase(nodes.begin()+offset()+1);
			set_has_right(false);
		}
		if(l_tris.size()==0)
		{
			delete get_left(nodes);
			nodes.erase(nodes.begin()+offset());
			set_has_left(false);
		}
		if(has_right() && !has_left())//trick, otherwise get_right gets wrong node
			set_offset(offset()-1);

		if(has_left())
			get_left(nodes)->DistributePrims(nxtLvl, max_level, l_aabb, nodes, tri_ind, /*prims,*/ l_tris, l_tri_start_ind);
		if(has_right())
			get_right(nodes)->DistributePrims(nxtLvl, max_level, r_aabb, nodes, tri_ind, /*prims,*/ r_tris, r_tri_start_ind);
		return 1;
	}

	//SAH_BEGIN////////////////////////////////////////////////////////////////////////
	int SplitSAH(iAaabb &p_aabb, iAaabb &l_aabb, iAaabb &r_aabb, unsigned int axis_index, float bound)
	{
		switch(axis_index)
		{
		case 0://X
			l_aabb.setData(p_aabb.x1, bound, p_aabb.y1, p_aabb.y2, p_aabb.z1, p_aabb.z2);
			r_aabb.setData(bound, p_aabb.x2, p_aabb.y1, p_aabb.y2, p_aabb.z1, p_aabb.z2);
			break;
		case 1://Y
			l_aabb.setData(p_aabb.x1, p_aabb.x2, p_aabb.y1, bound, p_aabb.z1, p_aabb.z2);
			r_aabb.setData(p_aabb.x1, p_aabb.x2, bound, p_aabb.y2, p_aabb.z1, p_aabb.z2);
			break;
		case 2://Z
			l_aabb.setData(p_aabb.x1, p_aabb.x2, p_aabb.y1, p_aabb.y2, p_aabb.z1, bound);
			r_aabb.setData(p_aabb.x1, p_aabb.x2, p_aabb.y1, p_aabb.y2, bound, p_aabb.z2);
			break;
		default:
			assert(false);
			return 0;
			break;
		}
		return 1;
	}

	int DistributePrimsSAH(int &level, int &max_level, iAaabb &m_aabb, std::vector<iABSPNode*> &nodes,
		std::vector<unsigned int> &tri_ind,
		std::vector<iATriPrim*> &parent_tris, unsigned int tri_start_ind)
	{
		unsigned int primSz = (unsigned int) parent_tris.size();
		if (level == max_level)
			setLeaf(true);
		if(isLeaf())
		{
			set_tri_start(tri_start_ind);
			set_tri_count(primSz);
			for (unsigned int i=0; i<primSz; i++)
				tri_ind.push_back(parent_tris[i]->GetIndex());
			return 1;
		}
		if(primSz<=dcast->stngs.MIN_TRI_PER_NODE)//if number of primitives in node small, node is leaf
		{
			setLeaf(true);
			set_tri_start(tri_start_ind);
			set_tri_count(primSz);
			for (unsigned int i=0; i<primSz; i++)
				tri_ind.push_back(parent_tris[i]->GetIndex());
			return 1;
		}
		//SAH bound determining
		float minCost = 10000000.f, cur_cost;
		unsigned int axis_ind=0, cur_axis_ind, is_maximum;
		unsigned int l_counter, r_counter;
		//unsigned int l_count=0, r_count=0;
		float bound=0, cur_bound;
		iAaabb l_aabb, r_aabb;
		for (unsigned int i=0; i<primSz; i++)
		{
			for (cur_axis_ind=0; cur_axis_ind<=2; cur_axis_ind++)
			{
				for (is_maximum=0; is_maximum<=1; is_maximum++)
				{
					l_counter=0; r_counter=0;
					cur_bound = ((iATriPrim*)parent_tris[i])->getAxisBound(cur_axis_ind, is_maximum);
					SplitSAH(m_aabb, l_aabb, r_aabb, cur_axis_ind, cur_bound );
					iAVec3f l_center = l_aabb.center(), l_h_size = l_aabb.half_size();
					iAVec3f r_center = r_aabb.center(), r_h_size = r_aabb.half_size();
					for (unsigned int i2=0; i2<primSz; i2++)
					{
						if( ((iATriPrim*)parent_tris[i2])->Intersect( l_aabb, l_center, l_h_size))
							l_counter++;
						if( ((iATriPrim*)parent_tris[i2])->Intersect( r_aabb, r_center, r_h_size))
							r_counter++;
					}
					cur_cost = 0.5f + l_aabb.surfaceArea()*l_counter + r_aabb.surfaceArea()*r_counter;
					if(cur_cost < minCost)
					{
						minCost = cur_cost;
						bound = cur_bound;
						axis_ind = cur_axis_ind;
						//l_count = l_counter;
						//r_count = r_counter;
					}
				}
			}
		}
		this->setAxisInd(axis_ind);
		this->set_splitCoord(bound);
		SplitSAH(m_aabb, l_aabb, r_aabb, axis_ind, bound);
		set_offset( (unsigned int) nodes.size() );
		nodes.push_back(new iABSPNode());
		set_has_left(true);
		nodes.push_back(new iABSPNode());
		set_has_right(true);

		unsigned int l_tri_start_ind = (unsigned int) tri_ind.size();
		unsigned int r_tri_start_ind = (unsigned int) tri_ind.size();

		std::vector<iATriPrim*> l_tris, r_tris;
		//int cntr;
		iAVec3f center = m_aabb.center(), h_size = m_aabb.half_size();
		for (unsigned int i=0; i<primSz; i++)
		{
			//cntr=0;
			if(parent_tris[i]->Intersect( l_aabb, center, h_size))
			{
				l_tris.push_back(parent_tris[i]);
				//cntr++;
			}
			if(parent_tris[i]->Intersect( r_aabb, center, h_size))
			{
				r_tris.push_back(parent_tris[i]);
				//cntr++;
			}
		}
		int nxtLvl = level+1;

		// order matters, else if left before right will try to access empty element
		if(r_tris.size()==0)
		{
			delete get_right(nodes);
			nodes.erase(nodes.begin()+offset()+1);
			set_has_right(false);
		}
		if(l_tris.size()==0)
		{
			delete get_left(nodes);
			nodes.erase(nodes.begin()+offset());
			set_has_left(false);
		}
		if(has_right() && !has_left())//trick, otherwise get_right gets wrong node
			set_offset(offset()-1);

		if(has_left())
			get_left(nodes)->DistributePrimsSAH(nxtLvl, max_level, l_aabb, nodes, tri_ind, /*prims,*/ l_tris, l_tri_start_ind);
		if(has_right())
			get_right(nodes)->DistributePrimsSAH(nxtLvl, max_level, r_aabb, nodes, tri_ind, /*prims,*/ r_tris, r_tri_start_ind);
		return 1;
	}
	//SAH_END////////////////////////////////////////////////////////////////////////
	//
	bool isLeaf() const {return masked_vars&0x80;}
	inline int axisInd() const {return masked_vars&0x03;}
	void setLeaf(bool a_isLeaf){
		masked_vars&=0x7f;//off
		if(a_isLeaf)masked_vars|=0x80;//on
	}
	inline void setAxisInd(int index)
	{
		switch(index)
		{
		case 0:
			masked_vars|=(char)0x00;
			break;
		case 1:
			masked_vars|=(char)0x01;
			break;
		case 2:
			masked_vars|=(char)0x02;
			break;
		default:
			break;
		}
	}
	bool has_left() const {return masked_vars&0x40;}
	bool has_right() const {return masked_vars&0x20;}
	void set_has_left (bool has)
	{
		masked_vars&=0xbf;//off
		if(has) masked_vars|=0x40;//on
	}
	void set_has_right(bool has)
	{
		masked_vars&=0xdf;//off
		if(has) masked_vars|=0x20;//on
	}
	union OffsetOrSplitCoord
	{
		unsigned int u;
		float f;
	};
	unsigned int internal1{};        //!< either tri_start or offset, depends if node is leaf or not
	OffsetOrSplitCoord internal2{};  //!< either tri_count or split coord, depends if node is leaf or not
	unsigned int masked_vars{};      //!< Is this node a leaf-node first bit -- is leaf, has left, has right, else -- axis index
	inline unsigned int tri_start() const       { return internal1; }
	inline unsigned int tri_count() const       { return internal2.u; }
	inline unsigned int offset()    const       { return internal1; }
	inline float & splitCoord()                 { return internal2.f; }
	inline void set_tri_start(unsigned int val) { internal1=val; }
	inline void set_tri_count(unsigned int val) { internal2.u=val; }
	inline void set_offset(unsigned int val)    { internal1=val; }
	inline void set_splitCoord(float val)       { internal2.f=val; }
	inline iABSPNode *get_left(std::vector<iABSPNode*> &nodes)  { return nodes[offset()]; }
	inline iABSPNode *get_right(std::vector<iABSPNode*> &nodes) { return nodes[offset()+1]; }
	inline iABSPNode *get_left(const std::vector<iABSPNode*> &nodes)  { return nodes[offset()]; }
	inline iABSPNode *get_right(const std::vector<iABSPNode*> &nodes) { return nodes[offset()+1]; }
};

struct iATraverseStack
{
	struct iATrace {
		iATrace() {}
		iATrace(unsigned int a_node, float a_tmin, float a_tmax): node(a_node), tmin(a_tmin), tmax(a_tmax) {}
		unsigned int node;
		float tmin;
		float tmax;
	};
	iATraverseStack(int a_size)
	{
		size = a_size;
		t = new iATrace[size];
		index=0;
	}
	~iATraverseStack()
	{
		if (t)
		{
			delete [] t;
			t = nullptr;
		}
	}
	void push(iATrace& node)
	{
		assert(index<size);
		t[index] = node;
		index++;
	}
	iATrace& get() const
	{
		assert((index-1)<size);
		assert(index>0);
		return t[index-1];
	}
	iATrace& pop()
	{
		index--;
		assert(index<size);
		assert(index>=0);
		return t[index];
	}
	int numElements()
	{
		return index;
	}
protected:
	int size;
	int index;
	iATrace * t;
};

//! Class representing a BSP-tree. Assigned with root node, level and AABB.
class iABSPTree
{
public:
	iABSPTree()
	{
		root = nullptr;
	}
	~iABSPTree()
	{
		for (unsigned int i=0; i<nodes.size(); i++)
		{
			delete nodes[i];
		}
	}
	//! Assigning split level and AABB to tree.
	//! @note only root node is created here, child nodes are not defined yet. Nodes are splited in FillTree function
	//! @see FillTree()
	//! @param a_splitLevel split level of tree.
	//! @param a_aabb AABB of tree.
	void BuildTree(int a_splitLevel, iAaabb& a_aabb)
	{
		dcast->log("Building BSP-tree("+QString::number(a_splitLevel)+")................");
		m_aabb.setData(a_aabb);
		splitLevel=a_splitLevel;
		root = new iABSPNode();
		nodes.push_back(root);
		dcast->log("done",true);

	}
	//! Fills empty tree with primitives. New nodes are created and divided here
	//! @param triangles primitives.
	void FillTree(std::vector<iATriPrim*>& triangles)
	{
		m_triangles = &triangles;
		dcast->log("Fill BSP-tree with data..........");
		int int_null = 0;
		if(dcast->stngs.USE_SAH != 0)
			root->DistributePrimsSAH(int_null, splitLevel, m_aabb, nodes, tri_ind, triangles,0);
		else
			root->DistributePrims(int_null, splitLevel, m_aabb, nodes, tri_ind, triangles,0);
		dcast->log("done",true);
	}
	//! Fills already created tree with primitives.
	//! @param triangles primitives.
	void FillLoadedTree(std::vector<iATriPrim*>& triangles)
	{
		m_triangles = &triangles;
		dcast->log("Fill BSP-tree with data..........");
		dcast->log("done\n",true);
	}

	//! Finds all intersections between ray and primitives of tree.
	//! @note non recursive (stack based) tree traversal version
	//! @note rd = 100000.f*ray.GetDirection()+ro;
	//! @param ray ray instance.
	//! @param[out] intersections vector where obtained intersections are placed.
	//! @param tr_stack
	//! @return 1 if intersect tree AABB , 0 - otherwise
	int GetIntersectionsNR(iARay & ray, std::vector<iAintersection*>& intersections, iATraverseStack * tr_stack) const
	{
		iAVec3f ro, rd;
		float tmin=0, tmax=100000.f, t=tmin;
		if(!IntersectAABB(ray, m_aabb, tmin, tmax)) return 0;
		iATraverseStack::iATrace cur_t;
		cur_t = iATraverseStack::iATrace(0,tmin,tmax);
		tr_stack->push(cur_t);
		iABSPNode * cur_node;
		unsigned int sign = 0;
		while (tr_stack->numElements()>0)
		{
			cur_t = tr_stack->pop();
			cur_node = nodes[cur_t.node];
			tmin=cur_t.tmin; tmax=cur_t.tmax;
			if(cur_node->isLeaf())
			{
				for (unsigned int i=0; i<cur_node->tri_count(); i++)
				{
					float a_Dist = 1000000.0f;
					if ((*m_triangles)[tri_ind[cur_node->tri_start()+i]]->Intersect( ray, a_Dist ))
					{
						intersections.push_back(new iAintersection((*m_triangles)[tri_ind[cur_node->tri_start()+i]], a_Dist));
					}
				}
			}
			else switch(GetIntersectionState(ray, tmin, tmax, cur_node->splitCoord(), cur_node->axisInd(),t))
			{
			case 1://right only
				if(cur_node->has_right())
				{
					iATraverseStack::iATrace newStackElem(cur_node->offset()+1, tmin,tmax);
					tr_stack->push(newStackElem);
				}
				break;
			case 2://both
				sign = ray.GetDirection()[cur_node->axisInd()]>=0.0f;
				if(cur_node->has_right())
				{
					if(sign)
					{
						iATraverseStack::iATrace newStackElem(cur_node->offset()+1, t, tmax);
						tr_stack->push(newStackElem);
					}
					else
					{
						iATraverseStack::iATrace newStackElem(cur_node->offset()+1, tmin, t);
						tr_stack->push(newStackElem);
					}
				}
				if(cur_node->has_left())
				{
					if(sign)
					{
						iATraverseStack::iATrace newStackElem(cur_node->offset(), tmin, t);
						tr_stack->push(newStackElem);
					}
					else
					{
						iATraverseStack::iATrace newStackElem(cur_node->offset(), t, tmax);
						tr_stack->push(newStackElem);
					}
				}
			    break;
			case 0://left only
				if(cur_node->has_left())
				{
					iATraverseStack::iATrace newStackElem(cur_node->offset(), tmin, tmax);
					tr_stack->push(newStackElem);
				}
				break;
			default:
				assert(false);
				break;
			}
		}
		return 1;
	}
	//! Saves tree in file specified by filename.
	//! @note tree in file [splitLevel][aabb][num nodes][n0...nN][num tri inds][ti1...tiN]
	//! @param filename filename of ouput file
	//! @return 1 if succed , 0 - otherwise
	int SaveTree(QString const & filename)
	{
		FILE *fptr;
		fptr = fopen( filename.toStdString().c_str(), "wb");
		if(!fptr)
		{
			dcast->log("failed(cannot open file)\n",true);
			return 0;
		}
		fwrite(&splitLevel, sizeof(splitLevel), 1, fptr);
		fwrite(&m_aabb.x1, sizeof(float), 1, fptr);
		fwrite(&m_aabb.x2, sizeof(float), 1, fptr);
		fwrite(&m_aabb.y1, sizeof(float), 1, fptr);
		fwrite(&m_aabb.y2, sizeof(float), 1, fptr);
		fwrite(&m_aabb.z1, sizeof(float), 1, fptr);
		fwrite(&m_aabb.z2, sizeof(float), 1, fptr);
		unsigned int a_size = (unsigned int) nodes.size();
		fwrite(&a_size, sizeof(unsigned int), 1, fptr);
		for(unsigned int i=0; i<a_size; i++)
		{
			fwrite(&nodes[i]->internal1, sizeof(unsigned int), 1, fptr);
			fwrite(&nodes[i]->internal2, sizeof(unsigned int), 1, fptr);
			fwrite(&nodes[i]->masked_vars, sizeof(unsigned int), 1, fptr);
		}
		a_size = (unsigned int) tri_ind.size();
		fwrite(&a_size, sizeof(unsigned int), 1, fptr);
		for(unsigned int i=0; i<a_size; i++)
		{
			fwrite(&tri_ind[i], sizeof(unsigned int), 1, fptr);
		}
		fclose(fptr);
		dcast->log("Tree saved under filename:"+QString(filename));
		return 1;
	}
	//! Loads tree from file specified by filename.
	//! @note tree in file [splitLevel][aabb][num nodes][n0...nN][num tri inds][ti1...tiN]
	//! @param filename filename of input file
	//! @return 1 if succed , 0 - otherwise
	int LoadTree(QString const & filename)
	{
		FILE *fptr = fopen( filename.toStdString().c_str(), "rb");
		if(!fptr)
		{
			dcast->log("failed to open file",true);
			return 0;
		}
		if (fread(&splitLevel, sizeof(splitLevel), 1, fptr) != 1 ||
			fread(&m_aabb.x1, sizeof(float), 1, fptr) != 1 ||
			fread(&m_aabb.x2, sizeof(float), 1, fptr) != 1 ||
			fread(&m_aabb.y1, sizeof(float), 1, fptr) != 1 ||
			fread(&m_aabb.y2, sizeof(float), 1, fptr) != 1 ||
			fread(&m_aabb.z1, sizeof(float), 1, fptr) != 1 ||
			fread(&m_aabb.z2, sizeof(float), 1, fptr) != 1)
		{
			fclose(fptr);
			dcast->log("failed to read file", true);
			return 0;
		}
		unsigned int a_size;
		if (fread(&a_size, sizeof(unsigned int), 1, fptr) != 1)
		{
			fclose(fptr);
			dcast->log("failed to read file", true);
			return 0;
		}
		for(unsigned int i=0; i<a_size; i++)
		{
			nodes.push_back(new iABSPNode());
			if (fread(&nodes[i]->internal1, sizeof(unsigned int), 1, fptr) != 1 ||
				fread(&nodes[i]->internal2, sizeof(unsigned int), 1, fptr) != 1 ||
				fread(&nodes[i]->masked_vars, sizeof(unsigned int), 1, fptr) != 1)
			{
				fclose(fptr);
				dcast->log("failed to read file", true);
				return 0;
			}
		}
		root=nodes[0];
		if (fread(&a_size, sizeof(unsigned int), 1, fptr) != 1)
		{
			fclose(fptr);
			dcast->log("failed to read file", true);
			return 0;
		}
		for(unsigned int i=0; i<a_size; i++)
		{
			tri_ind.push_back(0);
			if (fread(&tri_ind[i], sizeof(unsigned int), 1, fptr) != 1)
			{
				fclose(fptr);
				dcast->log("failed to read file", true);
				return 0;
			}
		}
		fclose(fptr);
		dcast->log("done\n",true);
		return 1;
	}
	iABSPNode *root;	//!< root node
	int splitLevel;	//!< tree split level
	iAaabb m_aabb;	//!< tree AABB
	std::vector<unsigned int> tri_ind;
	std::vector<iABSPNode*> nodes;
protected:
	std::vector<iATriPrim*>* m_triangles;
};
