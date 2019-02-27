/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

#include "scene.h"
#include "../../dreamcaster.h"

#include <io/iAFileUtils.h>

#include <algorithm>
#include <cassert>
#include <vector>

extern DreamCaster * dcast;
//namespace Raytracer{

/**	\class BSPNode.
	\brief Class representing a BSP-tree node.

	AABB specified BSP-tree node.	
*/
class BSPNode
{
public:
	BSPNode()
	{
		internal1=0; 
		internal2=0; 
		masked_vars=0;
	}
	~BSPNode()
	{
	}
	/**
	* Splits current node's AABB by maximum dimension.
	* Creates two child-nodes with derived AABBs.
	* Stops if maximum level is reached.
	* @param level level of current node.
	* @param max_level nodes with max_level are leafs and therefore not divided.
	* @param l_aabb [out]left child's aabb
	* @param r_aabb [out]right child's aabb
	* @param nodes vector of tree nodes
	* @return 1 if successful, 0 otherwise
	*/
	int Split(aabb &p_aabb, aabb &l_aabb, aabb &r_aabb)
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
	/**
	* Splits current node's AABB. Creates two child-nodes with derived AABBs and distributes primitives from 
	* current node primitives vector among two child-nodes.
	* Recursively called for child-nodes. 
	* Stops if maximum level is reached.
	* @note If current node does not have any primitives recursion is stopped.
	* @param level level of current node.
	* @param max_level nodes with max_level are leafs.
	* @param m_aabb node's aabb
	* @param nodes vector of all tree nodes
	* @param tri_ind vector of index-triangle mapping array
	* @param prims vector of all tree primitives
	* @return 1 if successful
	*/
	int DistributePrims(int &level, int &max_level, aabb &m_aabb, std::vector<BSPNode*> &nodes,
						std::vector<unsigned int> &tri_ind, /*std::vector<Primitive*> &prims,*/ 
						std::vector<TriPrim*> &parent_tris, unsigned int tri_start_ind)
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
		aabb l_aabb, r_aabb;
		Split(m_aabb, l_aabb, r_aabb);
		set_offset( (unsigned int) nodes.size() );
		nodes.push_back(new BSPNode());
		set_has_left(true);
		nodes.push_back(new BSPNode());
		set_has_right(true);

		iAVec3f center = m_aabb.center(), h_size = m_aabb.half_size();
		unsigned int l_tri_start_ind = (unsigned int) tri_ind.size();//get_left(nodes)->set_tri_start(tri_ind.size());//left node
		//unsigned int counter=0;
		std::vector<TriPrim*> l_tris, r_tris;
		for (unsigned int i=0; i<trisSz; i++)
		{
			if(parent_tris[i]->Intersect( l_aabb, center, h_size))
			{
				//tri_ind.push_back(parent_prims[i]->GetIndex());
				l_tris.push_back(parent_tris[i]);
				//counter++;
			}
		}
		//get_left(nodes)->set_tri_count(counter);

		unsigned int r_tri_start_ind = (unsigned int) tri_ind.size();//get_right(nodes)->set_tri_start(tri_ind.size());
		//counter=0;
		//center = r_aabb.center(); h_size = r_aabb.half_size();
		//center = r_aabb.center(); h_size = r_aabb.half_size();
		for (unsigned int i=0; i<trisSz; i++)
		{
			if(parent_tris[i]->Intersect( r_aabb, center, h_size))
			{
				//tri_ind.push_back(parent_prims[i]->GetIndex());
				r_tris.push_back(parent_tris[i]);
				//counter++;
			}	
		}
		//get_right(nodes)->set_tri_count(counter);//left node
		int nxtLvl = level+1;

		///order matters, else if left before right will try to access empty element 
		if(r_tris.size()==0)//if(get_right(nodes)->tri_count()==0)
		{
			delete get_right(nodes);
			nodes.erase(nodes.begin()+offset()+1);
			set_has_right(false);
		}
		if(l_tris.size()==0)//if(get_left(nodes)->tri_count()==0)
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
	int SplitSAH(aabb &p_aabb, aabb &l_aabb, aabb &r_aabb, unsigned int axis_index, float bound)
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
	
