#define THREAD_W 1
#define THREAD_H 32

#define MAX_ISEC_COUNT 24

/* Texture to output resultats */
//texture<float, 1, cudaReadModeElementType> out_tex;

/**	\struct Vec3.
\brief Class representing 3 dimensional vector. CUDA version.
*/

/**	\struct ct_state.
	\brief Structure describe current CT state.
*/
struct ct_state{
	float4 o;///< rays origin
	float4 c;///< corner of plate
	float4 dx;///< dx of plane in 3d
	float4 dy;///< dy of plane in 3d
};

//**	The axis-aligned bounding box
struct aabb {
	float x1,x2,y1,y2,z1,z2;
};

int isInside(struct aabb * box, float4 * v)
{
	if((*v).x<=box->x2)
		if((*v).x>=box->x1) 
			if((*v).y<=box->y2)
				if((*v).y>=box->y1) 
					if((*v).z<=box->z2)
						if((*v).z>=box->z1)
							return 1;
	return 0;
}

/**	\struct BSPNode.
\brief Class representing a BSP-tree node.

BSP-tree node.	
*/


struct BSPNode
{
	union {
		unsigned int tri_start;
		unsigned int offset;
	};
	union {
		unsigned int tri_count;
		float splitCoord;
	};
	unsigned int masked_vars;
};

#define isLeaf(node)	(bool)( (node->masked_vars) & 0x00000080 )
#define has_left(node)	(bool)( (node->masked_vars) & 0x00000040 )
#define has_right(node)	(bool)( (node->masked_vars) & 0x00000020 )
#define axisInd(node)	(int) ( (node->masked_vars) & 0x00000003 )


/**	\struct traverse_stack.
	\brief Used for tree traversal.

	CUDA version.	
*/
struct trace_t 
{
	//trace_t(unsigned int aa, float a, float b):node(aa),tmin(a),tmax(b){}
	unsigned int node;
	float tmin;
	float tmax;
};

struct traverse_stack
{
	int index;
	struct trace_t t[30];
};

#define STACK_GET(stack) ( &( stack.t[ stack.index - 1 ] ) )
#define STACK_PUSH(stack, a_node, a_tmin, a_tmax)	{ stack.t[ stack.index ].node = a_node;	stack.t[ stack.index ].tmin = a_tmin; stack.t[ stack.index ].tmax = a_tmax;	stack.index++; }
#define STACK_POP(stack) stack.index--;

/**	\struct wald_tri.
\brief Structure representing a triangle data needed for intersection test.

Wald triangle structure.	
*/

struct wald_tri
{
	float N[3];
	float A[3];
	float nu, nv, nd;
	unsigned int k;
	float bnu, bnv, cnu, cnv;
};

#define wt_N			(float4)(wt->N[0], wt->N[1], wt->N[2], 0.0f)
#define wt_A			(float4)(wt->A[0], wt->A[1], wt->A[2], 0.0f)

/**	\class Ray.
\brief Class representing ray in 3D.

OpenCL version.	
*/
struct Ray
{
	float4 o;	///< ray origin's position
	float4 dir;	///< ray direction vector
};

/**	\struct intersection.
\brief Structure representing intersection data.

Contains data about primitive. OpenCL version	
*/
struct intersection
{
	unsigned int prim_ind;
	float dist;
};


/**
* Ray-AABB intersection routine. CUDA version.
* @param ray ray class.
* @param box axis aligned bounding box structure.
* @return 
1 - if ray intersects AABB
0 - otherwise
*/
static inline unsigned int IntersectRayAABB(struct Ray * ray, __global struct aabb * box, float tmin, float tmax)
{
	float l1 = (box->x1 - ray->o.x) / ray->dir.x;
	float l2 = (box->x2 - ray->o.x) / ray->dir.x;
	tmin = fmax(fmin(l1,l2), tmin);
	tmax = fmin(fmax(l1,l2), tmax);
	l1 = (box->y1 - ray->o.y) / ray->dir.y;
	l2 = (box->y2 - ray->o.y) / ray->dir.y;
	tmin = fmax(fmin(l1,l2), tmin);
	tmax = fmin(fmax(l1,l2), tmax);
	l1 = (box->z1 - ray->o.z) / ray->dir.z;
	l2 = (box->z2 - ray->o.z) / ray->dir.z;
	tmin = fmax(fmin(l1,l2), tmin);
	tmax = fmin(fmax(l1,l2), tmax);
	return ((tmax >= tmin) & (tmax >= 0.f));
}
/**
* Ray-AABB intersection routine.
* checks which subnodes' AABBs are intersected by ray. CUDA version.
* @param ray ray class.
* @param tmin parent AABB min t.
* @param tmin parent AABB max t.
* @param split split plane's coordinates
* @param splitIndex index of splitting axis
* @param t [out] split plane's t
* @return 
0 - left node intersected
1 - both nodes intersected
2 - right node intersected
*/
inline int GetIntersectionState(struct Ray * ray, 
								const float tmin, 
								const float tmax, 
								const float split, 
								const int splitIndex, 
								float * t)
{	
	float rd = ((float*)(&ray->dir))[splitIndex];
	float ro = ((float*)(&ray->o))[splitIndex];
	if(!rd)
		rd = 0.0000000001f;
	(*t) = (split - ro) / rd;
	const int sign = (rd >= 0.0f);
	if((*t) < tmin) 
		return (sign^0);
	if((*t) > tmax) 
		return (sign^1);
	return 2;
}

