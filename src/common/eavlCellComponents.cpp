// Copyright 2010-2013 UT-Battelle, LLC.  See LICENSE.txt for more information.
#include "eavlCellComponents.h"

// The orderings used in these tables match that used in VisIt.

// ----------------------------------------------------------------------------
signed char eavlTetEdges[6][2] =
{
  { 0, 1 }, // 0
  { 1, 2 }, // 1
  { 2, 0 }, // 2
  { 0, 3 }, // 3
  { 1, 3 }, // 4
  { 2, 3 }  // 5
};

signed char eavlTetTriangleFaces[4][3] =
{
  { 0, 1, 3 }, // 0
  { 1, 2, 3 }, // 1
  { 2, 0, 3 }, // 2
  { 0, 2, 1 }  // 3
};


// ----------------------------------------------------------------------------
signed char eavlPyramidEdges[8][2] =
{
  { 0, 1 }, // 0
  { 1, 2 }, // 1
  { 2, 3 }, // 2
  { 3, 0 }, // 3
  { 0, 4 }, // 4
  { 1, 4 }, // 5
  { 2, 4 }, // 6
  { 3, 4 }  // 7
};

signed char eavlPyramidTriangleFaces[4][3] =
{
  { 0, 1, 4 }, // 0
  { 1, 2, 4 }, // 1
  { 2, 3, 4 }, // 2
  { 3, 0, 4 }  // 3
};

signed char eavlPyramidQuadFaces[1][4] =
{
  { 0, 3, 2, 1 } // 0
};


// ----------------------------------------------------------------------------
signed char eavlWedgeEdges[9][2] =
{
  { 0, 1 }, // 0
  { 1, 2 }, // 1
  { 2, 0 }, // 2
  { 3, 4 }, // 3
  { 4, 5 }, // 4
  { 5, 3 }, // 5
  { 0, 3 }, // 6
  { 1, 4 }, // 7
  { 2, 5 }  // 8
};

signed char eavlWedgeTriangleFaces[2][3] =
{
  { 0, 1, 2 }, // 0
  { 3, 5, 4 }  // 1
};

signed char eavlWedgeQuadFaces[3][4] =
{
  { 0, 3, 4, 1 }, // 0
  { 1, 4, 5, 2 }, // 1
  { 2, 5, 3, 0 }  // 2
};


// ----------------------------------------------------------------------------
signed char eavlHexEdges[12][2] =
{
  { 0, 1 },   //  0
  { 1, 2 },   //  1
  { 2, 3 },   //  2
  { 0, 3 },   //  3
  { 4, 5 },   //  4
  { 5, 6 },   //  5
  { 6, 7 },   //  6
  { 4, 7 },   //  7
  { 0, 4 },   //  8
  { 1, 5 },   //  9
  { 3, 7 },   // 10
  { 2, 6 }    // 11
};

signed char eavlHexQuadFaces[6][4] =
{
  { 0, 4, 7, 3 }, // 0
  { 1, 2, 6, 5 }, // 1
  { 0, 1, 5, 4 }, // 2
  { 3, 7, 6, 2 }, // 3
  { 0, 3, 2, 1 }, // 4
  { 4, 5, 6, 7 }  // 5
};


// ----------------------------------------------------------------------------
signed char eavlVoxEdges[12][2] =
{
  { 0, 1 }, //  0
  { 1, 3 }, //  1
  { 2, 3 }, //  2
  { 0, 2 }, //  3
  { 4, 5 }, //  4
  { 5, 7 }, //  5
  { 6, 7 }, //  6
  { 4, 6 }, //  7
  { 0, 4 }, //  8
  { 1, 5 }, //  9
  { 2, 6 }, // 10
  { 3, 7 }  // 11
};


signed char eavlVoxQuadFaces[6][4] =
{ 
  { 0, 4, 6, 2 }, // 0
  { 1, 3, 7, 5 }, // 1
  { 0, 1, 5, 4 }, // 2
  { 2, 6, 7, 3 }, // 3
  { 0, 2, 3, 1 }, // 4
  { 4, 5, 7, 6 }  // 5
};

// ----------------------------------------------------------------------------
///\todo: all isoline tables should be moved to the new GPU friendly style
signed char eavlTriIsolineEdges[8][3] =
{
  { -1, -1, -1}, // 0
  {  0,  2, -1}, // 1  0
  {  0,  1, -1}, // 2  1
  {  1,  2, -1}, // 3  0&1
  {  1,  2, -1}, // 4  2
  {  0,  1, -1}, // 5  2&0
  {  0,  2, -1}, // 6  2&1
  { -1, -1, -1}  // 7  2&0&1
};

signed char eavlTriEdges[3][2] =
{
  {0,1}, // 0
  {1,2}, // 1
  {2,0}  // 2
};

signed char eavlQuadIsolineEdges[16][5] =
{
  { -1, -1, -1, -1, -1}, // 0
  {  0,  3, -1, -1, -1}, // 1   0
  {  0,  1, -1, -1, -1}, // 2   1
  {  1,  3, -1, -1, -1}, // 3   0&1
  {  1,  2, -1, -1, -1}, // 4   2
  {  0,  1,  2,  3, -1}, // 5   2&0
  {  0,  2, -1, -1, -1}, // 6   2&1
  {  2,  3, -1, -1, -1}, // 7   2&0&1 (3)
  {  2,  3, -1, -1, -1}, // 8   3
  {  0,  2, -1, -1, -1}, // 9   3&0
  {  1,  2,  3,  0, -1}, // 10  3&1
  {  1,  2, -1, -1, -1}, // 11  3&0&1 (2)
  {  1,  3, -1, -1, -1}, // 12  3&2
  {  0,  1, -1, -1, -1}, // 13  3&2&0 (1)
  {  0,  3, -1, -1, -1}, // 14  3&2&1 (0)
  { -1, -1, -1, -1, -1}  // 15  3&2&0&1
};

signed char eavlQuadEdges[4][2] =
{
  {0,1}, // 0
  {1,2}, // 1
  {2,3}, // 2
  {3,0}  // 3
};

// ----------------------------------------------------------------------------
signed char eavlPixelIsolineEdges[16][5] =
{
  { -1, -1, -1, -1, -1}, // 0
  {  0,  3, -1, -1, -1}, // 1   0
  {  0,  1, -1, -1, -1}, // 2   1
  {  1,  3, -1, -1, -1}, // 3   0&1
  {  2,  3, -1, -1, -1}, // 4   2
  {  0,  2, -1, -1, -1}, // 5   2&0
  {  1,  2,  3,  0, -1}, // 6   2&1
  {  1,  2, -1, -1, -1}, // 7   2&0&1 (3)
  {  1,  2, -1, -1, -1}, // 8   3
  {  0,  1,  2,  3, -1}, // 9   3&0
  {  0,  2, -1, -1, -1}, // 10  3&1
  {  2,  3, -1, -1, -1}, // 11  3&0&1 (2)
  {  1,  3, -1, -1, -1}, // 12  3&2
  {  0,  1, -1, -1, -1}, // 13  3&2&0 (1)
  {  0,  3, -1, -1, -1}, // 14  3&2&1 (0)
  { -1, -1, -1, -1, -1}  // 15  3&2&0&1


};

signed char eavlPixelEdges[4][2] =
{
  {0,1}, // 0
  {1,3}, // 1
  {3,2}, // 2
  {2,0}  // 3
};

// ----------------------------------------------------------------------------
signed char eavlLineEdges[1][2] =
{
  {0,1} // 0
};

