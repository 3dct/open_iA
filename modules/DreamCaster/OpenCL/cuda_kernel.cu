//#include <cutil.h>

extern "C" void init_cuda(int argc, char **argv);
extern "C" void node_texture_bind(const void *data, size_t size);
extern "C" void node_texture_unbind();
extern "C" void tri_texture_bind(const void *data, size_t size);
extern "C" void tri_texture_unbind();
extern "C" void id_texture_bind(const void *data, size_t size);
extern "C" void id_texture_unbind();
extern "C" void normal_texture_bind(const void *data, size_t size);
extern "C" void normal_texture_unbind();
extern "C" void cuda_raycast(void *a_aabb, const void * a_o, const void * a_c, const void * a_dx, const void * a_dy, int w, int h, float* out_res, float * out_dip_res, void * a_cut_aabbs, unsigned int a_cut_aabbs_count);
extern "C" void cuda_raycast_batch(void *a_aabb, void * a_o, void * a_c, void * a_dx, void * a_dy, int w, int h, float* out_res, float * out_dip_res, unsigned int batchSize, void * a_cut_aabbs, unsigned int cut_aabbs_count);
extern "C" void cuda_init(int w, int h, unsigned int batchSize);
extern "C" void cuda_terminate();

#define THREAD_W 1
#define THREAD_H 32

#define MAX_BATCH_SIZE 500
#define MAX_CUT_AAB_COUNT 10
#define MAX_ISEC_COUNT 24

void *device_out_data = 0;
void *device_out_dip = 0;
unsigned int out_size;
/* Texture for kd-tree nodes */
texture<unsigned int, 1, cudaReadModeElementType> node_tex;

/* Texture for triangles */
texture<float4, 1, cudaReadModeElementType> tri_tex;

/* Texture for ids */
texture<unsigned int, 1, cudaReadModeElementType> id_tex;

/* Texture to output resultats */
//texture<float, 1, cudaReadModeElementType> out_tex;
/***************************************************************************//**
* Initialize the node texture
/******************************************************************************/
void node_texture_bind(const void *data, size_t size)
{
	node_tex.addressMode[0] = cudaAddressModeWrap;
	node_tex.addressMode[1] = cudaAddressModeWrap;
	node_tex.filterMode = cudaFilterModePoint;
	node_tex.normalized = false;
	cudaBindTexture(0, node_tex, data, size);
}
void node_texture_unbind()
{
	cudaUnbindTexture(node_tex);
}
/***************************************************************************//**
* Initialize the triangle texture
/******************************************************************************/
void tri_texture_bind(const void *data, size_t size)
{
	tri_tex.addressMode[0] = cudaAddressModeWrap;
	tri_tex.addressMode[1] = cudaAddressModeWrap;
	tri_tex.filterMode = cudaFilterModePoint;
	tri_tex.normalized = false;
	cudaBindTexture(0, tri_tex, data, size);
}
void tri_texture_unbind()
{
	cudaUnbindTexture(tri_tex);
}
/***************************************************************************//**
* Initialize the id texture
/******************************************************************************/
void id_texture_bind(const void *data, size_t size)
{
	id_tex.addressMode[0] = cudaAddressModeWrap;
	id_tex.addressMode[1] = cudaAddressModeWrap;
	id_tex.filterMode = cudaFilterModePoint;
	id_tex.normalized = false;
	cudaBindTexture(0, id_tex, data, size);
}
void id_texture_unbind()
{
	cudaUnbindTexture(id_tex);
}