	int DistributePrimsSAH(int &level, int &max_level, aabb &m_aabb, std::vector<BSPNode*> &nodes,
		std::vector<unsigned int> &tri_ind, /*std::vector<Primitive*> &prims,*/ 
		std::vector<TriPrim*> &parent_tris, unsigned int tri_start_ind)
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
		unsigned int axis_ind, cur_axis_ind, is_maximum;
		unsigned int l_counter, r_counter;
		unsigned int l_count, r_count;
		float bound, cur_bound;
		aabb l_aabb, r_aabb;
		for (unsigned int i=0; i<primSz; i++)
		{
			for (cur_axis_ind=0; cur_axis_ind<=2; cur_axis_ind++)
			{
				for (is_maximum=0; is_maximum<=1; is_maximum++)
				{
					l_counter=0; r_counter=0;
					cur_bound = ((TriPrim*)parent_tris[i])->getAxisBound(cur_axis_ind, is_maximum);
					SplitSAH(m_aabb, l_aabb, r_aabb, cur_axis_ind, cur_bound );
					iAVec3f l_center = l_aabb.center(), l_h_size = l_aabb.half_size();
					iAVec3f r_center = r_aabb.center(), r_h_size = r_aabb.half_size();
					for (unsigned int i2=0; i2<primSz; i2++)
					{
						//if( ((TriPrim*)parent_tris[i])->CenterInside( l_aabb) )
						if( ((TriPrim*)parent_tris[i2])->Intersect( l_aabb, l_center, l_h_size))
							l_counter++;
						//if( ((TriPrim*)parent_tris[i])->CenterInside( r_aabb) )
						if( ((TriPrim*)parent_tris[i2])->Intersect( r_aabb, r_center, r_h_size))
							r_counter++;
					}
					cur_cost = 0.5f + l_aabb.surfaceArea()*l_counter + r_aabb.surfaceArea()*r_counter;
					if(cur_cost < minCost)
					{
						minCost = cur_cost;
						bound = cur_bound;
						axis_ind = cur_axis_ind;
						l_count = l_counter;
						r_count = r_counter;
					}
				}
			}
		}
		/*switch(axis_ind)
		{
		case 0:
			if(bound<m_aabb.x1) bound = m_aabb.x1;
			if(bound>m_aabb.x2) bound = m_aabb.x2;
			break;
		case 1:
			if(bound<m_aabb.y1) bound = m_aabb.y1;
			if(bound>m_aabb.y2) bound = m_aabb.y2;
		    break;
		case 2:
			if(bound<m_aabb.z1) bound = m_aabb.z1;
			if(bound>m_aabb.z2) bound = m_aabb.z2;
		    break;
		}*/
		this->setAxisInd(axis_ind);
		this->set_splitCoord(bound);
		//Split(m_aabb, l_aabb, r_aabb);
		SplitSAH(m_aabb, l_aabb, r_aabb, axis_ind, bound);
		set_offset( (unsigned int) nodes.size() );
		nodes.push_back(new BSPNode());
		set_has_left(true);
		nodes.push_back(new BSPNode());
		set_has_right(true);

		unsigned int l_tri_start_ind = (unsigned int) tri_ind.size();//get_left(nodes)->set_tri_start(tri_ind.size());//left node
		unsigned int r_tri_start_ind = (unsigned int) tri_ind.size();//get_right(nodes)->set_tri_start(tri_ind.size());
		
		std::vector<TriPrim*> l_tris, r_tris;
		int cntr;
		iAVec3f center = m_aabb.center(), h_size = m_aabb.half_size();
		for (unsigned int i=0; i<primSz; i++)
		{
			cntr=0;
			if(parent_tris[i]->Intersect( l_aabb, center, h_size))
			{
				l_tris.push_back(parent_tris[i]);
				cntr++;
			}
			if(parent_tris[i]->Intersect( r_aabb, center, h_size))
			{
				r_tris.push_back(parent_tris[i]);
				cntr++;
			}
		}				
		//get_right(nodes)->set_tri_count(counter);//left node
		int nxtLvl = level+1;