//** Ray vs Wald Tri intersection routine
// Intersection method return values
#define HIT		 1		// Ray hit primitive
#define MISS	 0		// Ray missed primitive
#define INPRIM	-1		// Ray started inside primitive

#define ku modulo[wt->k + 1]
#define kv modulo[wt->k + 2]
static __constant unsigned int modulo[] = { 0, 1, 2, 0, 1 };

inline int IntersectRayWTri( struct Ray * a_Ray, __global struct wald_tri * wt, float * a_Dist, float * a_Dip )
{
	float * prt_dir = &a_Ray->dir;
	float * prt_o = &a_Ray->o;
	const float dir_wt_k = prt_dir[wt->k];
	const float dir_ku = prt_dir[ku];
	const float dir_kv = prt_dir[kv];
	const float o_wt_k = prt_o[wt->k];
	const float o_ku = prt_o[ku];
	const float o_kv = prt_o[kv];
	float4 A = wt_A;
	float * ptr_A = &A;
	const float wt_A_ku = ptr_A[ku];
	const float wt_A_kv = ptr_A[kv];

	const float lnd = 1.0f / (dir_wt_k + wt->nu * dir_ku + wt->nv * dir_kv);
	const float t = (wt->nd - o_wt_k - wt->nu * o_ku - wt->nv * o_kv) * lnd;
	if (!((*a_Dist) > t && t > 0)) return MISS;
	const float hu = o_ku + t*dir_ku - wt_A_ku;
	const float hv = o_kv + t*dir_kv - wt_A_kv;
	const float beta = hv * wt->bnu + hu * wt->bnv;
	if (beta < 0) return MISS;
	const float gamma = hu * wt->cnu + hv * wt->cnv;
	if (gamma < 0) return MISS;
	if ((beta + gamma) > 1) return MISS;
	(*a_Dist) = t;
	(*a_Dip) = dot(a_Ray->dir, wt_N);
	return ( (*a_Dip) > 0 ) ? INPRIM : HIT;
}