/**	\struct iAVec3.
\brief Class representing 3 dimensional vector. CUDA version.
*/
struct iAVec3
{
public:
	float x,y,z;
	__device__ iAVec3(){}
	__device__ iAVec3(float px, float py, float pz) :x(px), y(py), z(pz)	{}
	//__device__ iAVec3(const iAVec3& v):x(v.x), y(v.y), z(v.z)	{}
	__device__ iAVec3& operator= (const iAVec3& v) { x=v.x;	y=v.y; z=v.z;	return *this; }
	__device__ iAVec3 operator+ () const { return *this; }
	__device__ iAVec3 operator- () const { return iAVec3(-x,-y,-z); }
	__device__ iAVec3& operator+= (const iAVec3& v)	{ x+=v.x; y+=v.y; z+=v.z;	return *this; }
	__device__ iAVec3& operator-= (const iAVec3& v)	{ x-=v.x; y-=v.y; z-=v.z;	return *this; }
	__device__ iAVec3& operator*= (const iAVec3& v) { x*=v.x; y*=v.y; z*=v.z;	return *this; }
	__device__ iAVec3& operator*= (float f)       { x*=f; y*=f; z*=f;			return *this; }
	__device__ iAVec3& operator/= (const iAVec3& v)	{ x/=v.x; y/=v.y; z/=v.z;	return *this; }
	__device__ const float& operator[] (int index) const { return *(index+&x); }
	__device__ float& operator[] (int index) { return *(index+&x); }
	__device__ int operator== (const iAVec3& v) const	{ return x==v.x&&y==v.y&&z==v.z; }
	__device__ int operator!= (const iAVec3& v) const	{ return x!=v.x||y!=v.y||z!=v.z; }
	__device__ int operator<  (const iAVec3& v) const	{ return ( x < v.x ) || ((x == v.x) && (y < v.y)); }
	__device__ int	    operator>  (const iAVec3& v) const { return ( x > v.x ) || ((x == v.x) && (y > v.y)); }
	__device__ float	length () const	{ return (float) sqrt(x*x + y*y + z*z);	}
	/*
	friend __device__ iAVec3 operator + (const iAVec3&,const iAVec3&);
		friend __device__ iAVec3 operator - (const iAVec3&,const iAVec3&);
		friend __device__ iAVec3 operator * (const iAVec3&,const iAVec3&);
		friend __device__ iAVec3 operator * (float          ,const iAVec3&);
		friend __device__ iAVec3 operator * (const iAVec3&,float);
		friend __device__ iAVec3 operator / (const iAVec3&,float);
		friend __device__ iAVec3 operator / (const iAVec3&,const iAVec3&);
		friend __device__ float    operator & (const iAVec3&,const iAVec3&);
		friend __device__ iAVec3 operator ^ (const iAVec3&,const iAVec3&);*/
	
};
__device__  iAVec3 operator + (const iAVec3& u,const iAVec3& v) {	return iAVec3(u.x + v.x, u.y + v.y, u.z + v.z); }
__device__  iAVec3 operator - (const iAVec3& u,const iAVec3& v) {	return iAVec3(u.x - v.x, u.y - v.y, u.z - v.z); }
__device__  iAVec3 operator * (const iAVec3& u,const iAVec3& v) { return iAVec3(u.x * v.x, u.y * v.y, u.z * v.z); }
__device__  iAVec3 operator * (const iAVec3& v,float a) { return iAVec3(v.x*a, v.y*a, v.z*a); }
__device__  iAVec3 operator * (float a, const iAVec3& v) { return iAVec3(v.x*a, v.y*a, v.z*a); }
__device__  iAVec3 operator / (const iAVec3& v,float a) { return iAVec3(v.x/a, v.y/a, v.z/a); }
__device__  iAVec3 operator / (const iAVec3& u,const iAVec3& v) {	return iAVec3(u.x / v.x, u.y / v.y, u.z / v.z); }
//dot
__device__  float    operator & (const iAVec3& u,const iAVec3& v) {	return u.x*v.x + u.y*v.y + u.z*v.z; }
//cross
__device__  iAVec3 operator ^ (const iAVec3& u,const iAVec3& v) {	return iAVec3(u.y*v.z-u.z*v.y, u.z*v.x-u.x*v.z, u.x*v.y-u.y*v.x); }
__device__ void normalize_vec3(iAVec3& u)
{
	float len = u.length();
	if(len>0)
		u = u / u.length();
}
/**	\struct ct_state.
	\brief Structure describe current CT state.
*/
struct ct_state{
	iAVec3 o;///< rays origin
	iAVec3 c;///< corner of plate
	iAVec3 dx;///< dx of plane in 3d
	iAVec3 dy;///< dy of plane in 3d
};

/***************************************************************************//**
* The axis-aligned bounding box
/******************************************************************************/
struct aabb {
	float x1,x2,y1,y2,z1,z2;
	int isInside(iAVec3& v) const
	{
		if(v.x<=x2)
			if(v.x>=x1) 
				if(v.y<=y2)
					if(v.y>=y1) 
						if(v.z<=z2)
							if(v.z>=z1)
								return 1;
		return 0;
	}
};