		///order matters, else if left before right will try to access empty element 
		if(r_tris.size()==0)//if(get_right(nodes)->tri_count()==0)
		{
			delete get_right(nodes);
			nodes.erase(nodes.begin()+offset()+1);
			set_has_right(false);
		}
		if(l_tris.size()==0)//if(get_left(nodes)->tri_count()==0)
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
	void set_has_left (bool has){
		masked_vars&=0xbf;//off
		if(has) masked_vars|=0x40;//on
	}
	void set_has_right(bool has){
		masked_vars&=0xdf;//off
		if(has) masked_vars|=0x20;//on
	}
	//
	unsigned int internal1, internal2;///< shared data, depends if node is leaf or not
	unsigned int masked_vars; ///< Is this node a leaf-node first bit -- is leaf, has left, has right, else -- axis index
	inline unsigned int tri_start() {return internal1;}
	inline unsigned int tri_count() {return internal2;}
	inline unsigned int offset()    {return internal1;}
	inline float & splitCoord()       {return *((float*)&internal2);}
	inline void set_tri_start(unsigned int val) { internal1=val; }
	inline void set_tri_count(unsigned int val) { internal2=val; }
	inline void set_offset(unsigned int val)    { internal1=val; }
	inline void set_splitCoord(float val)       { internal2=*((unsigned int*)&val); }
	inline BSPNode *get_left(std::vector<BSPNode*> &nodes)  { return nodes[offset()]; }
	inline BSPNode *get_right(std::vector<BSPNode*> &nodes) { return nodes[offset()+1]; }
	inline BSPNode *get_left(const std::vector<BSPNode*> &nodes)  { return nodes[offset()]; }
	inline BSPNode *get_right(const std::vector<BSPNode*> &nodes) { return nodes[offset()+1]; }
};