//** Trace the ray inside the tree
int trace_tree(	struct Ray * ray, 
				float * res, 
				float * res_dip,
				
				__global struct aabb * cl_aabb,
				__global struct aabb * cut_aabbs,
				unsigned int cut_aabbs_count,
				
				__global struct BSPNode * nodes,
				__global struct wald_tri * tris,
				__global unsigned int * ids)
{
	float cur_tmin = 0;
	float cur_tmax = 100000.f;
	//check main AABB
	bool intersects = IntersectRayAABB(ray, cl_aabb, cur_tmin, cur_tmax);
	if(!intersects)
		return 0;
	//check region-of-interest AABBs
	if(intersects && cut_aabbs_count != 0)
	{
		intersects = false;
		for (unsigned int i=0; i<cut_aabbs_count; i++)
		{
			float a=0.f, b=100000.f;
			if(IntersectRayAABB(ray, &cut_aabbs[i], a, b)) 
			{
				intersects = true;
				break;
			}
		}
	}
	if(!intersects)
		return 0;

	struct intersection intersections[MAX_ISEC_COUNT];//intersection intersections[20];
	intersections[0].dist = 0;//intersections[0].dist = res;

	struct traverse_stack tr_stack;
	tr_stack.index = 0;
	//const unsigned int resint = Intersect(ray, cl_aabb, cur_tmin, cur_tmax);
	unsigned int cur_node_id = 0;
	STACK_PUSH( tr_stack, cur_node_id, cur_tmin, cur_tmax);

	__global struct BSPNode * cur_node;
	__global struct wald_tri * cur_tri;
	unsigned int sign = 0;
	unsigned int isec_count = 0;

	while (tr_stack.index > 0)
	{
		cur_node_id		= STACK_GET(tr_stack)->node;
		cur_tmin		= STACK_GET(tr_stack)->tmin;
		cur_tmax		= STACK_GET(tr_stack)->tmax;
		STACK_POP(tr_stack);
		cur_node = &nodes[cur_node_id];
		if(isLeaf(cur_node))
		{
			for (unsigned int i=0; i<cur_node->tri_count; i++)
			{
				float a_Dist = 1000000.0f, a_Dip;
				unsigned int indx = ids[i + cur_node->tri_start];
				cur_tri = &tris[indx];//read the triangle

				if (IntersectRayWTri( ray, cur_tri, &a_Dist, &a_Dip )) 
				{
					bool again = false;
					for (unsigned int i=0; i<isec_count; i++)
						if(intersections[i].prim_ind == indx)
							again = true;
					if(again)
						continue;
					
					(*res_dip) += fabs(a_Dip);
					intersections[isec_count].prim_ind = indx;
					intersections[isec_count].dist = a_Dist;
					isec_count++;
					if(isec_count == MAX_ISEC_COUNT)
						goto MAX_INTERSECTIONS_HIT;
				}
			}
		}
		else 
		{
			float t;
			const int resisec = GetIntersectionState(ray, 
				cur_tmin, cur_tmax,
				cur_node->splitCoord, axisInd(cur_node), &t);
			switch(resisec)
			{
			case 0://ray intersects left only
				if(has_left(cur_node))
					STACK_PUSH(tr_stack, cur_node->offset, cur_tmin, cur_tmax);
				break;
			case 1://ray intersects right only
				if(has_right(cur_node))
					STACK_PUSH(tr_stack, cur_node->offset+1, cur_tmin, cur_tmax);
				break;
			case 2://ray intersects left and right
				sign = ((float*)&ray->dir)[axisInd(cur_node)] >= 0.0f;
				if(sign)
				{
					if(has_left(cur_node))
						STACK_PUSH(tr_stack, cur_node->offset, cur_tmin, t);
					if(has_right(cur_node))
						STACK_PUSH(tr_stack, cur_node->offset+1, t, cur_tmax);
				}
				else
				{
					if(has_right(cur_node))
						STACK_PUSH(tr_stack, cur_node->offset+1, cur_tmin, t);
					if(has_left(cur_node))
						STACK_PUSH(tr_stack, cur_node->offset, t, cur_tmax);	
				}
				break;
			}
		}
	}

MAX_INTERSECTIONS_HIT:

	//sort intersections in one node
	for(unsigned int i = 0; i < isec_count; i++)
	{
		float tmp;
		unsigned char swapped=0;
		for(unsigned int j = 0; j < isec_count-1-i; j++)
		{
			if( intersections[j].dist > intersections[j+1].dist )
			{
				tmp = intersections[j].dist;
				intersections[j].dist = intersections[j+1].dist;
				intersections[j+1].dist = tmp;
				if(swapped==0) swapped=1;
			}
		}
		if(swapped == 0) break;
	}
	unsigned int n=0;
	if(isec_count % 2 == 0) //TODO: temporary workaround
		for (unsigned int i=0; i<isec_count; i++)
			if((i+n)%2==1)
				(*res) += intersections[i].dist - intersections[i-1].dist;
	if(isec_count) 
		(*res_dip) /= isec_count;

	return 1;
}

#define float4_at(data, index) (float4)(data[index], data[index+1], data[index+2], 0.0f)
//** Perform the batch ray casting
__kernel void raycast_batch(
					int w, int h,
					__global float * a_out_data, 
					__global float * out_dip,
					
					__global float * os,
					__global float * cs,
					__global float * dxs,
					__global float * dys,
					__global struct aabb * cl_aabb,
					__global struct aabb * cut_aabbs,
					unsigned int cut_aabbs_count,
					
					__global struct BSPNode * nodes,
					__global struct wald_tri * tris,
					__global unsigned int * ids)
{
	const int vIdx = get_global_id(0);
	const int batch = vIdx / (h*w);
	const int y = ( vIdx - (batch*w*h) ) / h;
	const int x = vIdx % w;
	int pos = 3 * batch;
	float4 dir =	float4_at(cs, pos) 
				+ x*float4_at(dxs, pos) 
				+ y*float4_at(dys, pos)
				-	float4_at(os, pos);
	dir = normalize( dir );
	struct Ray r;
	r.o = float4_at(os, pos);
	r.dir = dir;//( o, dir );
	float res = 0.0f, res_dip = 0.0f;

	trace_tree(	&r, &res, &res_dip,
				cl_aabb,
				cut_aabbs, cut_aabbs_count,
				nodes, tris, ids);

 	a_out_data	[vIdx] = res;
 	out_dip		[vIdx] = res_dip;
}