//constants  *******************************************************************/
__constant__ iAVec3 o;
__constant__ iAVec3 c;
__constant__ iAVec3 dx;
__constant__ iAVec3 dy;
//for batching MAX_BATCH_SIZE - is max batch size
__constant__ iAVec3 os[MAX_BATCH_SIZE];
__constant__ iAVec3 cs[MAX_BATCH_SIZE];
__constant__ iAVec3 dxs[MAX_BATCH_SIZE];
__constant__ iAVec3 dys[MAX_BATCH_SIZE];
//__constant__ unsigned int tri_count;
//__constant__ ct_state cur_ctstate;
/* The bounding box of the scene */
__constant__ aabb cu_aabb;
__constant__ unsigned int cut_aabbs_count;
__constant__ aabb cut_aabbs[MAX_CUT_AAB_COUNT];


/**	\struct BSPNode.
\brief Class representing a BSP-tree node.

BSP-tree node.	
*/
class BSPNode
{
#define internal1 internal_data.x
#define internal2 internal_data.y
#define masked_vars internal_data.z
public:
	//
	//char masked_vars; ///< Is this node a leaf-node first bit -- is leaf, has left, has right, else -- axis index
	uint3 internal_data;
	//internal1, internal2,masked_vars;///< shared data, depends if node is leaf or not
	//
	__device__ bool isLeaf() const {return masked_vars&0x00000080;}
	__device__ int axisInd() const { return masked_vars&0x00000003;}
	__device__ void setLeaf(bool a_isLeaf){ masked_vars&=0x0000007f; if(a_isLeaf)masked_vars|=0x00000080; }
	__device__ void setAxisInd(int index){
		switch(index){
			case 0:	 masked_vars|=(char)0x00000000; break;
			case 1:	 masked_vars|=(char)0x00000001; break;
			default: masked_vars|=(char)0x00000002; break;
		}
	}
	__device__ bool has_left() const {return masked_vars&0x00000040;}
	__device__ bool has_right() const {return masked_vars&0x00000020;}
	__device__ void set_has_left (bool has){ masked_vars&=0x000000bf; if(has) masked_vars|=0x00000040;	}
	__device__ void set_has_right(bool has){ masked_vars&=0x000000df; if(has) masked_vars|=0x00000020;	}
	__device__ unsigned int tri_start() {return internal1;}
	__device__ unsigned int tri_count() {return internal2;}
	__device__ unsigned int offset()    {return internal1;}
	__device__ float & splitCoord()      {return *((float*)&internal2);}
	__device__ void set_tri_start(unsigned int val) { internal1=val; }
	__device__ void set_tri_count(unsigned int val) { internal2=val; }
	__device__ void set_offset(unsigned int val)    { internal1=val; }
	__device__ void set_splitCoord(float val)       { internal2=*((unsigned int*)&val); }
	//__device__ BSPNode *get_left(std::vector<BSPNode*> &nodes)  { return nodes[offset()]; }
	//__device__ BSPNode *get_right(std::vector<BSPNode*> &nodes) { return nodes[offset()+1]; }
	//__device__ BSPNode *get_left(const std::vector<BSPNode*> &nodes)  { return nodes[offset()]; }
	//__device__ BSPNode *get_right(const std::vector<BSPNode*> &nodes) { return nodes[offset()+1]; }
};
/**	\struct traverse_stack.
	\brief Used for tree traversal.

	CUDA version.	
*/
struct trace_t {
	//__device__ trace_t(unsigned int aa, float a, float b):node(aa),tmin(a),tmax(b){}
	unsigned int node;
	float tmin;
	float tmax;
};
struct traverse_stack
{
	int index;
	trace_t t[30];

	//__device__ traverse_stack(int a_size){ size = a_size; index=0; }
	__device__ traverse_stack(): index(0) {}
	__device__ inline void push(unsigned int node,	float tmin,	float tmax)
	{ 
		t[index].node = node; t[index].tmin = tmin; t[index].tmax = tmax; 
		index++;
	}
	__device__ inline trace_t& get() { return t[index-1]; }
	__device__ inline trace_t& pop() { index--; return t[index]; }
	__device__ inline int numElements() { return index; }
};
/**	\struct wald_tri.
\brief Structure representing a triangle data needed for intersection test.

Wald triangle structure.	
*/

struct wald_tri
{
	float4 intern0, intern1, intern2, intern3;