struct traverse_stack
{
	struct trace_t {
		trace_t() {}
		trace_t(unsigned int a_node, float a_tmin, float a_tmax): node(a_node), tmin(a_tmin), tmax(a_tmax) {}
		unsigned int node;
		float tmin;
		float tmax;
	};
	traverse_stack(int a_size)
	{
		size = a_size;
		t = new trace_t[size];
		index=0;
	}
	~traverse_stack()
	{
		if(t)
		{
			delete [] t;
			t=0;
		}
	}
	void push(trace_t& node)
	{
		assert(index<size);
		t[index] = node;
		index++;
	}
	trace_t& get() const
	{
		assert((index-1)<size);
		assert(index>0);
		return t[index-1];
	}
	trace_t& pop()
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
	trace_t * t;
};
/**	\class BSPTree.
\brief Class representing a BSP-tree

Assigned with root node, level and AABB.	
*/
class BSPTree
{
public:
	BSPTree()
	{
		root = 0;
	}
	~BSPTree()
	{
		for (unsigned int i=0; i<nodes.size(); i++)
		{
			delete nodes[i];
		}
	}
	/**
	* Assigning split level and AABB to tree.
	* @note only root node is created here, child nodes are not defined yet. Nodes are splited in FillTree function
	* @see FillTree()
	* @param a_splitLevel split level of tree.
	* @param a_aabb AABB of tree.
	* @return 1
	*/
	int BuildTree(int a_splitLevel, aabb& a_aabb)
	{
		dcast->log("Building BSP-tree("+QString::number(a_splitLevel)+")................");
		m_aabb.setData(a_aabb);
		splitLevel=a_splitLevel;
		root = new BSPNode();
		nodes.push_back(root);
		//root->Split(0, splitLevel-1);
		dcast->log("done",true);
		return 1;
	}
	/**
	* Fills empty tree with primitives.
	* @note new nodes are created and divided here
	* @param prims primitives.
	* @return 1
	*/
	int FillTree(std::vector<TriPrim*>& triangles)
	{
		m_triangles = &triangles;
		dcast->log("Fill BSP-tree with data..........");
		int int_null = 0;
		if(dcast->stngs.USE_SAH != 0)
			root->DistributePrimsSAH(int_null, splitLevel, m_aabb, nodes, tri_ind, /*prims,*/ triangles,0);
		else
			root->DistributePrims(int_null, splitLevel, m_aabb, nodes, tri_ind, /*prims,*/ triangles,0);
		dcast->log("done",true);
		return 1;
	}
	/**
	* Fills already created tree with primitives.
	* @param a descr.
	* @param prims primitives.
	* @return 1
	*/
	int FillLoadedTree(std::vector<TriPrim*>& triangles)
	{
		m_triangles = &triangles;
		dcast->log("Fill BSP-tree with data..........");
		dcast->log("done\n",true);
		return 1;
	}
	/**
	* Finds all intersections between ray and primitives of tree.
	* @note non recursive (stack based) tree traversal version
	* @note rd = 100000.f*ray.GetDirection()+ro;
	* @param ray ray instance.
	* @param[out] intersections vector where obtained intersections are placed.
	* @return 1 if intersect tree AABB , 0 - otherwise
	*/
	int GetIntersectionsNR(Ray & ray, std::vector<intersection*>& intersections, traverse_stack * tr_stack) const
	{
		iAVec3f ro, rd;
		float tmin=0, tmax=100000.f, t=tmin;
		if(!IntersectAABB(ray, m_aabb, tmin, tmax)) return 0;
		traverse_stack::trace_t cur_t;
		cur_t = traverse_stack::trace_t(0,tmin,tmax);
		tr_stack->push(cur_t);
		BSPNode * cur_node;
		//int intersects;
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
					//int res;
					float a_Dist = 1000000.0f;
					if ((*m_triangles)[tri_ind[cur_node->tri_start()+i]]->Intersect( ray, a_Dist )) 
					{
						//iAVec3 isec = ray.GetOrigin()+ray.GetDirection()*a_Dist;			
						intersections.push_back(new intersection((*m_triangles)[tri_ind[cur_node->tri_start()+i]], a_Dist));//checked
					}
				}
			}
			//cur_node->GetIntersectionsNR(ray, ro, rd, intersections);
			else switch(GetIntersectionState(ray, tmin, tmax, cur_node->splitCoord(), cur_node->axisInd(),t))
			{
			case 1://right only
				if(cur_node->has_right())
				{
					traverse_stack::trace_t newStackElem(cur_node->offset()+1, tmin,tmax);
					tr_stack->push(newStackElem);
				}
				break;
			case 2://both
				sign = ray.GetDirection()[cur_node->axisInd()]>=0.0f;
				if(cur_node->has_right())
				{
					if(sign)
					{
						traverse_stack::trace_t newStackElem(cur_node->offset()+1, t, tmax);
						tr_stack->push(newStackElem);
					}
					else
					{
						traverse_stack::trace_t newStackElem(cur_node->offset()+1, tmin, t);
						tr_stack->push(newStackElem);
					}
				}
				if(cur_node->has_left())
				{
					if(sign)
					{
						traverse_stack::trace_t newStackElem(cur_node->offset(), tmin, t);
						tr_stack->push(newStackElem);
					}
					else
					{
						traverse_stack::trace_t newStackElem(cur_node->offset(), t, tmax);
						tr_stack->push(newStackElem);
					}
				}
			    break;
			case 0://left only
				if(cur_node->has_left())
				{
					traverse_stack::trace_t newStackElem(cur_node->offset(), tmin, tmax);
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
	/**
	* Saves tree in file specified by filename.
	* @note tree in file [splitLevel][aabb][num nodes][n0...nN][num tri inds][ti1...tiN]
	* @param filename filename of ouput file
	* @return 1 if succed , 0 - otherwise
	*/
	int SaveTree(QString const & filename)
	{
		FILE *fptr;
		//fopen_s( &fptr, filename, "wb" );
		fptr = fopen( getLocalEncodingFileName(filename).c_str(), "wb" );
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
	/**
	* Loads tree from file specified by filename.
	* @note tree in file [splitLevel][aabb][num nodes][n0...nN][num tri inds][ti1...tiN]
	* @param filename filename of input file
	* @return 1 if succed , 0 - otherwise
	*/
	int LoadTree(QString const & filename)
	{
		FILE *fptr = fopen( getLocalEncodingFileName(filename).c_str(),"rb");
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
			nodes.push_back(new BSPNode());
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
	BSPNode *root;	///< root node
	int splitLevel;	///< tree split level
	aabb m_aabb;	///< tree AABB
	std::vector<unsigned int> tri_ind;
	std::vector<BSPNode*> nodes;
protected:
	std::vector<TriPrim*>* m_triangles;
};
//};//raytracer