	__device__ iAVec3 m_N(){ return iAVec3(intern0.x, intern0.y, intern0.z);}
	__device__ iAVec3 m_A(){ return iAVec3(intern0.w, intern1.x, intern1.y);}					
	__device__ float nu(){ return intern1.z;}
	__device__ float nv(){ return intern1.w;}
	__device__ float nd(){ return intern2.x;}						
	__device__ unsigned int k(){ return __float_as_int(intern2.y);}										
	__device__ float bnu(){ return intern2.z;}
	__device__ float bnv(){ return intern2.w;}						
	__device__ float cnu(){ return intern3.x;}
	__device__ float cnv(){ return intern3.y;}					
};

/**	\class Ray.
\brief Class representing ray in 3D.

CUDA version.	
*/
class Ray
{
public:
	__device__ Ray() : o( iAVec3( 0.f, 0.f, 0.f ) ), dir( iAVec3( 0, 0, 0 ) ) {}
	__device__ Ray( iAVec3& a_Origin, iAVec3& a_Dir ) {o=a_Origin; dir = a_Dir;}

	iAVec3 o;	///< ray origin's position
	iAVec3 dir;	///< ray direction vector
};
/**	\struct intersection.
\brief Structure representing intersection data.

Contains data about primitive. CUDA version	
*/
struct intersection
{
	unsigned int prim_ind;
	float dist;
};

/***************************************************************************//**
 * Initialize CUDA
/******************************************************************************/
void init_cuda(int argc, char **argv)
{
	//CUT_DEVICE_INIT(argc, argv);
}
/**
* Ray-AABB intersection routine. CUDA version.
* @param ray ray class.
* @param box axis aligned bounding box structure.
* @return 
1 - if ray intersects AABB
0 - otherwise
*/
static __device__ inline unsigned int Intersect(const Ray &ray, const aabb& box, float &tmin, float&tmax)
{
	float l1 = __fdividef(box.x1 - ray.o.x, ray.dir.x);
	float l2 = __fdividef(box.x2 - ray.o.x, ray.dir.x);
	tmin = fmaxf(fminf(l1,l2), tmin);
	tmax = fminf(fmaxf(l1,l2), tmax);
	l1 = __fdividef(box.y1 - ray.o.y, ray.dir.y);
	l2 = __fdividef(box.y2 - ray.o.y, ray.dir.y);
	tmin = fmaxf(fminf(l1,l2), tmin);
	tmax = fminf(fmaxf(l1,l2), tmax);
	l1 = __fdividef(box.z1 - ray.o.z, ray.dir.z);
	l2 = __fdividef(box.z2 - ray.o.z, ray.dir.z);
	tmin = fmaxf(fminf(l1,l2), tmin);
	tmax = fminf(fmaxf(l1,l2), tmax);
	return ((tmax >= tmin) & (tmax >= 0.f));
/*
	float txmin, txmax, tymin, tymax;
	float ddx = 1.0f/ray.dir.x;
	float ddy = 1.0f/ray.dir.y;
	if(ddx>=0)
	{
		txmin = (box.x1 - ray.o.x) * ddx;
		txmax = (box.x2 - ray.o.x) * ddx;
	}
	else
	{
		txmin = (box.x2 - ray.o.x) * ddx;
		txmax = (box.x1 - ray.o.x) * ddx;
	}
	if(ddy>=0)
	{
		tymin = (box.y1 - ray.o.y) * ddy;
		tymax = (box.y2 - ray.o.y) * ddy;
	}
	else
	{
		tymin = (box.y2 - ray.o.y) * ddy;
		tymax = (box.y1 - ray.o.y) * ddy;
	}
	if( (txmin>tymax) || (tymin>txmax) ) return 0;
	if( tymin>txmin ) txmin=tymin;
	if( tymax<txmax ) txmax=tymax;

	float tzmin, tzmax;
	float ddz = 1.0f/ray.dir.z;
	if(ddz>=0)
	{
		tzmin = (box.z1 - ray.o.z) * ddz;
		tzmax = (box.z2 - ray.o.z) * ddz;
	}
	else
	{
		tzmin = (box.z2 - ray.o.z) * ddz;
		tzmax = (box.z1 - ray.o.z) * ddz;
	}
	if( (txmin>tzmax) || (tzmin>txmax) ) return 0;
	if( tzmin>txmin ) txmin=tzmin;
	if( tzmax<txmax ) txmax=tzmax;
	tmin=txmin;
	tmax=txmax;
	return 1;*/

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
__device__ inline int GetIntersectionState(const Ray &ray, float &tmin, float &tmax, float &split, int splitIndex, float &t)
{
	float rd = *(&ray.dir.x+splitIndex);
	if(!rd)
		rd=0.00000001f;
	t = (split - *(&ray.o.x+splitIndex)) / rd;
	const unsigned int sign = (rd >= 0.0f);
	if(t<tmin) return (int)(sign^0);
	if(t>tmax) return (int)(sign^1);
	return 2;
}
/***************************************************************************//**
* Ray vs Wald Tri intersection routine
/******************************************************************************/
// Intersection method return values
#define HIT		 1		// Ray hit primitive
#define MISS	 0		// Ray missed primitive
#define INPRIM	-1		// Ray started inside primitive
#define ku modulo[wt.k() + 1]
#define kv modulo[wt.k() + 2]

__constant__ unsigned int modulo[] = { 0, 1, 2, 0, 1 };
__device__ inline int Intersect(const Ray& a_Ray, wald_tri& wt, float& a_Dist, float& a_Dip )
{
	const float lnd = __fdividef(1.0f, (a_Ray.dir[wt.k()] + wt.nu() * a_Ray.dir[ku] + wt.nv() * a_Ray.dir[kv]));
	const float t = (wt.nd() - a_Ray.o[wt.k()] - wt.nu() * a_Ray.o[ku] - wt.nv() * a_Ray.o[kv]) * lnd;
	if (!(a_Dist > t && t > 0)) return MISS;
	const float hu = a_Ray.o[ku] + t*a_Ray.dir[ku] - wt.m_A()[ku];
	const float hv = a_Ray.o[kv] + t*a_Ray.dir[kv] - wt.m_A()[kv];
	const float beta = hv * wt.bnu() + hu * wt.bnv();//=wt.m_U=//или наоборот
	if (beta < 0) return MISS;
	const float gamma = hu * wt.cnu() + hv * wt.cnv();//=m_WaldTri.m_V=
	if (gamma < 0) return MISS;
	if ((beta + gamma) > 1) return MISS;
	a_Dist = t;
	a_Dip=a_Ray.dir&wt.m_N();
	return ( a_Dip > 0 )? INPRIM : HIT;
}
__device__ inline int Intersect_v2(const Ray& a_Ray, wald_tri& wt, float& a_Dist )
{
	const float lnd = 1.0f / (a_Ray.dir[wt.k()] + wt.nu() * a_Ray.dir[ku] + wt.nv() * a_Ray.dir[kv]);
	const float t = (wt.nd() - a_Ray.o[wt.k()] - wt.nu() * a_Ray.o[ku] - wt.nv() * a_Ray.o[kv]) * lnd;
	if (!(a_Dist > t && t > 0)) return MISS;
	float hu = a_Ray.o[ku] + t*a_Ray.dir[ku] - wt.m_A()[ku];
	float hv = a_Ray.o[kv] + t*a_Ray.dir[kv] - wt.m_A()[kv];
	float beta = hv * wt.bnu() + hu * wt.bnv();//=wt.m_U=//или наоборот
	if (beta < 0) return MISS;
	float gamma = hu * wt.cnu() + hv * wt.cnv();//=m_WaldTri.m_V=
	if (gamma < 0) return MISS;
	if ((beta + gamma) > 1) return MISS;
	a_Dist = t;
	return ((a_Ray.dir&wt.m_N() ) > 0)? INPRIM : HIT;
}

__device__ inline  void ReadTri(wald_tri &tri, const unsigned int & index)
{
	tri.intern0 = tex1Dfetch(tri_tex, 4 * index);
	tri.intern1 = tex1Dfetch(tri_tex, 4 * index+1);
	tri.intern2 = tex1Dfetch(tri_tex, 4 * index+2);
	tri.intern3 = tex1Dfetch(tri_tex, 4 * index+3);
}
/***************************************************************************//**
* Trace the ray inside the tree
/******************************************************************************/
static __device__ inline int trace_tree(const Ray &ray, float &res, float &res_dip)
{
	//__shared__ wald_tri cur_tri[THREAD_W][THREAD_H];
	//unsigned int isec_count=0;
	res = 0;//100000.f;
	res_dip=0;
	float cur_tmin=0;
	float cur_tmax=100000.f;
	if(!Intersect(ray, cu_aabb, cur_tmin, cur_tmax)) return 0;
	//check for cut aabbs
	if(cut_aabbs_count!=0)
	{
		bool intersects = false;
		for (unsigned int i=0; i<cut_aabbs_count; i++)
		{
			float a=0.f, b=100000.f;
			if(Intersect(ray, cut_aabbs[i], a, b)) 
			{
				intersects = true;
				break;
			}
		}
		if(!intersects)
			return 0;
	}
	intersection intersections[MAX_ISEC_COUNT];//intersection intersections[20];
	intersections[0].dist = 0;//intersections[0].dist = res;
	unsigned int isec_count=0;
	traverse_stack tr_stack;
	//const unsigned int resint = Intersect(ray, cu_aabb, cur_tmin, cur_tmax);
	unsigned int cur_node_id=0;
	tr_stack.push(cur_node_id, cur_tmin, cur_tmax);
	BSPNode  cur_node;
	unsigned int sign = 0;
	//unsigned int node_isecs_start=0;
	while (tr_stack.numElements() > 0)
	{
		cur_node_id = tr_stack.get().node;
		cur_tmin = tr_stack.get().tmin;
		cur_tmax = tr_stack.get().tmax;
		tr_stack.pop();
		cur_node.internal1 = tex1Dfetch(node_tex, 3 * cur_node_id);
		cur_node.internal2 = tex1Dfetch(node_tex, 3 * cur_node_id+1);
		cur_node.masked_vars = tex1Dfetch(node_tex, 3 * cur_node_id+2);

		//tmin=cur_tmin; tmax=cur_tmax;
		if(cur_node.isLeaf())
		{
			wald_tri cur_tri;
			//node_isecs_start = isec_count;
			for (unsigned int i=0; i<cur_node.tri_count(); i++)
			{
				float a_Dist = 1000000.0f, a_Dip;
				//ReadTri(cur_tri, cur_node.tri_start()+i);
				const unsigned int indx = tex1Dfetch(id_tex, cur_node.tri_start()+i);
				ReadTri(cur_tri, indx);
				if (Intersect( ray, cur_tri, a_Dist, a_Dip )) 
				{
					unsigned char again=0;
					for (unsigned int i=0; i<isec_count; i++)
					{
						if(intersections[i].prim_ind==indx)
							again=1;
							
					}
					if(again)
						continue;
					//iAVec3 isec = ray.o+ray.dir*a_Dist;			
					intersections[isec_count].prim_ind = indx;
					intersections[isec_count].dist=a_Dist;//checked
					isec_count++;
					res_dip+=fabsf(a_Dip);
					/*intersections[isec_count].prim_ind = cur_node.tri_start()+i;
					intersections[isec_count].dist =a_Dist;//checked
					if(intersections[isec_count].dist<intersections[0].dist)
					{
						intersections[isec_count].prim_ind = intersections[0].prim_ind;
						intersections[isec_count].dist =intersections[0].dist;
						intersections[0].prim_ind = cur_node.tri_start()+i;
						intersections[0].dist = a_Dist;//checked
					}
					isec_count++;*/
				}
			}
		}
		else 
		{
			float t;
			const int resisec = GetIntersectionState(ray, cur_tmin, cur_tmax, cur_node.splitCoord(), cur_node.axisInd(),t);
			switch(resisec)
			{
			case 0://left only
				if(cur_node.has_left())
				{
					tr_stack.push(cur_node.offset(),cur_tmin,cur_tmax);
				}
				break;
			case 1://right only
				if(cur_node.has_right())
				{
					tr_stack.push(cur_node.offset()+1,cur_tmin,cur_tmax);
				}
				break;
			case 2://both
				sign = ray.dir[cur_node.axisInd()]>=0.0f;
				if(sign)
				{
					if(cur_node.has_left())
						tr_stack.push(cur_node.offset(),cur_tmin,t);
					if(cur_node.has_right())
						tr_stack.push(cur_node.offset()+1,t,cur_tmax);
				}
				else
				{
					if(cur_node.has_right())
						tr_stack.push(cur_node.offset()+1,cur_tmin,t);
					if(cur_node.has_left())
						tr_stack.push(cur_node.offset(),t,cur_tmax);	
				}
				/*if(cur_node.has_right())
				{
					if(sign)
						tr_stack.push(cur_node.offset()+1,t,cur_tmax);
					else
						tr_stack.push(cur_node.offset()+1,cur_tmin,t);
				}
				if(cur_node.has_left())
				{
					if(sign)
						tr_stack.push(cur_node.offset(),cur_tmin,t);
					else
						tr_stack.push(cur_node.offset(),t,cur_tmax);					
				}*/
				break;
			}
		}
	}

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
	{
		if((i+n)%2==1)
		{
			//TODO: no triangles repeated?
			if(intersections[i].prim_ind == intersections[i-1].prim_ind)//if intersections belong to the same triangle
				n++;
			else
				res+=intersections[i].dist - intersections[i-1].dist;
		}
	}
	if(isec_count) res_dip /= isec_count;
	//res=intersections.dist;
	//res=intersections[0].dist;
	return 1;
}
/***************************************************************************//**
* Brutforce raycasting
/******************************************************************************/
/*static __device__ inline int brutforce(const Ray &ray, float &res)
{
	intersection isec;
	res = 100000.f;
	isec.dist = res;
	float cur_tmin=0;
	float cur_tmax=100000.f;
	if(!Intersect(ray, cu_aabb, cur_tmin, cur_tmax)) return 0;
	wald_tri cur_tri;
	for (unsigned int i=0; i<tri_count; i++)
	{
		float a_Dist = 1000000.0f;
		ReadTri(cur_tri, i);
		if (Intersect( ray, cur_tri, a_Dist )) 
		{
			//iAVec3 isec = ray.GetOrigin()+ray.GetDirection()*a_Dist;			
			if(a_Dist<isec.dist)
			{
				isec.prim_ind = i;
				isec.dist =a_Dist;//checked
			}
		}
	}
	res=isec.dist;
	return 1;
}*/
/***************************************************************************//**
* Perform the ray casting
/******************************************************************************/
__global__ void raycast(int w, int h, void* a_out_data, void* out_dip)
{
	const int tx = threadIdx.x;
	const int ty = threadIdx.y;
	const int bw = blockDim.x;
	const int bh = blockDim.y;
	const int x = blockIdx.x*bw + tx;
	const int y = blockIdx.y*bh + ty;
	
	iAVec3 dir = (c + x*dx + y*dy) - o;
	normalize_vec3( dir );
	Ray r;
	r.o=o;
	r.dir=dir;//( o, dir );
	float res, res_dip;
	trace_tree(r, res, res_dip);
	//cudaMemcpy(out_data+(y * w + x)*sizeof(float), &res, sizeof(float), cudaMemcpyDeviceToDevice);
	const int xinv=w-1-x;
	((float*)a_out_data)[y * w + xinv]=res;
	((float*)out_dip)[y * w + xinv]=res_dip;
}
/***************************************************************************//**
* Raycast with CUDA
/******************************************************************************/
void cuda_raycast(void *a_aabb, const void * a_o, const void * a_c, const void * a_dx, const void * a_dy, int w, int h, float* out_res, float * out_dip_res, void * a_cut_aabbs, unsigned int a_cut_aabbs_count)
{
	cudaError_t res = cudaMalloc(&device_out_data, out_size);
	res = cudaMalloc(&device_out_dip, out_size);
	dim3 block, grid;
	block = dim3(THREAD_W, THREAD_H, 1);
	grid = dim3(w / block.x, h / block.y, 1);
	cudaMemcpyToSymbol(o, a_o, sizeof(iAVec3));
	cudaMemcpyToSymbol(c, a_c, sizeof(iAVec3));
	cudaMemcpyToSymbol(dx, a_dx, sizeof(iAVec3));
	cudaMemcpyToSymbol(dy, a_dy, sizeof(iAVec3));
	cudaMemcpyToSymbol(cu_aabb, a_aabb, sizeof(aabb));
	cudaMemcpyToSymbol(cut_aabbs_count, &a_cut_aabbs_count, sizeof(unsigned int));
	cudaMemcpyToSymbol(cut_aabbs, a_cut_aabbs, sizeof(aabb)*MAX_CUT_AAB_COUNT);//TODO: krivo max size brat'
	raycast<<<grid, block>>>(w, h, device_out_data, device_out_dip);	
	res = cudaMemcpy(out_res, device_out_data, out_size, cudaMemcpyDeviceToHost);
	res = cudaMemcpy(out_dip_res, device_out_dip, out_size, cudaMemcpyDeviceToHost);
	res = cudaFree(device_out_data);
	device_out_data=0;
	res = cudaFree(device_out_dip);
	device_out_dip=0;
}

/***************************************************************************//**
* Perform the batch ray casting
/******************************************************************************/
__global__ void raycast_batch(int w, int h, unsigned int batchSize, void* a_out_data, void* out_dip)
{
	const int tx = threadIdx.x;
	const int ty = threadIdx.y;
	const int bw = blockDim.x;
	const int bh = blockDim.y;
	const int x1 = blockIdx.x*bw + tx;
	const int batch = x1 / w;
	const int offset = w*h*batch;
	const int x = x1 % w;
	const int y = blockIdx.y*bh + ty;

	iAVec3 dir = (cs[batch] + x*dxs[batch] + y*dys[batch]) - os[batch];
	normalize_vec3( dir );
	Ray r;
	r.o=os[batch];
	r.dir=dir;//( o, dir );
	float res, res_dip;
	trace_tree(r, res, res_dip);
	//cudaMemcpy(out_data+(y * w + x)*sizeof(float), &res, sizeof(float), cudaMemcpyDeviceToDevice);
	const int xinv = w - x - 1; //inv for inteverion)
	((float*)a_out_data)[offset + y * w + xinv] = res;
	((float*)out_dip)[offset + y * w + xinv] = res_dip;
}
/***************************************************************************//**
* Raycast batch with CUDA
/******************************************************************************/
//TODO: handling restrictions, maybe should return error code
void cuda_raycast_batch(void *a_aabb, void * a_o, void * a_c, void * a_dx, void * a_dy, int w, int h, float* out_res, float * out_dip_res, unsigned int batchSize, void * a_cut_aabbs, unsigned int a_cut_aabbs_count)
{
	cudaError_t res = cudaMalloc(&device_out_data, out_size);
	res = cudaMalloc(&device_out_dip, out_size);
	dim3 block, grid;
	block = dim3(THREAD_W, THREAD_H, 1);
	grid = dim3(w*batchSize / block.x, h / block.y, 1);
	cudaMemcpyToSymbol(os, a_o, sizeof(iAVec3)*batchSize);
	cudaMemcpyToSymbol(cs, a_c, sizeof(iAVec3)*batchSize);
	cudaMemcpyToSymbol(dxs, a_dx, sizeof(iAVec3)*batchSize);
	cudaMemcpyToSymbol(dys, a_dy, sizeof(iAVec3)*batchSize);
	cudaMemcpyToSymbol(cu_aabb, a_aabb, sizeof(aabb));
	cudaMemcpyToSymbol(cut_aabbs_count, &a_cut_aabbs_count, sizeof(unsigned int));
	cudaMemcpyToSymbol(cut_aabbs, a_cut_aabbs, sizeof(aabb)*MAX_CUT_AAB_COUNT);//TODO: krivo max size brat'
	//cudaMemcpyToSymbol(tri_count, a_tri_count, sizeof(unsigned int));
	raycast_batch<<<grid, block>>>(w, h, batchSize, device_out_data, device_out_dip);	
	res = cudaMemcpy(out_res, device_out_data, out_size, cudaMemcpyDeviceToHost);
	res = cudaMemcpy(out_dip_res, device_out_dip, out_size, cudaMemcpyDeviceToHost);
	res = cudaFree(device_out_data);
	device_out_data=0;
	res = cudaFree(device_out_dip);
	device_out_dip=0;
	//raycast<<<grid, block, 100*(4*sizeof(iAVec3)+sizeof(cu_aabb))>>>(w, h);
}
void cuda_init(int w, int h, unsigned int batchSize)
{
	out_size = w*h*sizeof(float)*batchSize;
	//cudaMalloc(&device_out_data, out_size);
	//cudaMalloc(&device_out_dip, out_size);
}
void cuda_terminate()
{
	if(device_out_data)
	{
		cudaFree(device_out_data);
		device_out_data=0;
	}
	if(device_out_dip)
	{
		cudaFree(device_out_dip);
		device_out_dip=0;
	}
}