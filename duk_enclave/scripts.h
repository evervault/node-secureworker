const size_t MAX_SCRIPT = 26;
const char *SCRIPTS[MAX_SCRIPT] = { "\
// 3D Cube Rotation\n\
// http://www.speich.net/computer/moztesting/3d.htm\n\
// Created by Simon Speich\n\
\n\
var Q = new Array();\n\
var MTrans = new Array();  // transformation matrix\n\
var MQube = new Array();  // position information of qube\n\
var I = new Array();      // entity matrix\n\
var Origin = new Object();\n\
var Testing = new Object();\n\
var LoopTimer;\n\
\n\
var validation = {\n\
 20: 2889.0000000000045,\n\
 40: 2889.0000000000055,\n\
 80: 2889.000000000005,\n\
 160: 2889.0000000000055\n\
};\n\
\n\
var DisplArea = new Object();\n\
DisplArea.Width = 300;\n\
DisplArea.Height = 300;\n\
\n\
function DrawLine(From, To) {\n\
  var x1 = From.V[0];\n\
  var x2 = To.V[0];\n\
  var y1 = From.V[1];\n\
  var y2 = To.V[1];\n\
  var dx = Math.abs(x2 - x1);\n\
  var dy = Math.abs(y2 - y1);\n\
  var x = x1;\n\
  var y = y1;\n\
  var IncX1, IncY1;\n\
  var IncX2, IncY2;  \n\
  var Den;\n\
  var Num;\n\
  var NumAdd;\n\
  var NumPix;\n\
\n\
  if (x2 >= x1) {  IncX1 = 1; IncX2 = 1;  }\n\
  else { IncX1 = -1; IncX2 = -1; }\n\
  if (y2 >= y1)  {  IncY1 = 1; IncY2 = 1; }\n\
  else { IncY1 = -1; IncY2 = -1; }\n\
  if (dx >= dy) {\n\
    IncX1 = 0;\n\
    IncY2 = 0;\n\
    Den = dx;\n\
    Num = dx / 2;\n\
    NumAdd = dy;\n\
    NumPix = dx;\n\
  }\n\
  else {\n\
    IncX2 = 0;\n\
    IncY1 = 0;\n\
    Den = dy;\n\
    Num = dy / 2;\n\
    NumAdd = dx;\n\
    NumPix = dy;\n\
  }\n\
\n\
  NumPix = Math.round(Q.LastPx + NumPix);\n\
\n\
  var i = Q.LastPx;\n\
  for (; i < NumPix; i++) {\n\
    Num += NumAdd;\n\
    if (Num >= Den) {\n\
      Num -= Den;\n\
      x += IncX1;\n\
      y += IncY1;\n\
    }\n\
    x += IncX2;\n\
    y += IncY2;\n\
  }\n\
  Q.LastPx = NumPix;\n\
}\n\
\n\
function CalcCross(V0, V1) {\n\
  var Cross = new Array();\n\
  Cross[0] = V0[1]*V1[2] - V0[2]*V1[1];\n\
  Cross[1] = V0[2]*V1[0] - V0[0]*V1[2];\n\
  Cross[2] = V0[0]*V1[1] - V0[1]*V1[0];\n\
  return Cross;\n\
}\n\
\n\
function CalcNormal(V0, V1, V2) {\n\
  var A = new Array();   var B = new Array(); \n\
  for (var i = 0; i < 3; i++) {\n\
    A[i] = V0[i] - V1[i];\n\
    B[i] = V2[i] - V1[i];\n\
  }\n\
  A = CalcCross(A, B);\n\
  var Length = Math.sqrt(A[0]*A[0] + A[1]*A[1] + A[2]*A[2]); \n\
  for (var i = 0; i < 3; i++) A[i] = A[i] / Length;\n\
  A[3] = 1;\n\
  return A;\n\
}\n\
\n\
function CreateP(X,Y,Z) {\n\
  this.V = [X,Y,Z,1];\n\
}\n\
\n\
// multiplies two matrices\n\
function MMulti(M1, M2) {\n\
  var M = [[],[],[],[]];\n\
  var i = 0;\n\
  var j = 0;\n\
  for (; i < 4; i++) {\n\
    j = 0;\n\
    for (; j < 4; j++) M[i][j] = M1[i][0] * M2[0][j] + M1[i][1] * M2[1][j] + M1[i][2] * M2[2][j] + M1[i][3] * M2[3][j];\n\
  }\n\
  return M;\n\
}\n\
\n\
//multiplies matrix with vector\n\
function VMulti(M, V) {\n\
  var Vect = new Array();\n\
  var i = 0;\n\
  for (;i < 4; i++) Vect[i] = M[i][0] * V[0] + M[i][1] * V[1] + M[i][2] * V[2] + M[i][3] * V[3];\n\
  return Vect;\n\
}\n\
\n\
function VMulti2(M, V) {\n\
  var Vect = new Array();\n\
  var i = 0;\n\
  for (;i < 3; i++) Vect[i] = M[i][0] * V[0] + M[i][1] * V[1] + M[i][2] * V[2];\n\
  return Vect;\n\
}\n\
\n\
// add to matrices\n\
function MAdd(M1, M2) {\n\
  var M = [[],[],[],[]];\n\
  var i = 0;\n\
  var j = 0;\n\
  for (; i < 4; i++) {\n\
    j = 0;\n\
    for (; j < 4; j++) M[i][j] = M1[i][j] + M2[i][j];\n\
  }\n\
  return M;\n\
}\n\
\n\
function Translate(M, Dx, Dy, Dz) {\n\
  var T = [\n\
  [1,0,0,Dx],\n\
  [0,1,0,Dy],\n\
  [0,0,1,Dz],\n\
  [0,0,0,1]\n\
  ];\n\
  return MMulti(T, M);\n\
}\n\
\n\
function RotateX(M, Phi) {\n\
  var a = Phi;\n\
  a *= Math.PI / 180;\n\
  var Cos = Math.cos(a);\n\
  var Sin = Math.sin(a);\n\
  var R = [\n\
  [1,0,0,0],\n\
  [0,Cos,-Sin,0],\n\
  [0,Sin,Cos,0],\n\
  [0,0,0,1]\n\
  ];\n\
  return MMulti(R, M);\n\
}\n\
\n\
function RotateY(M, Phi) {\n\
  var a = Phi;\n\
  a *= Math.PI / 180;\n\
  var Cos = Math.cos(a);\n\
  var Sin = Math.sin(a);\n\
  var R = [\n\
  [Cos,0,Sin,0],\n\
  [0,1,0,0],\n\
  [-Sin,0,Cos,0],\n\
  [0,0,0,1]\n\
  ];\n\
  return MMulti(R, M);\n\
}\n\
\n\
function RotateZ(M, Phi) {\n\
  var a = Phi;\n\
  a *= Math.PI / 180;\n\
  var Cos = Math.cos(a);\n\
  var Sin = Math.sin(a);\n\
  var R = [\n\
  [Cos,-Sin,0,0],\n\
  [Sin,Cos,0,0],\n\
  [0,0,1,0],   \n\
  [0,0,0,1]\n\
  ];\n\
  return MMulti(R, M);\n\
}\n\
\n\
function DrawQube() {\n\
  // calc current normals\n\
  var CurN = new Array();\n\
  var i = 5;\n\
  Q.LastPx = 0;\n\
  for (; i > -1; i--) CurN[i] = VMulti2(MQube, Q.Normal[i]);\n\
  if (CurN[0][2] < 0) {\n\
    if (!Q.Line[0]) { DrawLine(Q[0], Q[1]); Q.Line[0] = true; };\n\
    if (!Q.Line[1]) { DrawLine(Q[1], Q[2]); Q.Line[1] = true; };\n\
    if (!Q.Line[2]) { DrawLine(Q[2], Q[3]); Q.Line[2] = true; };\n\
    if (!Q.Line[3]) { DrawLine(Q[3], Q[0]); Q.Line[3] = true; };\n\
  }\n\
  if (CurN[1][2] < 0) {\n\
    if (!Q.Line[2]) { DrawLine(Q[3], Q[2]); Q.Line[2] = true; };\n\
    if (!Q.Line[9]) { DrawLine(Q[2], Q[6]); Q.Line[9] = true; };\n\
    if (!Q.Line[6]) { DrawLine(Q[6], Q[7]); Q.Line[6] = true; };\n\
    if (!Q.Line[10]) { DrawLine(Q[7], Q[3]); Q.Line[10] = true; };\n\
  }\n\
  if (CurN[2][2] < 0) {\n\
    if (!Q.Line[4]) { DrawLine(Q[4], Q[5]); Q.Line[4] = true; };\n\
    if (!Q.Line[5]) { DrawLine(Q[5], Q[6]); Q.Line[5] = true; };\n\
    if (!Q.Line[6]) { DrawLine(Q[6], Q[7]); Q.Line[6] = true; };\n\
    if (!Q.Line[7]) { DrawLine(Q[7], Q[4]); Q.Line[7] = true; };\n\
  }\n\
  if (CurN[3][2] < 0) {\n\
    if (!Q.Line[4]) { DrawLine(Q[4], Q[5]); Q.Line[4] = true; };\n\
    if (!Q.Line[8]) { DrawLine(Q[5], Q[1]); Q.Line[8] = true; };\n\
    if (!Q.Line[0]) { DrawLine(Q[1], Q[0]); Q.Line[0] = true; };\n\
    if (!Q.Line[11]) { DrawLine(Q[0], Q[4]); Q.Line[11] = true; };\n\
  }\n\
  if (CurN[4][2] < 0) {\n\
    if (!Q.Line[11]) { DrawLine(Q[4], Q[0]); Q.Line[11] = true; };\n\
    if (!Q.Line[3]) { DrawLine(Q[0], Q[3]); Q.Line[3] = true; };\n\
    if (!Q.Line[10]) { DrawLine(Q[3], Q[7]); Q.Line[10] = true; };\n\
    if (!Q.Line[7]) { DrawLine(Q[7], Q[4]); Q.Line[7] = true; };\n\
  }\n\
  if (CurN[5][2] < 0) {\n\
    if (!Q.Line[8]) { DrawLine(Q[1], Q[5]); Q.Line[8] = true; };\n\
    if (!Q.Line[5]) { DrawLine(Q[5], Q[6]); Q.Line[5] = true; };\n\
    if (!Q.Line[9]) { DrawLine(Q[6], Q[2]); Q.Line[9] = true; };\n\
    if (!Q.Line[1]) { DrawLine(Q[2], Q[1]); Q.Line[1] = true; };\n\
  }\n\
  Q.Line = [false,false,false,false,false,false,false,false,false,false,false,false];\n\
  Q.LastPx = 0;\n\
}\n\
\n\
function Loop() {\n\
  if (Testing.LoopCount > Testing.LoopMax) return;\n\
  var TestingStr = String(Testing.LoopCount);\n\
  while (TestingStr.length < 3) TestingStr = \"0\" + TestingStr;\n\
  MTrans = Translate(I, -Q[8].V[0], -Q[8].V[1], -Q[8].V[2]);\n\
  MTrans = RotateX(MTrans, 1);\n\
  MTrans = RotateY(MTrans, 3);\n\
  MTrans = RotateZ(MTrans, 5);\n\
  MTrans = Translate(MTrans, Q[8].V[0], Q[8].V[1], Q[8].V[2]);\n\
  MQube = MMulti(MTrans, MQube);\n\
  var i = 8;\n\
  for (; i > -1; i--) {\n\
    Q[i].V = VMulti(MTrans, Q[i].V);\n\
  }\n\
  DrawQube();\n\
  Testing.LoopCount++;\n\
  Loop();\n\
}\n\
\n\
function Init(CubeSize) {\n\
  // init/reset vars\n\
  Origin.V = [150,150,20,1];\n\
  Testing.LoopCount = 0;\n\
  Testing.LoopMax = 50;\n\
  Testing.TimeMax = 0;\n\
  Testing.TimeAvg = 0;\n\
  Testing.TimeMin = 0;\n\
  Testing.TimeTemp = 0;\n\
  Testing.TimeTotal = 0;\n\
  Testing.Init = false;\n\
\n\
  // transformation matrix\n\
  MTrans = [\n\
  [1,0,0,0],\n\
  [0,1,0,0],\n\
  [0,0,1,0],\n\
  [0,0,0,1]\n\
  ];\n\
  \n\
  // position information of qube\n\
  MQube = [\n\
  [1,0,0,0],\n\
  [0,1,0,0],\n\
  [0,0,1,0],\n\
  [0,0,0,1]\n\
  ];\n\
  \n\
  // entity matrix\n\
  I = [\n\
  [1,0,0,0],\n\
  [0,1,0,0],\n\
  [0,0,1,0],\n\
  [0,0,0,1]\n\
  ];\n\
  \n\
  // create qube\n\
  Q[0] = new CreateP(-CubeSize,-CubeSize, CubeSize);\n\
  Q[1] = new CreateP(-CubeSize, CubeSize, CubeSize);\n\
  Q[2] = new CreateP( CubeSize, CubeSize, CubeSize);\n\
  Q[3] = new CreateP( CubeSize,-CubeSize, CubeSize);\n\
  Q[4] = new CreateP(-CubeSize,-CubeSize,-CubeSize);\n\
  Q[5] = new CreateP(-CubeSize, CubeSize,-CubeSize);\n\
  Q[6] = new CreateP( CubeSize, CubeSize,-CubeSize);\n\
  Q[7] = new CreateP( CubeSize,-CubeSize,-CubeSize);\n\
  \n\
  // center of gravity\n\
  Q[8] = new CreateP(0, 0, 0);\n\
  \n\
  // anti-clockwise edge check\n\
  Q.Edge = [[0,1,2],[3,2,6],[7,6,5],[4,5,1],[4,0,3],[1,5,6]];\n\
  \n\
  // calculate squad normals\n\
  Q.Normal = new Array();\n\
  for (var i = 0; i < Q.Edge.length; i++) Q.Normal[i] = CalcNormal(Q[Q.Edge[i][0]].V, Q[Q.Edge[i][1]].V, Q[Q.Edge[i][2]].V);\n\
  \n\
  // line drawn ?\n\
  Q.Line = [false,false,false,false,false,false,false,false,false,false,false,false];\n\
  \n\
  // create line pixels\n\
  Q.NumPx = 9 * 2 * CubeSize;\n\
  for (var i = 0; i < Q.NumPx; i++) CreateP(0,0,0);\n\
  \n\
  MTrans = Translate(MTrans, Origin.V[0], Origin.V[1], Origin.V[2]);\n\
  MQube = MMulti(MTrans, MQube);\n\
\n\
  var i = 0;\n\
  for (; i < 9; i++) {\n\
    Q[i].V = VMulti(MTrans, Q[i].V);\n\
  }\n\
  DrawQube();\n\
  Testing.Init = true;\n\
  Loop();\n\
  \n\
  // Perform a simple sum-based verification.\n\
  var sum = 0;\n\
  for (var i = 0; i < Q.length; ++i) {\n\
    var vector = Q[i].V;\n\
    for (var j = 0; j < vector.length; ++j)\n\
      sum += vector[j];\n\
  }\n\
  if (sum != validation[CubeSize])\n\
    throw \"Error: bad vector sum for CubeSize = \" + CubeSize + \"; expected \" + validation[CubeSize] + \" but got \" + sum;\n\
}\n\
\n\
for ( var i = 20; i <= 160; i *= 2 ) {\n\
  Init(i);\n\
}\n\
\n\
Q = null;\n\
MTrans = null;\n\
MQube = null;\n\
I = null;\n\
Origin = null;\n\
Testing = null;\n\
LoopTime = null;\n\
DisplArea = null;\n\
\n\
\n\
", "\
/*\n\
 * Copyright (C) 2007 Apple Inc.  All rights reserved.\n\
 *\n\
 * Redistribution and use in source and binary forms, with or without\n\
 * modification, are permitted provided that the following conditions\n\
 * are met:\n\
 * 1. Redistributions of source code must retain the above copyright\n\
 *    notice, this list of conditions and the following disclaimer.\n\
 * 2. Redistributions in binary form must reproduce the above copyright\n\
 *    notice, this list of conditions and the following disclaimer in the\n\
 *    documentation and/or other materials provided with the distribution.\n\
 *\n\
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY\n\
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n\
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR\n\
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR\n\
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,\n\
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,\n\
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR\n\
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY\n\
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\n\
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n\
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. \n\
 */\n\
\n\
var loops = 15\n\
var nx = 120\n\
var nz = 120\n\
\n\
function morph(a, f) {\n\
    var PI2nx = Math.PI * 8/nx\n\
    var sin = Math.sin\n\
    var f30 = -(50 * sin(f*Math.PI*2))\n\
    \n\
    for (var i = 0; i < nz; ++i) {\n\
        for (var j = 0; j < nx; ++j) {\n\
            a[3*(i*nx+j)+1]    = sin((j-1) * PI2nx ) * -f30\n\
        }\n\
    }\n\
}\n\
\n\
    \n\
var a = Array()\n\
for (var i=0; i < nx*nz*3; ++i) \n\
    a[i] = 0\n\
\n\
for (var i = 0; i < loops; ++i) {\n\
    morph(a, i/loops)\n\
}\n\
\n\
testOutput = 0;\n\
for (var i = 0; i < nx; i++)\n\
    testOutput += a[3*(i*nx+i)+1];\n\
a = null;\n\
\n\
// This has to be an approximate test since ECMAscript doesn't formally specify\n\
// what sin() returns. Even if it did specify something like for example what Java 7\n\
// says - that sin() has to return a value within 1 ulp of exact - then we still\n\
// would not be able to do an exact test here since that would allow for just enough\n\
// low-bit slop to create possibly big errors due to testOutput being a sum.\n\
var epsilon = 1e-13;\n\
if (Math.abs(testOutput) >= epsilon)\n\
    throw \"Error: bad test output: expected magnitude below \" + epsilon + \" but got \" + testOutput;\n\
\n\
", "\
/*\n\
 * Copyright (C) 2007 Apple Inc.  All rights reserved.\n\
 *\n\
 * Redistribution and use in source and binary forms, with or without\n\
 * modification, are permitted provided that the following conditions\n\
 * are met:\n\
 * 1. Redistributions of source code must retain the above copyright\n\
 *    notice, this list of conditions and the following disclaimer.\n\
 * 2. Redistributions in binary form must reproduce the above copyright\n\
 *    notice, this list of conditions and the following disclaimer in the\n\
 *    documentation and/or other materials provided with the distribution.\n\
 *\n\
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY\n\
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n\
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR\n\
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR\n\
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,\n\
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,\n\
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR\n\
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY\n\
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\n\
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n\
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. \n\
 */\n\
\n\
function createVector(x,y,z) {\n\
    return new Array(x,y,z);\n\
}\n\
\n\
function sqrLengthVector(self) {\n\
    return self[0] * self[0] + self[1] * self[1] + self[2] * self[2];\n\
}\n\
\n\
function lengthVector(self) {\n\
    return Math.sqrt(self[0] * self[0] + self[1] * self[1] + self[2] * self[2]);\n\
}\n\
\n\
function addVector(self, v) {\n\
    self[0] += v[0];\n\
    self[1] += v[1];\n\
    self[2] += v[2];\n\
    return self;\n\
}\n\
\n\
function subVector(self, v) {\n\
    self[0] -= v[0];\n\
    self[1] -= v[1];\n\
    self[2] -= v[2];\n\
    return self;\n\
}\n\
\n\
function scaleVector(self, scale) {\n\
    self[0] *= scale;\n\
    self[1] *= scale;\n\
    self[2] *= scale;\n\
    return self;\n\
}\n\
\n\
function normaliseVector(self) {\n\
    var len = Math.sqrt(self[0] * self[0] + self[1] * self[1] + self[2] * self[2]);\n\
    self[0] /= len;\n\
    self[1] /= len;\n\
    self[2] /= len;\n\
    return self;\n\
}\n\
\n\
function add(v1, v2) {\n\
    return new Array(v1[0] + v2[0], v1[1] + v2[1], v1[2] + v2[2]);\n\
}\n\
\n\
function sub(v1, v2) {\n\
    return new Array(v1[0] - v2[0], v1[1] - v2[1], v1[2] - v2[2]);\n\
}\n\
\n\
function scalev(v1, v2) {\n\
    return new Array(v1[0] * v2[0], v1[1] * v2[1], v1[2] * v2[2]);\n\
}\n\
\n\
function dot(v1, v2) {\n\
    return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];\n\
}\n\
\n\
function scale(v, scale) {\n\
    return [v[0] * scale, v[1] * scale, v[2] * scale];\n\
}\n\
\n\
function cross(v1, v2) {\n\
    return [v1[1] * v2[2] - v1[2] * v2[1], \n\
            v1[2] * v2[0] - v1[0] * v2[2],\n\
            v1[0] * v2[1] - v1[1] * v2[0]];\n\
\n\
}\n\
\n\
function normalise(v) {\n\
    var len = lengthVector(v);\n\
    return [v[0] / len, v[1] / len, v[2] / len];\n\
}\n\
\n\
function transformMatrix(self, v) {\n\
    var vals = self;\n\
    var x  = vals[0] * v[0] + vals[1] * v[1] + vals[2] * v[2] + vals[3];\n\
    var y  = vals[4] * v[0] + vals[5] * v[1] + vals[6] * v[2] + vals[7];\n\
    var z  = vals[8] * v[0] + vals[9] * v[1] + vals[10] * v[2] + vals[11];\n\
    return [x, y, z];\n\
}\n\
\n\
function invertMatrix(self) {\n\
    var temp = new Array(16);\n\
    var tx = -self[3];\n\
    var ty = -self[7];\n\
    var tz = -self[11];\n\
    for (h = 0; h < 3; h++) \n\
        for (v = 0; v < 3; v++) \n\
            temp[h + v * 4] = self[v + h * 4];\n\
    for (i = 0; i < 11; i++)\n\
        self[i] = temp[i];\n\
    self[3] = tx * self[0] + ty * self[1] + tz * self[2];\n\
    self[7] = tx * self[4] + ty * self[5] + tz * self[6];\n\
    self[11] = tx * self[8] + ty * self[9] + tz * self[10];\n\
    return self;\n\
}\n\
\n\
\n\
// Triangle intersection using barycentric coord method\n\
function Triangle(p1, p2, p3) {\n\
    var edge1 = sub(p3, p1);\n\
    var edge2 = sub(p2, p1);\n\
    var normal = cross(edge1, edge2);\n\
    if (Math.abs(normal[0]) > Math.abs(normal[1]))\n\
        if (Math.abs(normal[0]) > Math.abs(normal[2]))\n\
            this.axis = 0; \n\
        else \n\
            this.axis = 2;\n\
    else\n\
        if (Math.abs(normal[1]) > Math.abs(normal[2])) \n\
            this.axis = 1;\n\
        else \n\
            this.axis = 2;\n\
    var u = (this.axis + 1) % 3;\n\
    var v = (this.axis + 2) % 3;\n\
    var u1 = edge1[u];\n\
    var v1 = edge1[v];\n\
    \n\
    var u2 = edge2[u];\n\
    var v2 = edge2[v];\n\
    this.normal = normalise(normal);\n\
    this.nu = normal[u] / normal[this.axis];\n\
    this.nv = normal[v] / normal[this.axis];\n\
    this.nd = dot(normal, p1) / normal[this.axis];\n\
    var det = u1 * v2 - v1 * u2;\n\
    this.eu = p1[u];\n\
    this.ev = p1[v]; \n\
    this.nu1 = u1 / det;\n\
    this.nv1 = -v1 / det;\n\
    this.nu2 = v2 / det;\n\
    this.nv2 = -u2 / det; \n\
    this.material = [0.7, 0.7, 0.7];\n\
}\n\
\n\
Triangle.prototype.intersect = function(orig, dir, near, far) {\n\
    var u = (this.axis + 1) % 3;\n\
    var v = (this.axis + 2) % 3;\n\
    var d = dir[this.axis] + this.nu * dir[u] + this.nv * dir[v];\n\
    var t = (this.nd - orig[this.axis] - this.nu * orig[u] - this.nv * orig[v]) / d;\n\
    if (t < near || t > far)\n\
        return null;\n\
    var Pu = orig[u] + t * dir[u] - this.eu;\n\
    var Pv = orig[v] + t * dir[v] - this.ev;\n\
    var a2 = Pv * this.nu1 + Pu * this.nv1;\n\
    if (a2 < 0) \n\
        return null;\n\
    var a3 = Pu * this.nu2 + Pv * this.nv2;\n\
    if (a3 < 0) \n\
        return null;\n\
\n\
    if ((a2 + a3) > 1) \n\
        return null;\n\
    return t;\n\
}\n\
\n\
function Scene(a_triangles) {\n\
    this.triangles = a_triangles;\n\
    this.lights = [];\n\
    this.ambient = [0,0,0];\n\
    this.background = [0.8,0.8,1];\n\
}\n\
var zero = new Array(0,0,0);\n\
\n\
Scene.prototype.intersect = function(origin, dir, near, far) {\n\
    var closest = null;\n\
    for (i = 0; i < this.triangles.length; i++) {\n\
        var triangle = this.triangles[i];   \n\
        var d = triangle.intersect(origin, dir, near, far);\n\
        if (d == null || d > far || d < near)\n\
            continue;\n\
        far = d;\n\
        closest = triangle;\n\
    }\n\
    \n\
    if (!closest)\n\
        return [this.background[0],this.background[1],this.background[2]];\n\
        \n\
    var normal = closest.normal;\n\
    var hit = add(origin, scale(dir, far)); \n\
    if (dot(dir, normal) > 0)\n\
        normal = [-normal[0], -normal[1], -normal[2]];\n\
    \n\
    var colour = null;\n\
    if (closest.shader) {\n\
        colour = closest.shader(closest, hit, dir);\n\
    } else {\n\
        colour = closest.material;\n\
    }\n\
    \n\
    // do reflection\n\
    var reflected = null;\n\
    if (colour.reflection > 0.001) {\n\
        var reflection = addVector(scale(normal, -2*dot(dir, normal)), dir);\n\
        reflected = this.intersect(hit, reflection, 0.0001, 1000000);\n\
        if (colour.reflection >= 0.999999)\n\
            return reflected;\n\
    }\n\
    \n\
    var l = [this.ambient[0], this.ambient[1], this.ambient[2]];\n\
    for (var i = 0; i < this.lights.length; i++) {\n\
        var light = this.lights[i];\n\
        var toLight = sub(light, hit);\n\
        var distance = lengthVector(toLight);\n\
        scaleVector(toLight, 1.0/distance);\n\
        distance -= 0.0001;\n\
        if (this.blocked(hit, toLight, distance))\n\
            continue;\n\
        var nl = dot(normal, toLight);\n\
        if (nl > 0)\n\
            addVector(l, scale(light.colour, nl));\n\
    }\n\
    l = scalev(l, colour);\n\
    if (reflected) {\n\
        l = addVector(scaleVector(l, 1 - colour.reflection), scaleVector(reflected, colour.reflection));\n\
    }\n\
    return l;\n\
}\n\
\n\
Scene.prototype.blocked = function(O, D, far) {\n\
    var near = 0.0001;\n\
    var closest = null;\n\
    for (i = 0; i < this.triangles.length; i++) {\n\
        var triangle = this.triangles[i];   \n\
        var d = triangle.intersect(O, D, near, far);\n\
        if (d == null || d > far || d < near)\n\
            continue;\n\
        return true;\n\
    }\n\
    \n\
    return false;\n\
}\n\
\n\
\n\
// this camera code is from notes i made ages ago, it is from *somewhere* -- i cannot remember where\n\
// that somewhere is\n\
function Camera(origin, lookat, up) {\n\
    var zaxis = normaliseVector(subVector(lookat, origin));\n\
    var xaxis = normaliseVector(cross(up, zaxis));\n\
    var yaxis = normaliseVector(cross(xaxis, subVector([0,0,0], zaxis)));\n\
    var m = new Array(16);\n\
    m[0] = xaxis[0]; m[1] = xaxis[1]; m[2] = xaxis[2];\n\
    m[4] = yaxis[0]; m[5] = yaxis[1]; m[6] = yaxis[2];\n\
    m[8] = zaxis[0]; m[9] = zaxis[1]; m[10] = zaxis[2];\n\
    invertMatrix(m);\n\
    m[3] = 0; m[7] = 0; m[11] = 0;\n\
    this.origin = origin;\n\
    this.directions = new Array(4);\n\
    this.directions[0] = normalise([-0.7,  0.7, 1]);\n\
    this.directions[1] = normalise([ 0.7,  0.7, 1]);\n\
    this.directions[2] = normalise([ 0.7, -0.7, 1]);\n\
    this.directions[3] = normalise([-0.7, -0.7, 1]);\n\
    this.directions[0] = transformMatrix(m, this.directions[0]);\n\
    this.directions[1] = transformMatrix(m, this.directions[1]);\n\
    this.directions[2] = transformMatrix(m, this.directions[2]);\n\
    this.directions[3] = transformMatrix(m, this.directions[3]);\n\
}\n\
\n\
Camera.prototype.generateRayPair = function(y) {\n\
    rays = new Array(new Object(), new Object());\n\
    rays[0].origin = this.origin;\n\
    rays[1].origin = this.origin;\n\
    rays[0].dir = addVector(scale(this.directions[0], y), scale(this.directions[3], 1 - y));\n\
    rays[1].dir = addVector(scale(this.directions[1], y), scale(this.directions[2], 1 - y));\n\
    return rays;\n\
}\n\
\n\
function renderRows(camera, scene, pixels, width, height, starty, stopy) {\n\
    for (var y = starty; y < stopy; y++) {\n\
        var rays = camera.generateRayPair(y / height);\n\
        for (var x = 0; x < width; x++) {\n\
            var xp = x / width;\n\
            var origin = addVector(scale(rays[0].origin, xp), scale(rays[1].origin, 1 - xp));\n\
            var dir = normaliseVector(addVector(scale(rays[0].dir, xp), scale(rays[1].dir, 1 - xp)));\n\
            var l = scene.intersect(origin, dir);\n\
            pixels[y][x] = l;\n\
        }\n\
    }\n\
}\n\
\n\
Camera.prototype.render = function(scene, pixels, width, height) {\n\
    var cam = this;\n\
    var row = 0;\n\
    renderRows(cam, scene, pixels, width, height, 0, height);\n\
}\n\
\n\
\n\
\n\
function raytraceScene()\n\
{\n\
    var startDate = new Date().getTime();\n\
    var numTriangles = 2 * 6;\n\
    var triangles = new Array();//numTriangles);\n\
    var tfl = createVector(-10,  10, -10);\n\
    var tfr = createVector( 10,  10, -10);\n\
    var tbl = createVector(-10,  10,  10);\n\
    var tbr = createVector( 10,  10,  10);\n\
    var bfl = createVector(-10, -10, -10);\n\
    var bfr = createVector( 10, -10, -10);\n\
    var bbl = createVector(-10, -10,  10);\n\
    var bbr = createVector( 10, -10,  10);\n\
    \n\
    // cube!!!\n\
    // front\n\
    var i = 0;\n\
    \n\
    triangles[i++] = new Triangle(tfl, tfr, bfr);\n\
    triangles[i++] = new Triangle(tfl, bfr, bfl);\n\
    // back\n\
    triangles[i++] = new Triangle(tbl, tbr, bbr);\n\
    triangles[i++] = new Triangle(tbl, bbr, bbl);\n\
    //        triangles[i-1].material = [0.7,0.2,0.2];\n\
    //            triangles[i-1].material.reflection = 0.8;\n\
    // left\n\
    triangles[i++] = new Triangle(tbl, tfl, bbl);\n\
    //            triangles[i-1].reflection = 0.6;\n\
    triangles[i++] = new Triangle(tfl, bfl, bbl);\n\
    //            triangles[i-1].reflection = 0.6;\n\
    // right\n\
    triangles[i++] = new Triangle(tbr, tfr, bbr);\n\
    triangles[i++] = new Triangle(tfr, bfr, bbr);\n\
    // top\n\
    triangles[i++] = new Triangle(tbl, tbr, tfr);\n\
    triangles[i++] = new Triangle(tbl, tfr, tfl);\n\
    // bottom\n\
    triangles[i++] = new Triangle(bbl, bbr, bfr);\n\
    triangles[i++] = new Triangle(bbl, bfr, bfl);\n\
    \n\
    //Floor!!!!\n\
    var green = createVector(0.0, 0.4, 0.0);\n\
    var grey = createVector(0.4, 0.4, 0.4);\n\
    grey.reflection = 1.0;\n\
    var floorShader = function(tri, pos, view) {\n\
        var x = ((pos[0]/32) % 2 + 2) % 2;\n\
        var z = ((pos[2]/32 + 0.3) % 2 + 2) % 2;\n\
        if (x < 1 != z < 1) {\n\
            //in the real world we use the fresnel term...\n\
            //    var angle = 1-dot(view, tri.normal);\n\
            //   angle *= angle;\n\
            //  angle *= angle;\n\
            // angle *= angle;\n\
            //grey.reflection = angle;\n\
            return grey;\n\
        } else \n\
            return green;\n\
    }\n\
    var ffl = createVector(-1000, -30, -1000);\n\
    var ffr = createVector( 1000, -30, -1000);\n\
    var fbl = createVector(-1000, -30,  1000);\n\
    var fbr = createVector( 1000, -30,  1000);\n\
    triangles[i++] = new Triangle(fbl, fbr, ffr);\n\
    triangles[i-1].shader = floorShader;\n\
    triangles[i++] = new Triangle(fbl, ffr, ffl);\n\
    triangles[i-1].shader = floorShader;\n\
    \n\
    var _scene = new Scene(triangles);\n\
    _scene.lights[0] = createVector(20, 38, -22);\n\
    _scene.lights[0].colour = createVector(0.7, 0.3, 0.3);\n\
    _scene.lights[1] = createVector(-23, 40, 17);\n\
    _scene.lights[1].colour = createVector(0.7, 0.3, 0.3);\n\
    _scene.lights[2] = createVector(23, 20, 17);\n\
    _scene.lights[2].colour = createVector(0.7, 0.7, 0.7);\n\
    _scene.ambient = createVector(0.1, 0.1, 0.1);\n\
    //  _scene.background = createVector(0.7, 0.7, 1.0);\n\
    \n\
    var size = 30;\n\
    var pixels = new Array();\n\
    for (var y = 0; y < size; y++) {\n\
        pixels[y] = new Array();\n\
        for (var x = 0; x < size; x++) {\n\
            pixels[y][x] = 0;\n\
        }\n\
    }\n\
\n\
    var _camera = new Camera(createVector(-40, 40, 40), createVector(0, 0, 0), createVector(0, 1, 0));\n\
    _camera.render(_scene, pixels, size, size);\n\
\n\
    return pixels;\n\
}\n\
\n\
function arrayToCanvasCommands(pixels)\n\
{\n\
    var s = '<canvas id=\"renderCanvas\" width=\"30px\" height=\"30px\"></canvas><scr' + 'ipt>\\nvar pixels = [';\n\
    var size = 30;\n\
    for (var y = 0; y < size; y++) {\n\
        s += \"[\";\n\
        for (var x = 0; x < size; x++) {\n\
            s += \"[\" + pixels[y][x] + \"],\";\n\
        }\n\
        s+= \"],\";\n\
    }\n\
    s += '];\\n    var canvas = document.getElementById(\"renderCanvas\").getContext(\"2d\");\\n\\\n\
\\n\\\n\
\\n\\\n\
    var size = 30;\\n\\\n\
    canvas.fillStyle = \"red\";\\n\\\n\
    canvas.fillRect(0, 0, size, size);\\n\\\n\
    canvas.scale(1, -1);\\n\\\n\
    canvas.translate(0, -size);\\n\\\n\
\\n\\\n\
    if (!canvas.setFillColor)\\n\\\n\
        canvas.setFillColor = function(r, g, b, a) {\\n\\\n\
            this.fillStyle = \"rgb(\"+[Math.floor(r * 255), Math.floor(g * 255), Math.floor(b * 255)]+\")\";\\n\\\n\
    }\\n\\\n\
\\n\\\n\
for (var y = 0; y < size; y++) {\\n\\\n\
  for (var x = 0; x < size; x++) {\\n\\\n\
    var l = pixels[y][x];\\n\\\n\
    canvas.setFillColor(l[0], l[1], l[2], 1);\\n\\\n\
    canvas.fillRect(x, y, 1, 1);\\n\\\n\
  }\\n\\\n\
}</scr' + 'ipt>';\n\
\n\
    return s;\n\
}\n\
\n\
testOutput = arrayToCanvasCommands(raytraceScene());\n\
\n\
var expectedLength = 20970;\n\
\n\
if (testOutput.length != expectedLength)\n\
    throw \"Error: bad result: expected length \" + expectedLength + \" but got \" + testOutput.length;\n\
\n\
\n\
", "\
/* The Great Computer Language Shootout\n\
   http://shootout.alioth.debian.org/\n\
   contributed by Isaac Gouy */\n\
\n\
function TreeNode(left,right,item){\n\
   this.left = left;\n\
   this.right = right;\n\
   this.item = item;\n\
}\n\
\n\
TreeNode.prototype.itemCheck = function(){\n\
   if (this.left==null) return this.item;\n\
   else return this.item + this.left.itemCheck() - this.right.itemCheck();\n\
}\n\
\n\
function bottomUpTree(item,depth){\n\
   if (depth>0){\n\
      return new TreeNode(\n\
          bottomUpTree(2*item-1, depth-1)\n\
         ,bottomUpTree(2*item, depth-1)\n\
         ,item\n\
      );\n\
   }\n\
   else {\n\
      return new TreeNode(null,null,item);\n\
   }\n\
}\n\
\n\
var ret = 0;\n\
\n\
for ( var n = 4; n <= 7; n += 1 ) {\n\
    var minDepth = 4;\n\
    var maxDepth = Math.max(minDepth + 2, n);\n\
    var stretchDepth = maxDepth + 1;\n\
    \n\
    var check = bottomUpTree(0,stretchDepth).itemCheck();\n\
    \n\
    var longLivedTree = bottomUpTree(0,maxDepth);\n\
    for (var depth=minDepth; depth<=maxDepth; depth+=2){\n\
        var iterations = 1 << (maxDepth - depth + minDepth);\n\
\n\
        check = 0;\n\
        for (var i=1; i<=iterations; i++){\n\
            check += bottomUpTree(i,depth).itemCheck();\n\
            check += bottomUpTree(-i,depth).itemCheck();\n\
        }\n\
    }\n\
\n\
    ret += longLivedTree.itemCheck();\n\
}\n\
\n\
var expected = -4;\n\
if (ret != expected)\n\
    throw \"ERROR: bad result: expected \" + expected + \" but got \" + ret;\n\
", "\
/* The Great Computer Language Shootout\n\
   http://shootout.alioth.debian.org/\n\
   contributed by Isaac Gouy */\n\
\n\
function fannkuch(n) {\n\
   var check = 0;\n\
   var perm = Array(n);\n\
   var perm1 = Array(n);\n\
   var count = Array(n);\n\
   var maxPerm = Array(n);\n\
   var maxFlipsCount = 0;\n\
   var m = n - 1;\n\
\n\
   for (var i = 0; i < n; i++) perm1[i] = i;\n\
   var r = n;\n\
\n\
   while (true) {\n\
      // write-out the first 30 permutations\n\
      if (check < 30){\n\
         var s = \"\";\n\
         for(var i=0; i<n; i++) s += (perm1[i]+1).toString();\n\
         check++;\n\
      }\n\
\n\
      while (r != 1) { count[r - 1] = r; r--; }\n\
      if (!(perm1[0] == 0 || perm1[m] == m)) {\n\
         for (var i = 0; i < n; i++) perm[i] = perm1[i];\n\
\n\
         var flipsCount = 0;\n\
         var k;\n\
\n\
         while (!((k = perm[0]) == 0)) {\n\
            var k2 = (k + 1) >> 1;\n\
            for (var i = 0; i < k2; i++) {\n\
               var temp = perm[i]; perm[i] = perm[k - i]; perm[k - i] = temp;\n\
            }\n\
            flipsCount++;\n\
         }\n\
\n\
         if (flipsCount > maxFlipsCount) {\n\
            maxFlipsCount = flipsCount;\n\
            for (var i = 0; i < n; i++) maxPerm[i] = perm1[i];\n\
         }\n\
      }\n\
\n\
      while (true) {\n\
         if (r == n) return maxFlipsCount;\n\
         var perm0 = perm1[0];\n\
         var i = 0;\n\
         while (i < r) {\n\
            var j = i + 1;\n\
            perm1[i] = perm1[j];\n\
            i = j;\n\
         }\n\
         perm1[r] = perm0;\n\
\n\
         count[r] = count[r] - 1;\n\
         if (count[r] > 0) break;\n\
         r++;\n\
      }\n\
   }\n\
}\n\
\n\
var n = 8;\n\
var ret = fannkuch(n);\n\
\n\
var expected = 22;\n\
if (ret != expected)\n\
    throw \"ERROR: bad result: expected \" + expected + \" but got \" + ret;\n\
\n\
\n\
", "\
/* The Great Computer Language Shootout\n\
   http://shootout.alioth.debian.org/\n\
   contributed by Isaac Gouy */\n\
\n\
var PI = 3.141592653589793;\n\
var SOLAR_MASS = 4 * PI * PI;\n\
var DAYS_PER_YEAR = 365.24;\n\
\n\
function Body(x,y,z,vx,vy,vz,mass){\n\
   this.x = x;\n\
   this.y = y;\n\
   this.z = z;\n\
   this.vx = vx;\n\
   this.vy = vy;\n\
   this.vz = vz;\n\
   this.mass = mass;\n\
}\n\
\n\
Body.prototype.offsetMomentum = function(px,py,pz) {\n\
   this.vx = -px / SOLAR_MASS;\n\
   this.vy = -py / SOLAR_MASS;\n\
   this.vz = -pz / SOLAR_MASS;\n\
   return this;\n\
}\n\
\n\
function Jupiter(){\n\
   return new Body(\n\
      4.84143144246472090e+00,\n\
      -1.16032004402742839e+00,\n\
      -1.03622044471123109e-01,\n\
      1.66007664274403694e-03 * DAYS_PER_YEAR,\n\
      7.69901118419740425e-03 * DAYS_PER_YEAR,\n\
      -6.90460016972063023e-05 * DAYS_PER_YEAR,\n\
      9.54791938424326609e-04 * SOLAR_MASS\n\
   );\n\
}\n\
\n\
function Saturn(){\n\
   return new Body(\n\
      8.34336671824457987e+00,\n\
      4.12479856412430479e+00,\n\
      -4.03523417114321381e-01,\n\
      -2.76742510726862411e-03 * DAYS_PER_YEAR,\n\
      4.99852801234917238e-03 * DAYS_PER_YEAR,\n\
      2.30417297573763929e-05 * DAYS_PER_YEAR,\n\
      2.85885980666130812e-04 * SOLAR_MASS\n\
   );\n\
}\n\
\n\
function Uranus(){\n\
   return new Body(\n\
      1.28943695621391310e+01,\n\
      -1.51111514016986312e+01,\n\
      -2.23307578892655734e-01,\n\
      2.96460137564761618e-03 * DAYS_PER_YEAR,\n\
      2.37847173959480950e-03 * DAYS_PER_YEAR,\n\
      -2.96589568540237556e-05 * DAYS_PER_YEAR,\n\
      4.36624404335156298e-05 * SOLAR_MASS\n\
   );\n\
}\n\
\n\
function Neptune(){\n\
   return new Body(\n\
      1.53796971148509165e+01,\n\
      -2.59193146099879641e+01,\n\
      1.79258772950371181e-01,\n\
      2.68067772490389322e-03 * DAYS_PER_YEAR,\n\
      1.62824170038242295e-03 * DAYS_PER_YEAR,\n\
      -9.51592254519715870e-05 * DAYS_PER_YEAR,\n\
      5.15138902046611451e-05 * SOLAR_MASS\n\
   );\n\
}\n\
\n\
function Sun(){\n\
   return new Body(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, SOLAR_MASS);\n\
}\n\
\n\
\n\
function NBodySystem(bodies){\n\
   this.bodies = bodies;\n\
   var px = 0.0;\n\
   var py = 0.0;\n\
   var pz = 0.0;\n\
   var size = this.bodies.length;\n\
   for (var i=0; i<size; i++){\n\
      var b = this.bodies[i];\n\
      var m = b.mass;\n\
      px += b.vx * m;\n\
      py += b.vy * m;\n\
      pz += b.vz * m;\n\
   }\n\
   this.bodies[0].offsetMomentum(px,py,pz);\n\
}\n\
\n\
NBodySystem.prototype.advance = function(dt){\n\
   var dx, dy, dz, distance, mag;\n\
   var size = this.bodies.length;\n\
\n\
   for (var i=0; i<size; i++) {\n\
      var bodyi = this.bodies[i];\n\
      for (var j=i+1; j<size; j++) {\n\
         var bodyj = this.bodies[j];\n\
         dx = bodyi.x - bodyj.x;\n\
         dy = bodyi.y - bodyj.y;\n\
         dz = bodyi.z - bodyj.z;\n\
\n\
         distance = Math.sqrt(dx*dx + dy*dy + dz*dz);\n\
         mag = dt / (distance * distance * distance);\n\
\n\
         bodyi.vx -= dx * bodyj.mass * mag;\n\
         bodyi.vy -= dy * bodyj.mass * mag;\n\
         bodyi.vz -= dz * bodyj.mass * mag;\n\
\n\
         bodyj.vx += dx * bodyi.mass * mag;\n\
         bodyj.vy += dy * bodyi.mass * mag;\n\
         bodyj.vz += dz * bodyi.mass * mag;\n\
      }\n\
   }\n\
\n\
   for (var i=0; i<size; i++) {\n\
      var body = this.bodies[i];\n\
      body.x += dt * body.vx;\n\
      body.y += dt * body.vy;\n\
      body.z += dt * body.vz;\n\
   }\n\
}\n\
\n\
NBodySystem.prototype.energy = function(){\n\
   var dx, dy, dz, distance;\n\
   var e = 0.0;\n\
   var size = this.bodies.length;\n\
\n\
   for (var i=0; i<size; i++) {\n\
      var bodyi = this.bodies[i];\n\
\n\
      e += 0.5 * bodyi.mass *\n\
         ( bodyi.vx * bodyi.vx\n\
         + bodyi.vy * bodyi.vy\n\
         + bodyi.vz * bodyi.vz );\n\
\n\
      for (var j=i+1; j<size; j++) {\n\
         var bodyj = this.bodies[j];\n\
         dx = bodyi.x - bodyj.x;\n\
         dy = bodyi.y - bodyj.y;\n\
         dz = bodyi.z - bodyj.z;\n\
\n\
         distance = Math.sqrt(dx*dx + dy*dy + dz*dz);\n\
         e -= (bodyi.mass * bodyj.mass) / distance;\n\
      }\n\
   }\n\
   return e;\n\
}\n\
\n\
var ret = 0;\n\
\n\
for ( var n = 3; n <= 24; n *= 2 ) {\n\
    (function(){\n\
        var bodies = new NBodySystem( Array(\n\
           Sun(),Jupiter(),Saturn(),Uranus(),Neptune()\n\
        ));\n\
        var max = n * 100;\n\
        \n\
        ret += bodies.energy();\n\
        for (var i=0; i<max; i++){\n\
            bodies.advance(0.01);\n\
        }\n\
        ret += bodies.energy();\n\
    })();\n\
}\n\
\n\
var expected = -1.3524862408537381;\n\
if (ret != expected)\n\
    throw \"ERROR: bad result: expected \" + expected + \" but got \" + ret;\n\
\n\
\n\
", "\
// The Great Computer Language Shootout\n\
// http://shootout.alioth.debian.org/\n\
//\n\
// modified by Isaac Gouy\n\
\n\
function pad(number,width){\n\
   var s = number.toString();\n\
   var prefixWidth = width - s.length;\n\
   if (prefixWidth>0){\n\
      for (var i=1; i<=prefixWidth; i++) s = \" \" + s;\n\
   }\n\
   return s;\n\
}\n\
\n\
function nsieve(m, isPrime){\n\
   var i, k, count;\n\
\n\
   for (i=2; i<=m; i++) { isPrime[i] = true; }\n\
   count = 0;\n\
\n\
   for (i=2; i<=m; i++){\n\
      if (isPrime[i]) {\n\
         for (k=i+i; k<=m; k+=i) isPrime[k] = false;\n\
         count++;\n\
      }\n\
   }\n\
   return count;\n\
}\n\
\n\
function sieve() {\n\
    var sum = 0;\n\
    for (var i = 1; i <= 3; i++ ) {\n\
        var m = (1<<i)*10000;\n\
        var flags = Array(m+1);\n\
        sum += nsieve(m, flags);\n\
    }\n\
    return sum;\n\
}\n\
\n\
var result = sieve();\n\
\n\
var expected = 14302;\n\
if (result != expected)\n\
    throw \"ERROR: bad result: expected \" + expected + \" but got \" + result;\n\
\n\
\n\
\n\
", "\
// Copyright (c) 2004 by Arthur Langereis (arthur_ext at domain xfinitegames, tld com\n\
\n\
var result = 0;\n\
\n\
// 1 op = 6 ANDs, 3 SHRs, 3 SHLs, 4 assigns, 2 ADDs\n\
// O(1)\n\
function fast3bitlookup(b) {\n\
var c, bi3b = 0xE994; // 0b1110 1001 1001 0100; // 3 2 2 1  2 1 1 0\n\
c  = 3 & (bi3b >> ((b << 1) & 14));\n\
c += 3 & (bi3b >> ((b >> 2) & 14));\n\
c += 3 & (bi3b >> ((b >> 5) & 6));\n\
return c;\n\
\n\
/*\n\
lir4,0xE994; 9 instructions, no memory access, minimal register dependence, 6 shifts, 2 adds, 1 inline assign\n\
rlwinmr5,r3,1,28,30\n\
rlwinmr6,r3,30,28,30\n\
rlwinmr7,r3,27,29,30\n\
rlwnmr8,r4,r5,30,31\n\
rlwnmr9,r4,r6,30,31\n\
rlwnmr10,r4,r7,30,31\n\
addr3,r8,r9\n\
addr3,r3,r10\n\
*/\n\
}\n\
\n\
\n\
function TimeFunc(func) {\n\
var x, y, t;\n\
var sum = 0;\n\
for(var x=0; x<500; x++)\n\
for(var y=0; y<256; y++) sum += func(y);\n\
return sum;\n\
}\n\
\n\
sum = TimeFunc(fast3bitlookup);\n\
\n\
var expected = 512000;\n\
if (sum != expected)\n\
    throw \"ERROR: bad result: expected \" + expected + \" but got \" + sum;\n\
\n\
", "\
// Copyright (c) 2004 by Arthur Langereis (arthur_ext at domain xfinitegames, tld com)\n\
\n\
\n\
var result = 0;\n\
\n\
// 1 op = 2 assigns, 16 compare/branches, 8 ANDs, (0-8) ADDs, 8 SHLs\n\
// O(n)\n\
function bitsinbyte(b) {\n\
var m = 1, c = 0;\n\
while(m<0x100) {\n\
if(b & m) c++;\n\
m <<= 1;\n\
}\n\
return c;\n\
}\n\
\n\
function TimeFunc(func) {\n\
var x, y, t;\n\
var sum = 0;\n\
for(var x=0; x<350; x++)\n\
for(var y=0; y<256; y++) sum += func(y);\n\
return sum;\n\
}\n\
\n\
result = TimeFunc(bitsinbyte);\n\
\n\
var expected = 358400;\n\
if (result != expected)\n\
    throw \"ERROR: bad result: expected \" + expected + \" but got \" + result;\n\
\n\
\n\
", "\
/*\n\
 * Copyright (C) 2007 Apple Inc.  All rights reserved.\n\
 *\n\
 * Redistribution and use in source and binary forms, with or without\n\
 * modification, are permitted provided that the following conditions\n\
 * are met:\n\
 * 1. Redistributions of source code must retain the above copyright\n\
 *    notice, this list of conditions and the following disclaimer.\n\
 * 2. Redistributions in binary form must reproduce the above copyright\n\
 *    notice, this list of conditions and the following disclaimer in the\n\
 *    documentation and/or other materials provided with the distribution.\n\
 *\n\
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY\n\
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n\
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR\n\
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR\n\
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,\n\
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,\n\
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR\n\
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY\n\
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\n\
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n\
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. \n\
 */\n\
\n\
bitwiseAndValue = 4294967296;\n\
for (var i = 0; i < 600000; i++)\n\
    bitwiseAndValue = bitwiseAndValue & i;\n\
\n\
var result = bitwiseAndValue;\n\
\n\
var expected = 0;\n\
if (result != expected)\n\
    throw \"ERROR: bad result: expected \" + expected + \" but got \" + result;\n\
\n\
\n\
", "\
// The Great Computer Language Shootout\n\
//  http://shootout.alioth.debian.org\n\
//\n\
//  Contributed by Ian Osgood\n\
\n\
function pad(n,width) {\n\
  var s = n.toString();\n\
  while (s.length < width) s = ' ' + s;\n\
  return s;\n\
}\n\
\n\
function primes(isPrime, n) {\n\
  var i, count = 0, m = 10000<<n, size = m+31>>5;\n\
\n\
  for (i=0; i<size; i++) isPrime[i] = 0xffffffff;\n\
\n\
  for (i=2; i<m; i++)\n\
    if (isPrime[i>>5] & 1<<(i&31)) {\n\
      for (var j=i+i; j<m; j+=i)\n\
        isPrime[j>>5] &= ~(1<<(j&31));\n\
      count++;\n\
    }\n\
}\n\
\n\
function sieve() {\n\
    for (var i = 4; i <= 4; i++) {\n\
        var isPrime = new Array((10000<<i)+31>>5);\n\
        primes(isPrime, i);\n\
    }\n\
    return isPrime;\n\
}\n\
\n\
var result = sieve();\n\
\n\
var sum = 0;\n\
for (var i = 0; i < result.length; ++i)\n\
    sum += result[i];\n\
\n\
var expected = -1286749544853;\n\
if (sum != expected)\n\
    throw \"ERROR: bad result: expected \" + expected + \" but got \" + sum;\n\
\n\
\n\
", "\
// The Computer Language Shootout\n\
// http://shootout.alioth.debian.org/\n\
// contributed by Isaac Gouy\n\
\n\
function ack(m,n){\n\
   if (m==0) { return n+1; }\n\
   if (n==0) { return ack(m-1,1); }\n\
   return ack(m-1, ack(m,n-1) );\n\
}\n\
\n\
function fib(n) {\n\
    if (n < 2){ return 1; }\n\
    return fib(n-2) + fib(n-1);\n\
}\n\
\n\
function tak(x,y,z) {\n\
    if (y >= x) return z;\n\
    return tak(tak(x-1,y,z), tak(y-1,z,x), tak(z-1,x,y));\n\
}\n\
\n\
var result = 0;\n\
\n\
for ( var i = 3; i <= 5; i++ ) {\n\
    result += ack(3,i);\n\
    result += fib(17.0+i);\n\
    result += tak(3*i+3,2*i+2,i+1);\n\
}\n\
\n\
var expected = 57775;\n\
if (result != expected)\n\
    throw \"ERROR: bad result: expected \" + expected + \" but got \" + result;\n\
\n\
\n\
", "\
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */\n\
\n\
/*\n\
 * AES Cipher function: encrypt 'input' with Rijndael algorithm\n\
 *\n\
 *   takes   byte-array 'input' (16 bytes)\n\
 *           2D byte-array key schedule 'w' (Nr+1 x Nb bytes)\n\
 *\n\
 *   applies Nr rounds (10/12/14) using key schedule w for 'add round key' stage\n\
 *\n\
 *   returns byte-array encrypted value (16 bytes)\n\
 */\n\
function Cipher(input, w) {    // main Cipher function [Section 5.1]\n\
  var Nb = 4;               // block size (in words): no of columns in state (fixed at 4 for AES)\n\
  var Nr = w.length/Nb - 1; // no of rounds: 10/12/14 for 128/192/256-bit keys\n\
\n\
  var state = [[],[],[],[]];  // initialise 4xNb byte-array 'state' with input [Section 3.4]\n\
  for (var i=0; i<4*Nb; i++) state[i%4][Math.floor(i/4)] = input[i];\n\
\n\
  state = AddRoundKey(state, w, 0, Nb);\n\
\n\
  for (var round=1; round<Nr; round++) {\n\
    state = SubBytes(state, Nb);\n\
    state = ShiftRows(state, Nb);\n\
    state = MixColumns(state, Nb);\n\
    state = AddRoundKey(state, w, round, Nb);\n\
  }\n\
\n\
  state = SubBytes(state, Nb);\n\
  state = ShiftRows(state, Nb);\n\
  state = AddRoundKey(state, w, Nr, Nb);\n\
\n\
  var output = new Array(4*Nb);  // convert state to 1-d array before returning [Section 3.4]\n\
  for (var i=0; i<4*Nb; i++) output[i] = state[i%4][Math.floor(i/4)];\n\
  return output;\n\
}\n\
\n\
\n\
function SubBytes(s, Nb) {    // apply SBox to state S [Section 5.1.1]\n\
  for (var r=0; r<4; r++) {\n\
    for (var c=0; c<Nb; c++) s[r][c] = Sbox[s[r][c]];\n\
  }\n\
  return s;\n\
}\n\
\n\
\n\
function ShiftRows(s, Nb) {    // shift row r of state S left by r bytes [Section 5.1.2]\n\
  var t = new Array(4);\n\
  for (var r=1; r<4; r++) {\n\
    for (var c=0; c<4; c++) t[c] = s[r][(c+r)%Nb];  // shift into temp copy\n\
    for (var c=0; c<4; c++) s[r][c] = t[c];         // and copy back\n\
  }          // note that this will work for Nb=4,5,6, but not 7,8 (always 4 for AES):\n\
  return s;  // see fp.gladman.plus.com/cryptography_technology/rijndael/aes.spec.311.pdf \n\
}\n\
\n\
\n\
function MixColumns(s, Nb) {   // combine bytes of each col of state S [Section 5.1.3]\n\
  for (var c=0; c<4; c++) {\n\
    var a = new Array(4);  // 'a' is a copy of the current column from 's'\n\
    var b = new Array(4);  // 'b' is a dot {02} in GF(2^8)\n\
    for (var i=0; i<4; i++) {\n\
      a[i] = s[i][c];\n\
      b[i] = s[i][c]&0x80 ? s[i][c]<<1 ^ 0x011b : s[i][c]<<1;\n\
    }\n\
    // a[n] ^ b[n] is a dot {03} in GF(2^8)\n\
    s[0][c] = b[0] ^ a[1] ^ b[1] ^ a[2] ^ a[3]; // 2*a0 + 3*a1 + a2 + a3\n\
    s[1][c] = a[0] ^ b[1] ^ a[2] ^ b[2] ^ a[3]; // a0 * 2*a1 + 3*a2 + a3\n\
    s[2][c] = a[0] ^ a[1] ^ b[2] ^ a[3] ^ b[3]; // a0 + a1 + 2*a2 + 3*a3\n\
    s[3][c] = a[0] ^ b[0] ^ a[1] ^ a[2] ^ b[3]; // 3*a0 + a1 + a2 + 2*a3\n\
  }\n\
  return s;\n\
}\n\
\n\
\n\
function AddRoundKey(state, w, rnd, Nb) {  // xor Round Key into state S [Section 5.1.4]\n\
  for (var r=0; r<4; r++) {\n\
    for (var c=0; c<Nb; c++) state[r][c] ^= w[rnd*4+c][r];\n\
  }\n\
  return state;\n\
}\n\
\n\
\n\
function KeyExpansion(key) {  // generate Key Schedule (byte-array Nr+1 x Nb) from Key [Section 5.2]\n\
  var Nb = 4;            // block size (in words): no of columns in state (fixed at 4 for AES)\n\
  var Nk = key.length/4  // key length (in words): 4/6/8 for 128/192/256-bit keys\n\
  var Nr = Nk + 6;       // no of rounds: 10/12/14 for 128/192/256-bit keys\n\
\n\
  var w = new Array(Nb*(Nr+1));\n\
  var temp = new Array(4);\n\
\n\
  for (var i=0; i<Nk; i++) {\n\
    var r = [key[4*i], key[4*i+1], key[4*i+2], key[4*i+3]];\n\
    w[i] = r;\n\
  }\n\
\n\
  for (var i=Nk; i<(Nb*(Nr+1)); i++) {\n\
    w[i] = new Array(4);\n\
    for (var t=0; t<4; t++) temp[t] = w[i-1][t];\n\
    if (i % Nk == 0) {\n\
      temp = SubWord(RotWord(temp));\n\
      for (var t=0; t<4; t++) temp[t] ^= Rcon[i/Nk][t];\n\
    } else if (Nk > 6 && i%Nk == 4) {\n\
      temp = SubWord(temp);\n\
    }\n\
    for (var t=0; t<4; t++) w[i][t] = w[i-Nk][t] ^ temp[t];\n\
  }\n\
\n\
  return w;\n\
}\n\
\n\
function SubWord(w) {    // apply SBox to 4-byte word w\n\
  for (var i=0; i<4; i++) w[i] = Sbox[w[i]];\n\
  return w;\n\
}\n\
\n\
function RotWord(w) {    // rotate 4-byte word w left by one byte\n\
  w[4] = w[0];\n\
  for (var i=0; i<4; i++) w[i] = w[i+1];\n\
  return w;\n\
}\n\
\n\
\n\
// Sbox is pre-computed multiplicative inverse in GF(2^8) used in SubBytes and KeyExpansion [Section 5.1.1]\n\
var Sbox =  [0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,\n\
             0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,\n\
             0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,\n\
             0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,\n\
             0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,\n\
             0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,\n\
             0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,\n\
             0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,\n\
             0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,\n\
             0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,\n\
             0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,\n\
             0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,\n\
             0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,\n\
             0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,\n\
             0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,\n\
             0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16];\n\
\n\
// Rcon is Round Constant used for the Key Expansion [1st col is 2^(r-1) in GF(2^8)] [Section 5.2]\n\
var Rcon = [ [0x00, 0x00, 0x00, 0x00],\n\
             [0x01, 0x00, 0x00, 0x00],\n\
             [0x02, 0x00, 0x00, 0x00],\n\
             [0x04, 0x00, 0x00, 0x00],\n\
             [0x08, 0x00, 0x00, 0x00],\n\
             [0x10, 0x00, 0x00, 0x00],\n\
             [0x20, 0x00, 0x00, 0x00],\n\
             [0x40, 0x00, 0x00, 0x00],\n\
             [0x80, 0x00, 0x00, 0x00],\n\
             [0x1b, 0x00, 0x00, 0x00],\n\
             [0x36, 0x00, 0x00, 0x00] ]; \n\
\n\
\n\
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */\n\
\n\
/* \n\
 * Use AES to encrypt 'plaintext' with 'password' using 'nBits' key, in 'Counter' mode of operation\n\
 *                           - see http://csrc.nist.gov/publications/nistpubs/800-38a/sp800-38a.pdf\n\
 *   for each block\n\
 *   - outputblock = cipher(counter, key)\n\
 *   - cipherblock = plaintext xor outputblock\n\
 */\n\
function AESEncryptCtr(plaintext, password, nBits) {\n\
  if (!(nBits==128 || nBits==192 || nBits==256)) return '';  // standard allows 128/192/256 bit keys\n\
\n\
  // for this example script, generate the key by applying Cipher to 1st 16/24/32 chars of password; \n\
  // for real-world applications, a more secure approach would be to hash the password e.g. with SHA-1\n\
  var nBytes = nBits/8;  // no bytes in key\n\
  var pwBytes = new Array(nBytes);\n\
  for (var i=0; i<nBytes; i++) pwBytes[i] = password.charCodeAt(i) & 0xff;\n\
  var key = Cipher(pwBytes, KeyExpansion(pwBytes));\n\
  key = key.concat(key.slice(0, nBytes-16));  // key is now 16/24/32 bytes long\n\
\n\
  // initialise counter block (NIST SP800-38A Section B.2): millisecond time-stamp for nonce in 1st 8 bytes,\n\
  // block counter in 2nd 8 bytes\n\
  var blockSize = 16;  // block size fixed at 16 bytes / 128 bits (Nb=4) for AES\n\
  var counterBlock = new Array(blockSize);  // block size fixed at 16 bytes / 128 bits (Nb=4) for AES\n\
  var nonce = (new Date()).getTime();  // milliseconds since 1-Jan-1970\n\
\n\
  // encode nonce in two stages to cater for JavaScript 32-bit limit on bitwise ops\n\
  for (var i=0; i<4; i++) counterBlock[i] = (nonce >>> i*8) & 0xff;\n\
  for (var i=0; i<4; i++) counterBlock[i+4] = (nonce/0x100000000 >>> i*8) & 0xff; \n\
\n\
  // generate key schedule - an expansion of the key into distinct Key Rounds for each round\n\
  var keySchedule = KeyExpansion(key);\n\
\n\
  var blockCount = Math.ceil(plaintext.length/blockSize);\n\
  var ciphertext = new Array(blockCount);  // ciphertext as array of strings\n\
  \n\
  for (var b=0; b<blockCount; b++) {\n\
    // set counter (block #) in last 8 bytes of counter block (leaving nonce in 1st 8 bytes)\n\
    // again done in two stages for 32-bit ops\n\
    for (var c=0; c<4; c++) counterBlock[15-c] = (b >>> c*8) & 0xff;\n\
    for (var c=0; c<4; c++) counterBlock[15-c-4] = (b/0x100000000 >>> c*8)\n\
\n\
    var cipherCntr = Cipher(counterBlock, keySchedule);  // -- encrypt counter block --\n\
    \n\
    // calculate length of final block:\n\
    var blockLength = b<blockCount-1 ? blockSize : (plaintext.length-1)%blockSize+1;\n\
\n\
    var ct = '';\n\
    for (var i=0; i<blockLength; i++) {  // -- xor plaintext with ciphered counter byte-by-byte --\n\
      var plaintextByte = plaintext.charCodeAt(b*blockSize+i);\n\
      var cipherByte = plaintextByte ^ cipherCntr[i];\n\
      ct += String.fromCharCode(cipherByte);\n\
    }\n\
    // ct is now ciphertext for this block\n\
\n\
    ciphertext[b] = escCtrlChars(ct);  // escape troublesome characters in ciphertext\n\
  }\n\
\n\
  // convert the nonce to a string to go on the front of the ciphertext\n\
  var ctrTxt = '';\n\
  for (var i=0; i<8; i++) ctrTxt += String.fromCharCode(counterBlock[i]);\n\
  ctrTxt = escCtrlChars(ctrTxt);\n\
\n\
  // use '-' to separate blocks, use Array.join to concatenate arrays of strings for efficiency\n\
  return ctrTxt + '-' + ciphertext.join('-');\n\
}\n\
\n\
\n\
/* \n\
 * Use AES to decrypt 'ciphertext' with 'password' using 'nBits' key, in Counter mode of operation\n\
 *\n\
 *   for each block\n\
 *   - outputblock = cipher(counter, key)\n\
 *   - cipherblock = plaintext xor outputblock\n\
 */\n\
function AESDecryptCtr(ciphertext, password, nBits) {\n\
  if (!(nBits==128 || nBits==192 || nBits==256)) return '';  // standard allows 128/192/256 bit keys\n\
\n\
  var nBytes = nBits/8;  // no bytes in key\n\
  var pwBytes = new Array(nBytes);\n\
  for (var i=0; i<nBytes; i++) pwBytes[i] = password.charCodeAt(i) & 0xff;\n\
  var pwKeySchedule = KeyExpansion(pwBytes);\n\
  var key = Cipher(pwBytes, pwKeySchedule);\n\
  key = key.concat(key.slice(0, nBytes-16));  // key is now 16/24/32 bytes long\n\
\n\
  var keySchedule = KeyExpansion(key);\n\
\n\
  ciphertext = ciphertext.split('-');  // split ciphertext into array of block-length strings \n\
\n\
  // recover nonce from 1st element of ciphertext\n\
  var blockSize = 16;  // block size fixed at 16 bytes / 128 bits (Nb=4) for AES\n\
  var counterBlock = new Array(blockSize);\n\
  var ctrTxt = unescCtrlChars(ciphertext[0]);\n\
  for (var i=0; i<8; i++) counterBlock[i] = ctrTxt.charCodeAt(i);\n\
\n\
  var plaintext = new Array(ciphertext.length-1);\n\
\n\
  for (var b=1; b<ciphertext.length; b++) {\n\
    // set counter (block #) in last 8 bytes of counter block (leaving nonce in 1st 8 bytes)\n\
    for (var c=0; c<4; c++) counterBlock[15-c] = ((b-1) >>> c*8) & 0xff;\n\
    for (var c=0; c<4; c++) counterBlock[15-c-4] = ((b/0x100000000-1) >>> c*8) & 0xff;\n\
\n\
    var cipherCntr = Cipher(counterBlock, keySchedule);  // encrypt counter block\n\
\n\
    ciphertext[b] = unescCtrlChars(ciphertext[b]);\n\
\n\
    var pt = '';\n\
    for (var i=0; i<ciphertext[b].length; i++) {\n\
      // -- xor plaintext with ciphered counter byte-by-byte --\n\
      var ciphertextByte = ciphertext[b].charCodeAt(i);\n\
      var plaintextByte = ciphertextByte ^ cipherCntr[i];\n\
      pt += String.fromCharCode(plaintextByte);\n\
    }\n\
    // pt is now plaintext for this block\n\
\n\
    plaintext[b-1] = pt;  // b-1 'cos no initial nonce block in plaintext\n\
  }\n\
\n\
  return plaintext.join('');\n\
}\n\
\n\
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */\n\
\n\
function escCtrlChars(str) {  // escape control chars which might cause problems handling ciphertext\n\
  return str.replace(/[\\0\\t\\n\\v\\f\\r\\xa0'\"!-]/g, function(c) { return '!' + c.charCodeAt(0) + '!'; });\n\
}  // \\xa0 to cater for bug in Firefox; include '-' to leave it free for use as a block marker\n\
\n\
function unescCtrlChars(str) {  // unescape potentially problematic control characters\n\
  return str.replace(/!\\d\\d?\\d?!/g, function(c) { return String.fromCharCode(c.slice(1,-1)); });\n\
}\n\
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */\n\
\n\
/*\n\
 * if escCtrlChars()/unescCtrlChars() still gives problems, use encodeBase64()/decodeBase64() instead\n\
 */\n\
var b64 = \"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=\";\n\
\n\
function encodeBase64(str) {  // http://tools.ietf.org/html/rfc4648\n\
   var o1, o2, o3, h1, h2, h3, h4, bits, i=0, enc='';\n\
   \n\
   str = encodeUTF8(str);  // encode multi-byte chars into UTF-8 for byte-array\n\
\n\
   do {  // pack three octets into four hexets\n\
      o1 = str.charCodeAt(i++);\n\
      o2 = str.charCodeAt(i++);\n\
      o3 = str.charCodeAt(i++);\n\
      \n\
      bits = o1<<16 | o2<<8 | o3;\n\
      \n\
      h1 = bits>>18 & 0x3f;\n\
      h2 = bits>>12 & 0x3f;\n\
      h3 = bits>>6 & 0x3f;\n\
      h4 = bits & 0x3f;\n\
      \n\
      // end of string? index to '=' in b64\n\
      if (isNaN(o3)) h4 = 64;\n\
      if (isNaN(o2)) h3 = 64;\n\
      \n\
      // use hexets to index into b64, and append result to encoded string\n\
      enc += b64.charAt(h1) + b64.charAt(h2) + b64.charAt(h3) + b64.charAt(h4);\n\
   } while (i < str.length);\n\
   \n\
   return enc;\n\
}\n\
\n\
function decodeBase64(str) {\n\
   var o1, o2, o3, h1, h2, h3, h4, bits, i=0, enc='';\n\
\n\
   do {  // unpack four hexets into three octets using index points in b64\n\
      h1 = b64.indexOf(str.charAt(i++));\n\
      h2 = b64.indexOf(str.charAt(i++));\n\
      h3 = b64.indexOf(str.charAt(i++));\n\
      h4 = b64.indexOf(str.charAt(i++));\n\
      \n\
      bits = h1<<18 | h2<<12 | h3<<6 | h4;\n\
      \n\
      o1 = bits>>16 & 0xff;\n\
      o2 = bits>>8 & 0xff;\n\
      o3 = bits & 0xff;\n\
      \n\
      if (h3 == 64)      enc += String.fromCharCode(o1);\n\
      else if (h4 == 64) enc += String.fromCharCode(o1, o2);\n\
      else               enc += String.fromCharCode(o1, o2, o3);\n\
   } while (i < str.length);\n\
\n\
   return decodeUTF8(enc);  // decode UTF-8 byte-array back to Unicode\n\
}\n\
\n\
function encodeUTF8(str) {  // encode multi-byte string into utf-8 multiple single-byte characters \n\
  str = str.replace(\n\
      /[\\u0080-\\u07ff]/g,  // U+0080 - U+07FF = 2-byte chars\n\
      function(c) { \n\
        var cc = c.charCodeAt(0);\n\
        return String.fromCharCode(0xc0 | cc>>6, 0x80 | cc&0x3f); }\n\
    );\n\
  str = str.replace(\n\
      /[\\u0800-\\uffff]/g,  // U+0800 - U+FFFF = 3-byte chars\n\
      function(c) { \n\
        var cc = c.charCodeAt(0); \n\
        return String.fromCharCode(0xe0 | cc>>12, 0x80 | cc>>6&0x3F, 0x80 | cc&0x3f); }\n\
    );\n\
  return str;\n\
}\n\
\n\
function decodeUTF8(str) {  // decode utf-8 encoded string back into multi-byte characters\n\
  str = str.replace(\n\
      /[\\u00c0-\\u00df][\\u0080-\\u00bf]/g,                 // 2-byte chars\n\
      function(c) { \n\
        var cc = (c.charCodeAt(0)&0x1f)<<6 | c.charCodeAt(1)&0x3f;\n\
        return String.fromCharCode(cc); }\n\
    );\n\
  str = str.replace(\n\
      /[\\u00e0-\\u00ef][\\u0080-\\u00bf][\\u0080-\\u00bf]/g,  // 3-byte chars\n\
      function(c) { \n\
        var cc = (c.charCodeAt(0)&0x0f)<<12 | (c.charCodeAt(1)&0x3f<<6) | c.charCodeAt(2)&0x3f; \n\
        return String.fromCharCode(cc); }\n\
    );\n\
  return str;\n\
}\n\
\n\
\n\
function byteArrayToHexStr(b) {  // convert byte array to hex string for displaying test vectors\n\
  var s = '';\n\
  for (var i=0; i<b.length; i++) s += b[i].toString(16) + ' ';\n\
  return s;\n\
}\n\
\n\
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */\n\
\n\
\n\
var plainText = \"ROMEO: But, soft! what light through yonder window breaks?\\n\\\n\
It is the east, and Juliet is the sun.\\n\\\n\
Arise, fair sun, and kill the envious moon,\\n\\\n\
Who is already sick and pale with grief,\\n\\\n\
That thou her maid art far more fair than she:\\n\\\n\
Be not her maid, since she is envious;\\n\\\n\
Her vestal livery is but sick and green\\n\\\n\
And none but fools do wear it; cast it off.\\n\\\n\
It is my lady, O, it is my love!\\n\\\n\
O, that she knew she were!\\n\\\n\
She speaks yet she says nothing: what of that?\\n\\\n\
Her eye discourses; I will answer it.\\n\\\n\
I am too bold, 'tis not to me she speaks:\\n\\\n\
Two of the fairest stars in all the heaven,\\n\\\n\
Having some business, do entreat her eyes\\n\\\n\
To twinkle in their spheres till they return.\\n\\\n\
What if her eyes were there, they in her head?\\n\\\n\
The brightness of her cheek would shame those stars,\\n\\\n\
As daylight doth a lamp; her eyes in heaven\\n\\\n"
"Would through the airy region stream so bright\\n\\\n\
That birds would sing and think it were not night.\\n\\\n\
See, how she leans her cheek upon her hand!\\n\\\n\
O, that I were a glove upon that hand,\\n\\\n\
That I might touch that cheek!\\n\\\n\
JULIET: Ay me!\\n\\\n\
ROMEO: She speaks:\\n\\\n\
O, speak again, bright angel! for thou art\\n\\\n\
As glorious to this night, being o'er my head\\n\\\n\
As is a winged messenger of heaven\\n\\\n\
Unto the white-upturned wondering eyes\\n\\\n\
Of mortals that fall back to gaze on him\\n\\\n\
When he bestrides the lazy-pacing clouds\\n\\\n\
And sails upon the bosom of the air.\";\n\
\n\
var password = \"O Romeo, Romeo! wherefore art thou Romeo?\";\n\
\n\
var cipherText = AESEncryptCtr(plainText, password, 256);\n\
var decryptedText = AESDecryptCtr(cipherText, password, 256);\n\
\n\
if (decryptedText != plainText)\n\
    throw \"ERROR: bad result: expected \" + plainText + \" but got \" + decryptedText;\n\
\n\
\n\
", "\
/*\n\
 * A JavaScript implementation of the RSA Data Security, Inc. MD5 Message\n\
 * Digest Algorithm, as defined in RFC 1321.\n\
 * Version 2.1 Copyright (C) Paul Johnston 1999 - 2002.\n\
 * Other contributors: Greg Holt, Andrew Kepert, Ydnar, Lostinet\n\
 * Distributed under the BSD License\n\
 * See http://pajhome.org.uk/crypt/md5 for more info.\n\
 */\n\
\n\
/*\n\
 * Configurable variables. You may need to tweak these to be compatible with\n\
 * the server-side, but the defaults work in most cases.\n\
 */\n\
var hexcase = 0;  /* hex output format. 0 - lowercase; 1 - uppercase        */\n\
var b64pad  = \"\"; /* base-64 pad character. \"=\" for strict RFC compliance   */\n\
var chrsz   = 8;  /* bits per input character. 8 - ASCII; 16 - Unicode      */\n\
\n\
/*\n\
 * These are the functions you'll usually want to call\n\
 * They take string arguments and return either hex or base-64 encoded strings\n\
 */\n\
function hex_md5(s){ return binl2hex(core_md5(str2binl(s), s.length * chrsz));}\n\
function b64_md5(s){ return binl2b64(core_md5(str2binl(s), s.length * chrsz));}\n\
function str_md5(s){ return binl2str(core_md5(str2binl(s), s.length * chrsz));}\n\
function hex_hmac_md5(key, data) { return binl2hex(core_hmac_md5(key, data)); }\n\
function b64_hmac_md5(key, data) { return binl2b64(core_hmac_md5(key, data)); }\n\
function str_hmac_md5(key, data) { return binl2str(core_hmac_md5(key, data)); }\n\
\n\
/*\n\
 * Perform a simple self-test to see if the VM is working\n\
 */\n\
function md5_vm_test()\n\
{\n\
  return hex_md5(\"abc\") == \"900150983cd24fb0d6963f7d28e17f72\";\n\
}\n\
\n\
/*\n\
 * Calculate the MD5 of an array of little-endian words, and a bit length\n\
 */\n\
function core_md5(x, len)\n\
{\n\
  /* append padding */\n\
  x[len >> 5] |= 0x80 << ((len) % 32);\n\
  x[(((len + 64) >>> 9) << 4) + 14] = len;\n\
\n\
  var a =  1732584193;\n\
  var b = -271733879;\n\
  var c = -1732584194;\n\
  var d =  271733878;\n\
\n\
  for(var i = 0; i < x.length; i += 16)\n\
  {\n\
    var olda = a;\n\
    var oldb = b;\n\
    var oldc = c;\n\
    var oldd = d;\n\
\n\
    a = md5_ff(a, b, c, d, x[i+ 0], 7 , -680876936);\n\
    d = md5_ff(d, a, b, c, x[i+ 1], 12, -389564586);\n\
    c = md5_ff(c, d, a, b, x[i+ 2], 17,  606105819);\n\
    b = md5_ff(b, c, d, a, x[i+ 3], 22, -1044525330);\n\
    a = md5_ff(a, b, c, d, x[i+ 4], 7 , -176418897);\n\
    d = md5_ff(d, a, b, c, x[i+ 5], 12,  1200080426);\n\
    c = md5_ff(c, d, a, b, x[i+ 6], 17, -1473231341);\n\
    b = md5_ff(b, c, d, a, x[i+ 7], 22, -45705983);\n\
    a = md5_ff(a, b, c, d, x[i+ 8], 7 ,  1770035416);\n\
    d = md5_ff(d, a, b, c, x[i+ 9], 12, -1958414417);\n\
    c = md5_ff(c, d, a, b, x[i+10], 17, -42063);\n\
    b = md5_ff(b, c, d, a, x[i+11], 22, -1990404162);\n\
    a = md5_ff(a, b, c, d, x[i+12], 7 ,  1804603682);\n\
    d = md5_ff(d, a, b, c, x[i+13], 12, -40341101);\n\
    c = md5_ff(c, d, a, b, x[i+14], 17, -1502002290);\n\
    b = md5_ff(b, c, d, a, x[i+15], 22,  1236535329);\n\
\n\
    a = md5_gg(a, b, c, d, x[i+ 1], 5 , -165796510);\n\
    d = md5_gg(d, a, b, c, x[i+ 6], 9 , -1069501632);\n\
    c = md5_gg(c, d, a, b, x[i+11], 14,  643717713);\n\
    b = md5_gg(b, c, d, a, x[i+ 0], 20, -373897302);\n\
    a = md5_gg(a, b, c, d, x[i+ 5], 5 , -701558691);\n\
    d = md5_gg(d, a, b, c, x[i+10], 9 ,  38016083);\n\
    c = md5_gg(c, d, a, b, x[i+15], 14, -660478335);\n\
    b = md5_gg(b, c, d, a, x[i+ 4], 20, -405537848);\n\
    a = md5_gg(a, b, c, d, x[i+ 9], 5 ,  568446438);\n\
    d = md5_gg(d, a, b, c, x[i+14], 9 , -1019803690);\n\
    c = md5_gg(c, d, a, b, x[i+ 3], 14, -187363961);\n\
    b = md5_gg(b, c, d, a, x[i+ 8], 20,  1163531501);\n\
    a = md5_gg(a, b, c, d, x[i+13], 5 , -1444681467);\n\
    d = md5_gg(d, a, b, c, x[i+ 2], 9 , -51403784);\n\
    c = md5_gg(c, d, a, b, x[i+ 7], 14,  1735328473);\n\
    b = md5_gg(b, c, d, a, x[i+12], 20, -1926607734);\n\
\n\
    a = md5_hh(a, b, c, d, x[i+ 5], 4 , -378558);\n\
    d = md5_hh(d, a, b, c, x[i+ 8], 11, -2022574463);\n\
    c = md5_hh(c, d, a, b, x[i+11], 16,  1839030562);\n\
    b = md5_hh(b, c, d, a, x[i+14], 23, -35309556);\n\
    a = md5_hh(a, b, c, d, x[i+ 1], 4 , -1530992060);\n\
    d = md5_hh(d, a, b, c, x[i+ 4], 11,  1272893353);\n\
    c = md5_hh(c, d, a, b, x[i+ 7], 16, -155497632);\n\
    b = md5_hh(b, c, d, a, x[i+10], 23, -1094730640);\n\
    a = md5_hh(a, b, c, d, x[i+13], 4 ,  681279174);\n\
    d = md5_hh(d, a, b, c, x[i+ 0], 11, -358537222);\n\
    c = md5_hh(c, d, a, b, x[i+ 3], 16, -722521979);\n\
    b = md5_hh(b, c, d, a, x[i+ 6], 23,  76029189);\n\
    a = md5_hh(a, b, c, d, x[i+ 9], 4 , -640364487);\n\
    d = md5_hh(d, a, b, c, x[i+12], 11, -421815835);\n\
    c = md5_hh(c, d, a, b, x[i+15], 16,  530742520);\n\
    b = md5_hh(b, c, d, a, x[i+ 2], 23, -995338651);\n\
\n\
    a = md5_ii(a, b, c, d, x[i+ 0], 6 , -198630844);\n\
    d = md5_ii(d, a, b, c, x[i+ 7], 10,  1126891415);\n\
    c = md5_ii(c, d, a, b, x[i+14], 15, -1416354905);\n\
    b = md5_ii(b, c, d, a, x[i+ 5], 21, -57434055);\n\
    a = md5_ii(a, b, c, d, x[i+12], 6 ,  1700485571);\n\
    d = md5_ii(d, a, b, c, x[i+ 3], 10, -1894986606);\n\
    c = md5_ii(c, d, a, b, x[i+10], 15, -1051523);\n\
    b = md5_ii(b, c, d, a, x[i+ 1], 21, -2054922799);\n\
    a = md5_ii(a, b, c, d, x[i+ 8], 6 ,  1873313359);\n\
    d = md5_ii(d, a, b, c, x[i+15], 10, -30611744);\n\
    c = md5_ii(c, d, a, b, x[i+ 6], 15, -1560198380);\n\
    b = md5_ii(b, c, d, a, x[i+13], 21,  1309151649);\n\
    a = md5_ii(a, b, c, d, x[i+ 4], 6 , -145523070);\n\
    d = md5_ii(d, a, b, c, x[i+11], 10, -1120210379);\n\
    c = md5_ii(c, d, a, b, x[i+ 2], 15,  718787259);\n\
    b = md5_ii(b, c, d, a, x[i+ 9], 21, -343485551);\n\
\n\
    a = safe_add(a, olda);\n\
    b = safe_add(b, oldb);\n\
    c = safe_add(c, oldc);\n\
    d = safe_add(d, oldd);\n\
  }\n\
  return Array(a, b, c, d);\n\
\n\
}\n\
\n\
/*\n\
 * These functions implement the four basic operations the algorithm uses.\n\
 */\n\
function md5_cmn(q, a, b, x, s, t)\n\
{\n\
  return safe_add(bit_rol(safe_add(safe_add(a, q), safe_add(x, t)), s),b);\n\
}\n\
function md5_ff(a, b, c, d, x, s, t)\n\
{\n\
  return md5_cmn((b & c) | ((~b) & d), a, b, x, s, t);\n\
}\n\
function md5_gg(a, b, c, d, x, s, t)\n\
{\n\
  return md5_cmn((b & d) | (c & (~d)), a, b, x, s, t);\n\
}\n\
function md5_hh(a, b, c, d, x, s, t)\n\
{\n\
  return md5_cmn(b ^ c ^ d, a, b, x, s, t);\n\
}\n\
function md5_ii(a, b, c, d, x, s, t)\n\
{\n\
  return md5_cmn(c ^ (b | (~d)), a, b, x, s, t);\n\
}\n\
\n\
/*\n\
 * Calculate the HMAC-MD5, of a key and some data\n\
 */\n\
function core_hmac_md5(key, data)\n\
{\n\
  var bkey = str2binl(key);\n\
  if(bkey.length > 16) bkey = core_md5(bkey, key.length * chrsz);\n\
\n\
  var ipad = Array(16), opad = Array(16);\n\
  for(var i = 0; i < 16; i++)\n\
  {\n\
    ipad[i] = bkey[i] ^ 0x36363636;\n\
    opad[i] = bkey[i] ^ 0x5C5C5C5C;\n\
  }\n\
\n\
  var hash = core_md5(ipad.concat(str2binl(data)), 512 + data.length * chrsz);\n\
  return core_md5(opad.concat(hash), 512 + 128);\n\
}\n\
\n\
/*\n\
 * Add integers, wrapping at 2^32. This uses 16-bit operations internally\n\
 * to work around bugs in some JS interpreters.\n\
 */\n\
function safe_add(x, y)\n\
{\n\
  var lsw = (x & 0xFFFF) + (y & 0xFFFF);\n\
  var msw = (x >> 16) + (y >> 16) + (lsw >> 16);\n\
  return (msw << 16) | (lsw & 0xFFFF);\n\
}\n\
\n\
/*\n\
 * Bitwise rotate a 32-bit number to the left.\n\
 */\n\
function bit_rol(num, cnt)\n\
{\n\
  return (num << cnt) | (num >>> (32 - cnt));\n\
}\n\
\n\
/*\n\
 * Convert a string to an array of little-endian words\n\
 * If chrsz is ASCII, characters >255 have their hi-byte silently ignored.\n\
 */\n\
function str2binl(str)\n\
{\n\
  var bin = Array();\n\
  var mask = (1 << chrsz) - 1;\n\
  for(var i = 0; i < str.length * chrsz; i += chrsz)\n\
    bin[i>>5] |= (str.charCodeAt(i / chrsz) & mask) << (i%32);\n\
  return bin;\n\
}\n\
\n\
/*\n\
 * Convert an array of little-endian words to a string\n\
 */\n\
function binl2str(bin)\n\
{\n\
  var str = \"\";\n\
  var mask = (1 << chrsz) - 1;\n\
  for(var i = 0; i < bin.length * 32; i += chrsz)\n\
    str += String.fromCharCode((bin[i>>5] >>> (i % 32)) & mask);\n\
  return str;\n\
}\n\
\n\
/*\n\
 * Convert an array of little-endian words to a hex string.\n\
 */\n\
function binl2hex(binarray)\n\
{\n\
  var hex_tab = hexcase ? \"0123456789ABCDEF\" : \"0123456789abcdef\";\n\
  var str = \"\";\n\
  for(var i = 0; i < binarray.length * 4; i++)\n\
  {\n\
    str += hex_tab.charAt((binarray[i>>2] >> ((i%4)*8+4)) & 0xF) +\n\
           hex_tab.charAt((binarray[i>>2] >> ((i%4)*8  )) & 0xF);\n\
  }\n\
  return str;\n\
}\n\
\n\
/*\n\
 * Convert an array of little-endian words to a base-64 string\n\
 */\n\
function binl2b64(binarray)\n\
{\n\
  var tab = \"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/\";\n\
  var str = \"\";\n\
  for(var i = 0; i < binarray.length * 4; i += 3)\n\
  {\n\
    var triplet = (((binarray[i   >> 2] >> 8 * ( i   %4)) & 0xFF) << 16)\n\
                | (((binarray[i+1 >> 2] >> 8 * ((i+1)%4)) & 0xFF) << 8 )\n\
                |  ((binarray[i+2 >> 2] >> 8 * ((i+2)%4)) & 0xFF);\n\
    for(var j = 0; j < 4; j++)\n\
    {\n\
      if(i * 8 + j * 6 > binarray.length * 32) str += b64pad;\n\
      else str += tab.charAt((triplet >> 6*(3-j)) & 0x3F);\n\
    }\n\
  }\n\
  return str;\n\
}\n\
\n\
var plainText = \"Rebellious subjects, enemies to peace,\\n\\\n\
Profaners of this neighbour-stained steel,--\\n\\\n\
Will they not hear? What, ho! you men, you beasts,\\n\\\n\
That quench the fire of your pernicious rage\\n\\\n\
With purple fountains issuing from your veins,\\n\\\n\
On pain of torture, from those bloody hands\\n\\\n\
Throw your mistemper'd weapons to the ground,\\n\\\n\
And hear the sentence of your moved prince.\\n\\\n\
Three civil brawls, bred of an airy word,\\n\\\n\
By thee, old Capulet, and Montague,\\n\\\n\
Have thrice disturb'd the quiet of our streets,\\n\\\n\
And made Verona's ancient citizens\\n\\\n\
Cast by their grave beseeming ornaments,\\n\\\n\
To wield old partisans, in hands as old,\\n\\\n\
Canker'd with peace, to part your canker'd hate:\\n\\\n\
If ever you disturb our streets again,\\n\\\n\
Your lives shall pay the forfeit of the peace.\\n\\\n\
For this time, all the rest depart away:\\n\\\n\
You Capulet; shall go along with me:\\n\\\n\
And, Montague, come you this afternoon,\\n\\\n\
To know our further pleasure in this case,\\n\\\n\
To old Free-town, our common judgment-place.\\n\\\n\
Once more, on pain of death, all men depart.\"\n\
\n\
for (var i = 0; i <4; i++) {\n\
    plainText += plainText;\n\
}\n\
\n\
var md5Output = hex_md5(plainText);\n\
\n\
var expected = \"a831e91e0f70eddcb70dc61c6f82f6cd\";\n\
\n\
if (md5Output != expected)\n\
    throw \"ERROR: bad result: expected \" + expected + \" but got \" + md5Output;\n\
\n\
\n\
", "\
/*\n\
 * A JavaScript implementation of the Secure Hash Algorithm, SHA-1, as defined\n\
 * in FIPS PUB 180-1\n\
 * Version 2.1a Copyright Paul Johnston 2000 - 2002.\n\
 * Other contributors: Greg Holt, Andrew Kepert, Ydnar, Lostinet\n\
 * Distributed under the BSD License\n\
 * See http://pajhome.org.uk/crypt/md5 for details.\n\
 */\n\
\n\
/*\n\
 * Configurable variables. You may need to tweak these to be compatible with\n\
 * the server-side, but the defaults work in most cases.\n\
 */\n\
var hexcase = 0;  /* hex output format. 0 - lowercase; 1 - uppercase        */\n\
var b64pad  = \"\"; /* base-64 pad character. \"=\" for strict RFC compliance   */\n\
var chrsz   = 8;  /* bits per input character. 8 - ASCII; 16 - Unicode      */\n\
\n\
/*\n\
 * These are the functions you'll usually want to call\n\
 * They take string arguments and return either hex or base-64 encoded strings\n\
 */\n\
function hex_sha1(s){return binb2hex(core_sha1(str2binb(s),s.length * chrsz));}\n\
function b64_sha1(s){return binb2b64(core_sha1(str2binb(s),s.length * chrsz));}\n\
function str_sha1(s){return binb2str(core_sha1(str2binb(s),s.length * chrsz));}\n\
function hex_hmac_sha1(key, data){ return binb2hex(core_hmac_sha1(key, data));}\n\
function b64_hmac_sha1(key, data){ return binb2b64(core_hmac_sha1(key, data));}\n\
function str_hmac_sha1(key, data){ return binb2str(core_hmac_sha1(key, data));}\n\
\n\
/*\n\
 * Perform a simple self-test to see if the VM is working\n\
 */\n\
function sha1_vm_test()\n\
{\n\
  return hex_sha1(\"abc\") == \"a9993e364706816aba3e25717850c26c9cd0d89d\";\n\
}\n\
\n\
/*\n\
 * Calculate the SHA-1 of an array of big-endian words, and a bit length\n\
 */\n\
function core_sha1(x, len)\n\
{\n\
  /* append padding */\n\
  x[len >> 5] |= 0x80 << (24 - len % 32);\n\
  x[((len + 64 >> 9) << 4) + 15] = len;\n\
\n\
  var w = Array(80);\n\
  var a =  1732584193;\n\
  var b = -271733879;\n\
  var c = -1732584194;\n\
  var d =  271733878;\n\
  var e = -1009589776;\n\
\n\
  for(var i = 0; i < x.length; i += 16)\n\
  {\n\
    var olda = a;\n\
    var oldb = b;\n\
    var oldc = c;\n\
    var oldd = d;\n\
    var olde = e;\n\
\n\
    for(var j = 0; j < 80; j++)\n\
    {\n\
      if(j < 16) w[j] = x[i + j];\n\
      else w[j] = rol(w[j-3] ^ w[j-8] ^ w[j-14] ^ w[j-16], 1);\n\
      var t = safe_add(safe_add(rol(a, 5), sha1_ft(j, b, c, d)),\n\
                       safe_add(safe_add(e, w[j]), sha1_kt(j)));\n\
      e = d;\n\
      d = c;\n\
      c = rol(b, 30);\n\
      b = a;\n\
      a = t;\n\
    }\n\
\n\
    a = safe_add(a, olda);\n\
    b = safe_add(b, oldb);\n\
    c = safe_add(c, oldc);\n\
    d = safe_add(d, oldd);\n\
    e = safe_add(e, olde);\n\
  }\n\
  return Array(a, b, c, d, e);\n\
\n\
}\n\
\n\
/*\n\
 * Perform the appropriate triplet combination function for the current\n\
 * iteration\n\
 */\n\
function sha1_ft(t, b, c, d)\n\
{\n\
  if(t < 20) return (b & c) | ((~b) & d);\n\
  if(t < 40) return b ^ c ^ d;\n\
  if(t < 60) return (b & c) | (b & d) | (c & d);\n\
  return b ^ c ^ d;\n\
}\n\
\n\
/*\n\
 * Determine the appropriate additive constant for the current iteration\n\
 */\n\
function sha1_kt(t)\n\
{\n\
  return (t < 20) ?  1518500249 : (t < 40) ?  1859775393 :\n\
         (t < 60) ? -1894007588 : -899497514;\n\
}\n\
\n\
/*\n\
 * Calculate the HMAC-SHA1 of a key and some data\n\
 */\n\
function core_hmac_sha1(key, data)\n\
{\n\
  var bkey = str2binb(key);\n\
  if(bkey.length > 16) bkey = core_sha1(bkey, key.length * chrsz);\n\
\n\
  var ipad = Array(16), opad = Array(16);\n\
  for(var i = 0; i < 16; i++)\n\
  {\n\
    ipad[i] = bkey[i] ^ 0x36363636;\n\
    opad[i] = bkey[i] ^ 0x5C5C5C5C;\n\
  }\n\
\n\
  var hash = core_sha1(ipad.concat(str2binb(data)), 512 + data.length * chrsz);\n\
  return core_sha1(opad.concat(hash), 512 + 160);\n\
}\n\
\n\
/*\n\
 * Add integers, wrapping at 2^32. This uses 16-bit operations internally\n\
 * to work around bugs in some JS interpreters.\n\
 */\n\
function safe_add(x, y)\n\
{\n\
  var lsw = (x & 0xFFFF) + (y & 0xFFFF);\n\
  var msw = (x >> 16) + (y >> 16) + (lsw >> 16);\n\
  return (msw << 16) | (lsw & 0xFFFF);\n\
}\n\
\n\
/*\n\
 * Bitwise rotate a 32-bit number to the left.\n\
 */\n\
function rol(num, cnt)\n\
{\n\
  return (num << cnt) | (num >>> (32 - cnt));\n\
}\n\
\n\
/*\n\
 * Convert an 8-bit or 16-bit string to an array of big-endian words\n\
 * In 8-bit function, characters >255 have their hi-byte silently ignored.\n\
 */\n\
function str2binb(str)\n\
{\n\
  var bin = Array();\n\
  var mask = (1 << chrsz) - 1;\n\
  for(var i = 0; i < str.length * chrsz; i += chrsz)\n\
    bin[i>>5] |= (str.charCodeAt(i / chrsz) & mask) << (32 - chrsz - i%32);\n\
  return bin;\n\
}\n\
\n\
/*\n\
 * Convert an array of big-endian words to a string\n\
 */\n\
function binb2str(bin)\n\
{\n\
  var str = \"\";\n\
  var mask = (1 << chrsz) - 1;\n\
  for(var i = 0; i < bin.length * 32; i += chrsz)\n\
    str += String.fromCharCode((bin[i>>5] >>> (32 - chrsz - i%32)) & mask);\n\
  return str;\n\
}\n\
\n\
/*\n\
 * Convert an array of big-endian words to a hex string.\n\
 */\n\
function binb2hex(binarray)\n\
{\n\
  var hex_tab = hexcase ? \"0123456789ABCDEF\" : \"0123456789abcdef\";\n\
  var str = \"\";\n\
  for(var i = 0; i < binarray.length * 4; i++)\n\
  {\n\
    str += hex_tab.charAt((binarray[i>>2] >> ((3 - i%4)*8+4)) & 0xF) +\n\
           hex_tab.charAt((binarray[i>>2] >> ((3 - i%4)*8  )) & 0xF);\n\
  }\n\
  return str;\n\
}\n\
\n\
/*\n\
 * Convert an array of big-endian words to a base-64 string\n\
 */\n\
function binb2b64(binarray)\n\
{\n\
  var tab = \"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/\";\n\
  var str = \"\";\n\
  for(var i = 0; i < binarray.length * 4; i += 3)\n\
  {\n\
    var triplet = (((binarray[i   >> 2] >> 8 * (3 -  i   %4)) & 0xFF) << 16)\n\
                | (((binarray[i+1 >> 2] >> 8 * (3 - (i+1)%4)) & 0xFF) << 8 )\n\
                |  ((binarray[i+2 >> 2] >> 8 * (3 - (i+2)%4)) & 0xFF);\n\
    for(var j = 0; j < 4; j++)\n\
    {\n\
      if(i * 8 + j * 6 > binarray.length * 32) str += b64pad;\n\
      else str += tab.charAt((triplet >> 6*(3-j)) & 0x3F);\n\
    }\n\
  }\n\
  return str;\n\
}\n\
\n\
\n\
var plainText = \"Two households, both alike in dignity,\\n\\\n\
In fair Verona, where we lay our scene,\\n\\\n\
From ancient grudge break to new mutiny,\\n\\\n\
Where civil blood makes civil hands unclean.\\n\\\n\
From forth the fatal loins of these two foes\\n\\\n\
A pair of star-cross'd lovers take their life;\\n\\\n\
Whole misadventured piteous overthrows\\n\\\n\
Do with their death bury their parents' strife.\\n\\\n\
The fearful passage of their death-mark'd love,\\n\\\n\
And the continuance of their parents' rage,\\n\\\n\
Which, but their children's end, nought could remove,\\n\\\n\
Is now the two hours' traffic of our stage;\\n\\\n\
The which if you with patient ears attend,\\n\\\n\
What here shall miss, our toil shall strive to mend.\";\n\
\n\
for (var i = 0; i <4; i++) {\n\
    plainText += plainText;\n\
}\n\
\n\
var sha1Output = hex_sha1(plainText);\n\
\n\
var expected = \"2524d264def74cce2498bf112bedf00e6c0b796d\";\n\
if (sha1Output != expected)\n\
    throw \"ERROR: bad result: expected \" + expected + \" but got \" + sha1Output;\n\
\n\
", "\
function arrayExists(array, x) {\n\
    for (var i = 0; i < array.length; i++) {\n\
        if (array[i] == x) return true;\n\
    }\n\
    return false;\n\
}\n\
\n\
Date.prototype.formatDate = function (input,time) {\n\
    // formatDate :\n\
    // a PHP date like function, for formatting date strings\n\
    // See: http://www.php.net/date\n\
    //\n\
    // input : format string\n\
    // time : epoch time (seconds, and optional)\n\
    //\n\
    // if time is not passed, formatting is based on \n\
    // the current \"this\" date object's set time.\n\
    //\n\
    // supported:\n\
    // a, A, B, d, D, F, g, G, h, H, i, j, l (lowercase L), L, \n\
    // m, M, n, O, r, s, S, t, U, w, W, y, Y, z\n\
    //\n\
    // unsupported:\n\
    // I (capital i), T, Z    \n\
\n\
    var switches =    [\"a\", \"A\", \"B\", \"d\", \"D\", \"F\", \"g\", \"G\", \"h\", \"H\", \n\
                       \"i\", \"j\", \"l\", \"L\", \"m\", \"M\", \"n\", \"O\", \"r\", \"s\", \n\
                       \"S\", \"t\", \"U\", \"w\", \"W\", \"y\", \"Y\", \"z\"];\n\
    var daysLong =    [\"Sunday\", \"Monday\", \"Tuesday\", \"Wednesday\", \n\
                       \"Thursday\", \"Friday\", \"Saturday\"];\n\
    var daysShort =   [\"Sun\", \"Mon\", \"Tue\", \"Wed\", \n\
                       \"Thu\", \"Fri\", \"Sat\"];\n\
    var monthsShort = [\"Jan\", \"Feb\", \"Mar\", \"Apr\",\n\
                       \"May\", \"Jun\", \"Jul\", \"Aug\", \"Sep\",\n\
                       \"Oct\", \"Nov\", \"Dec\"];\n\
    var monthsLong =  [\"January\", \"February\", \"March\", \"April\",\n\
                       \"May\", \"June\", \"July\", \"August\", \"September\",\n\
                       \"October\", \"November\", \"December\"];\n\
    var daysSuffix = [\"st\", \"nd\", \"rd\", \"th\", \"th\", \"th\", \"th\", // 1st - 7th\n\
                      \"th\", \"th\", \"th\", \"th\", \"th\", \"th\", \"th\", // 8th - 14th\n\
                      \"th\", \"th\", \"th\", \"th\", \"th\", \"th\", \"st\", // 15th - 21st\n\
                      \"nd\", \"rd\", \"th\", \"th\", \"th\", \"th\", \"th\", // 22nd - 28th\n\
                      \"th\", \"th\", \"st\"];                        // 29th - 31st\n\
\n\
    function a() {\n\
        // Lowercase Ante meridiem and Post meridiem\n\
        return self.getHours() > 11? \"pm\" : \"am\";\n\
    }\n\
    function A() {\n\
        // Uppercase Ante meridiem and Post meridiem\n\
        return self.getHours() > 11? \"PM\" : \"AM\";\n\
    }\n\
\n\
    function B(){\n\
        // Swatch internet time. code simply grabbed from ppk,\n\
        // since I was feeling lazy:\n\
        // http://www.xs4all.nl/~ppk/js/beat.html\n\
        var off = (self.getTimezoneOffset() + 60)*60;\n\
        var theSeconds = (self.getHours() * 3600) + \n\
                         (self.getMinutes() * 60) + \n\
                          self.getSeconds() + off;\n\
        var beat = Math.floor(theSeconds/86.4);\n\
        if (beat > 1000) beat -= 1000;\n\
        if (beat < 0) beat += 1000;\n\
        if ((\"\"+beat).length == 1) beat = \"00\"+beat;\n\
        if ((\"\"+beat).length == 2) beat = \"0\"+beat;\n\
        return beat;\n\
    }\n\
    \n\
    function d() {\n\
        // Day of the month, 2 digits with leading zeros\n\
        return new String(self.getDate()).length == 1?\n\
        \"0\"+self.getDate() : self.getDate();\n\
    }\n\
    function D() {\n\
        // A textual representation of a day, three letters\n\
        return daysShort[self.getDay()];\n\
    }\n\
    function F() {\n\
        // A full textual representation of a month\n\
        return monthsLong[self.getMonth()];\n\
    }\n\
    function g() {\n\
        // 12-hour format of an hour without leading zeros\n\
        return self.getHours() > 12? self.getHours()-12 : self.getHours();\n\
    }\n\
    function G() {\n\
        // 24-hour format of an hour without leading zeros\n\
        return self.getHours();\n\
    }\n\
    function h() {\n\
        // 12-hour format of an hour with leading zeros\n\
        if (self.getHours() > 12) {\n\
          var s = new String(self.getHours()-12);\n\
          return s.length == 1?\n\
          \"0\"+ (self.getHours()-12) : self.getHours()-12;\n\
        } else { \n\
          var s = new String(self.getHours());\n\
          return s.length == 1?\n\
          \"0\"+self.getHours() : self.getHours();\n\
        }  \n\
    }\n\
    function H() {\n\
        // 24-hour format of an hour with leading zeros\n\
        return new String(self.getHours()).length == 1?\n\
        \"0\"+self.getHours() : self.getHours();\n\
    }\n\
    function i() {\n\
        // Minutes with leading zeros\n\
        return new String(self.getMinutes()).length == 1? \n\
        \"0\"+self.getMinutes() : self.getMinutes(); \n\
    }\n\
    function j() {\n\
        // Day of the month without leading zeros\n\
        return self.getDate();\n\
    }    \n\
    function l() {\n\
        // A full textual representation of the day of the week\n\
        return daysLong[self.getDay()];\n\
    }\n\
    function L() {\n\
        // leap year or not. 1 if leap year, 0 if not.\n\
        // the logic should match iso's 8601 standard.\n\
        var y_ = Y();\n\
        if (         \n\
            (y_ % 4 == 0 && y_ % 100 != 0) ||\n\
            (y_ % 4 == 0 && y_ % 100 == 0 && y_ % 400 == 0)\n\
            ) {\n\
            return 1;\n\
        } else {\n\
            return 0;\n\
        }\n\
    }\n\
    function m() {\n\
        // Numeric representation of a month, with leading zeros\n\
        return self.getMonth() < 9?\n\
        \"0\"+(self.getMonth()+1) : \n\
        self.getMonth()+1;\n\
    }\n\
    function M() {\n\
        // A short textual representation of a month, three letters\n\
        return monthsShort[self.getMonth()];\n\
    }\n\
    function n() {\n\
        // Numeric representation of a month, without leading zeros\n\
        return self.getMonth()+1;\n\
    }\n\
    function O() {\n\
        // Difference to Greenwich time (GMT) in hours\n\
        var os = Math.abs(self.getTimezoneOffset());\n\
        var h = \"\"+Math.floor(os/60);\n\
        var m = \"\"+(os%60);\n\
        h.length == 1? h = \"0\"+h:1;\n\
        m.length == 1? m = \"0\"+m:1;\n\
        return self.getTimezoneOffset() < 0 ? \"+\"+h+m : \"-\"+h+m;\n\
    }\n\
    function r() {\n\
        // RFC 822 formatted date\n\
        var r; // result\n\
        //  Thu    ,     21          Dec         2000\n\
        r = D() + \", \" + j() + \" \" + M() + \" \" + Y() +\n\
        //        16     :    01     :    07          +0200\n\
            \" \" + H() + \":\" + i() + \":\" + s() + \" \" + O();\n\
        return r;\n\
    }\n\
    function S() {\n\
        // English ordinal suffix for the day of the month, 2 characters\n\
        return daysSuffix[self.getDate()-1];\n\
    }\n\
    function s() {\n\
        // Seconds, with leading zeros\n\
        return new String(self.getSeconds()).length == 1?\n\
        \"0\"+self.getSeconds() : self.getSeconds();\n\
    }\n\
    function t() {\n\
\n\
        // thanks to Matt Bannon for some much needed code-fixes here!\n\
        var daysinmonths = [null,31,28,31,30,31,30,31,31,30,31,30,31];\n\
        if (L()==1 && n()==2) return 29; // leap day\n\
        return daysinmonths[n()];\n\
    }\n\
    function U() {\n\
        // Seconds since the Unix Epoch (January 1 1970 00:00:00 GMT)\n\
        return Math.round(self.getTime()/1000);\n\
    }\n\
    function W() {\n\
        // Weeknumber, as per ISO specification:\n\
        // http://www.cl.cam.ac.uk/~mgk25/iso-time.html\n\
        \n\
        // if the day is three days before newyears eve,\n\
        // there's a chance it's \"week 1\" of next year.\n\
        // here we check for that.\n\
        var beforeNY = 364+L() - z();\n\
        var afterNY  = z();\n\
        var weekday = w()!=0?w()-1:6; // makes sunday (0), into 6.\n\
        if (beforeNY <= 2 && weekday <= 2-beforeNY) {\n\
            return 1;\n\
        }\n\
        // similarly, if the day is within threedays of newyears\n\
        // there's a chance it belongs in the old year.\n\
        var ny = new Date(\"January 1 \" + Y() + \" 00:00:00\");\n\
        var nyDay = ny.getDay()!=0?ny.getDay()-1:6;\n\
        if (\n\
            (afterNY <= 2) && \n\
            (nyDay >=4)  && \n\
            (afterNY >= (6-nyDay))\n\
            ) {\n\
            // Since I'm not sure we can just always return 53,\n\
            // i call the function here again, using the last day\n\
            // of the previous year, as the date, and then just\n\
            // return that week.\n\
            var prevNY = new Date(\"December 31 \" + (Y()-1) + \" 00:00:00\");\n\
            return prevNY.formatDate(\"W\");\n\
        }\n\
        \n\
        // week 1, is the week that has the first thursday in it.\n\
        // note that this value is not zero index.\n\
        if (nyDay <= 3) {\n\
            // first day of the year fell on a thursday, or earlier.\n\
            return 1 + Math.floor( ( z() + nyDay ) / 7 );\n\
        } else {\n\
            // first day of the year fell on a friday, or later.\n\
            return 1 + Math.floor( ( z() - ( 7 - nyDay ) ) / 7 );\n\
        }\n\
    }\n\
    function w() {\n\
        // Numeric representation of the day of the week\n\
        return self.getDay();\n\
    }\n\
    \n\
    function Y() {\n\
        // A full numeric representation of a year, 4 digits\n\
\n\
        // we first check, if getFullYear is supported. if it\n\
        // is, we just use that. ppks code is nice, but wont\n\
        // work with dates outside 1900-2038, or something like that\n\
        if (self.getFullYear) {\n\
            var newDate = new Date(\"January 1 2001 00:00:00 +0000\");\n\
            var x = newDate .getFullYear();\n\
            if (x == 2001) {              \n\
                // i trust the method now\n\
                return self.getFullYear();\n\
            }\n\
        }\n\
        // else, do this:\n\
        // codes thanks to ppk:\n\
        // http://www.xs4all.nl/~ppk/js/introdate.html\n\
        var x = self.getYear();\n\
        var y = x % 100;\n\
        y += (y < 38) ? 2000 : 1900;\n\
        return y;\n\
    }\n\
    function y() {\n\
        // A two-digit representation of a year\n\
        var y = Y()+\"\";\n\
        return y.substring(y.length-2,y.length);\n\
    }\n\
    function z() {\n\
        // The day of the year, zero indexed! 0 through 366\n\
        var t = new Date(\"January 1 \" + Y() + \" 00:00:00\");\n\
        var diff = self.getTime() - t.getTime();\n\
        return Math.floor(diff/1000/60/60/24);\n\
    }\n\
        \n\
    var self = this;\n\
    if (time) {\n\
        // save time\n\
        var prevTime = self.getTime();\n\
        self.setTime(time);\n\
    }\n\
    \n\
    var ia = input.split(\"\");\n\
    var ij = 0;\n\
    while (ia[ij]) {\n\
        if (ia[ij] == \"\\\\\") {\n\
            // this is our way of allowing users to escape stuff\n\
            ia.splice(ij,1);\n\
        } else {\n\
            if (arrayExists(switches,ia[ij])) {\n\
                ia[ij] = eval(ia[ij] + \"()\");\n\
            }\n\
        }\n\
        ij++;\n\
    }\n\
    // reset time, back to what it was\n\
    if (prevTime) {\n\
        self.setTime(prevTime);\n\
    }\n\
    return ia.join(\"\");\n\
}\n\
\n\
var date = new Date(\"1/1/2007 1:11:11\");\n\
\n\
for (i = 0; i < 500; ++i) {\n\
    var shortFormat = date.formatDate(\"Y-m-d\");\n\
    var longFormat = date.formatDate(\"l, F d, Y g:i:s A\");\n\
    date.setTime(date.getTime() + 84266956);\n\
}\n\
\n\
// FIXME: Find a way to validate this test.\n\
// https://bugs.webkit.org/show_bug.cgi?id=114849\n\
\n\
", "\
/*\n\
 * Copyright (C) 2004 Baron Schwartz <baron at sequent dot org>\n\
 *\n\
 * This program is free software; you can redistribute it and/or modify it\n\
 * under the terms of the GNU Lesser General Public License as published by the\n\
 * Free Software Foundation, version 2.1.\n\
 *\n\
 * This program is distributed in the hope that it will be useful, but WITHOUT\n\
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS\n\
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more\n\
 * details.\n\
 */\n\
\n\
Date.parseFunctions = {count:0};\n\
Date.parseRegexes = [];\n\
Date.formatFunctions = {count:0};\n\
\n\
Date.prototype.dateFormat = function(format) {\n\
    if (Date.formatFunctions[format] == null) {\n\
        Date.createNewFormat(format);\n\
    }\n\
    var func = Date.formatFunctions[format];\n\
    return this[func]();\n\
}\n\
\n\
Date.createNewFormat = function(format) {\n\
    var funcName = \"format\" + Date.formatFunctions.count++;\n\
    Date.formatFunctions[format] = funcName;\n\
    var code = \"Date.prototype.\" + funcName + \" = function(){return \";\n\
    var special = false;\n\
    var ch = '';\n\
    for (var i = 0; i < format.length; ++i) {\n\
        ch = format.charAt(i);\n\
        if (!special && ch == \"\\\\\") {\n\
            special = true;\n\
        }\n\
        else if (special) {\n\
            special = false;\n\
            code += \"'\" + String.escape(ch) + \"' + \";\n\
        }\n\
        else {\n\
            code += Date.getFormatCode(ch);\n\
        }\n\
    }\n\
    eval(code.substring(0, code.length - 3) + \";}\");\n\
}\n\
\n\
Date.getFormatCode = function(character) {\n\
    switch (character) {\n\
    case \"d\":\n\
        return \"String.leftPad(this.getDate(), 2, '0') + \";\n\
    case \"D\":\n\
        return \"Date.dayNames[this.getDay()].substring(0, 3) + \";\n\
    case \"j\":\n\
        return \"this.getDate() + \";\n\
    case \"l\":\n\
        return \"Date.dayNames[this.getDay()] + \";\n\
    case \"S\":\n\
        return \"this.getSuffix() + \";\n\
    case \"w\":\n\
        return \"this.getDay() + \";\n\
    case \"z\":\n\
        return \"this.getDayOfYear() + \";\n\
    case \"W\":\n\
        return \"this.getWeekOfYear() + \";\n\
    case \"F\":\n\
        return \"Date.monthNames[this.getMonth()] + \";\n\
    case \"m\":\n\
        return \"String.leftPad(this.getMonth() + 1, 2, '0') + \";\n\
    case \"M\":\n\
        return \"Date.monthNames[this.getMonth()].substring(0, 3) + \";\n\
    case \"n\":\n\
        return \"(this.getMonth() + 1) + \";\n\
    case \"t\":\n\
        return \"this.getDaysInMonth() + \";\n\
    case \"L\":\n\
        return \"(this.isLeapYear() ? 1 : 0) + \";\n\
    case \"Y\":\n\
        return \"this.getFullYear() + \";\n\
    case \"y\":\n\
        return \"('' + this.getFullYear()).substring(2, 4) + \";\n\
    case \"a\":\n\
        return \"(this.getHours() < 12 ? 'am' : 'pm') + \";\n\
    case \"A\":\n\
        return \"(this.getHours() < 12 ? 'AM' : 'PM') + \";\n\
    case \"g\":\n\
        return \"((this.getHours() %12) ? this.getHours() % 12 : 12) + \";\n\
    case \"G\":\n\
        return \"this.getHours() + \";\n\
    case \"h\":\n\
        return \"String.leftPad((this.getHours() %12) ? this.getHours() % 12 : 12, 2, '0') + \";\n\
    case \"H\":\n\
        return \"String.leftPad(this.getHours(), 2, '0') + \";\n\
    case \"i\":\n\
        return \"String.leftPad(this.getMinutes(), 2, '0') + \";\n\
    case \"s\":\n\
        return \"String.leftPad(this.getSeconds(), 2, '0') + \";\n\
    case \"O\":\n\
        return \"this.getGMTOffset() + \";\n\
    case \"T\":\n\
        return \"this.getTimezone() + \";\n\
    case \"Z\":\n\
        return \"(this.getTimezoneOffset() * -60) + \";\n\
    default:\n\
        return \"'\" + String.escape(character) + \"' + \";\n\
    }\n\
}\n\
\n\
Date.parseDate = function(input, format) {\n\
    if (Date.parseFunctions[format] == null) {\n\
        Date.createParser(format);\n\
    }\n\
    var func = Date.parseFunctions[format];\n\
    return Date[func](input);\n\
}\n\
\n\
Date.createParser = function(format) {\n\
    var funcName = \"parse\" + Date.parseFunctions.count++;\n\
    var regexNum = Date.parseRegexes.length;\n\
    var currentGroup = 1;\n\
    Date.parseFunctions[format] = funcName;\n\
\n\
    var code = \"Date.\" + funcName + \" = function(input){\\n\"\n\
        + \"var y = -1, m = -1, d = -1, h = -1, i = -1, s = -1;\\n\"\n\
        + \"var d = new Date();\\n\"\n\
        + \"y = d.getFullYear();\\n\"\n\
        + \"m = d.getMonth();\\n\"\n\
        + \"d = d.getDate();\\n\"\n\
        + \"var results = input.match(Date.parseRegexes[\" + regexNum + \"]);\\n\"\n\
        + \"if (results && results.length > 0) {\"\n\
    var regex = \"\";\n\
\n\
    var special = false;\n\
    var ch = '';\n\
    for (var i = 0; i < format.length; ++i) {\n\
        ch = format.charAt(i);\n\
        if (!special && ch == \"\\\\\") {\n\
            special = true;\n\
        }\n\
        else if (special) {\n\
            special = false;\n\
            regex += String.escape(ch);\n\
        }\n\
        else {\n\
            obj = Date.formatCodeToRegex(ch, currentGroup);\n\
            currentGroup += obj.g;\n\
            regex += obj.s;\n\
            if (obj.g && obj.c) {\n\
                code += obj.c;\n\
            }\n\
        }\n\
    }\n\
\n\
    code += \"if (y > 0 && m >= 0 && d > 0 && h >= 0 && i >= 0 && s >= 0)\\n\"\n\
        + \"{return new Date(y, m, d, h, i, s);}\\n\"\n\
        + \"else if (y > 0 && m >= 0 && d > 0 && h >= 0 && i >= 0)\\n\"\n\
        + \"{return new Date(y, m, d, h, i);}\\n\"\n\
        + \"else if (y > 0 && m >= 0 && d > 0 && h >= 0)\\n\"\n\
        + \"{return new Date(y, m, d, h);}\\n\"\n\
        + \"else if (y > 0 && m >= 0 && d > 0)\\n\"\n\
        + \"{return new Date(y, m, d);}\\n\"\n\
        + \"else if (y > 0 && m >= 0)\\n\"\n\
        + \"{return new Date(y, m);}\\n\"\n\
        + \"else if (y > 0)\\n\"\n\
        + \"{return new Date(y);}\\n\"\n\
        + \"}return null;}\";\n\
\n\
    Date.parseRegexes[regexNum] = new RegExp(\"^\" + regex + \"$\");\n\
    eval(code);\n\
}\n\
\n\
Date.formatCodeToRegex = function(character, currentGroup) {\n\
    switch (character) {\n\
    case \"D\":\n\
        return {g:0,\n\
        c:null,\n\
        s:\"(?:Sun|Mon|Tue|Wed|Thu|Fri|Sat)\"};\n\
    case \"j\":\n\
    case \"d\":\n\
        return {g:1,\n\
            c:\"d = parseInt(results[\" + currentGroup + \"], 10);\\n\",\n\
            s:\"(\\\\d{1,2})\"};\n\
    case \"l\":\n\
        return {g:0,\n\
            c:null,\n\
            s:\"(?:\" + Date.dayNames.join(\"|\") + \")\"};\n\
    case \"S\":\n\
        return {g:0,\n\
            c:null,\n\
            s:\"(?:st|nd|rd|th)\"};\n\
    case \"w\":\n\
        return {g:0,\n\
            c:null,\n\
            s:\"\\\\d\"};\n\
    case \"z\":\n\
        return {g:0,\n\
            c:null,\n\
            s:\"(?:\\\\d{1,3})\"};\n\
    case \"W\":\n\
        return {g:0,\n\
            c:null,\n\
            s:\"(?:\\\\d{2})\"};\n\
    case \"F\":\n\
        return {g:1,\n\
            c:\"m = parseInt(Date.monthNumbers[results[\" + currentGroup + \"].substring(0, 3)], 10);\\n\",\n\
            s:\"(\" + Date.monthNames.join(\"|\") + \")\"};\n\
    case \"M\":\n\
        return {g:1,\n\
            c:\"m = parseInt(Date.monthNumbers[results[\" + currentGroup + \"]], 10);\\n\",\n\
            s:\"(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\"};\n\
    case \"n\":\n\
    case \"m\":\n\
        return {g:1,\n\
            c:\"m = parseInt(results[\" + currentGroup + \"], 10) - 1;\\n\",\n\
            s:\"(\\\\d{1,2})\"};\n\
    case \"t\":\n\
        return {g:0,\n\
            c:null,\n\
            s:\"\\\\d{1,2}\"};\n\
    case \"L\":\n\
        return {g:0,\n\
            c:null,\n\
            s:\"(?:1|0)\"};\n\
    case \"Y\":\n\
        return {g:1,\n\
            c:\"y = parseInt(results[\" + currentGroup + \"], 10);\\n\",\n\
            s:\"(\\\\d{4})\"};\n\
    case \"y\":\n\
        return {g:1,\n\
            c:\"var ty = parseInt(results[\" + currentGroup + \"], 10);\\n\"\n\
                + \"y = ty > Date.y2kYear ? 1900 + ty : 2000 + ty;\\n\",\n\
            s:\"(\\\\d{1,2})\"};\n\
    case \"a\":\n\
        return {g:1,\n\
            c:\"if (results[\" + currentGroup + \"] == 'am') {\\n\"\n\
                + \"if (h == 12) { h = 0; }\\n\"\n\
                + \"} else { if (h < 12) { h += 12; }}\",\n\
            s:\"(am|pm)\"};\n\
    case \"A\":\n\
        return {g:1,\n\
            c:\"if (results[\" + currentGroup + \"] == 'AM') {\\n\"\n\
                + \"if (h == 12) { h = 0; }\\n\"\n\
                + \"} else { if (h < 12) { h += 12; }}\",\n\
            s:\"(AM|PM)\"};\n\
    case \"g\":\n\
    case \"G\":\n\
    case \"h\":\n\
    case \"H\":\n\
        return {g:1,\n\
            c:\"h = parseInt(results[\" + currentGroup + \"], 10);\\n\",\n\
            s:\"(\\\\d{1,2})\"};\n\
    case \"i\":\n\
        return {g:1,\n\
            c:\"i = parseInt(results[\" + currentGroup + \"], 10);\\n\",\n\
            s:\"(\\\\d{2})\"};\n\
    case \"s\":\n\
        return {g:1,\n\
            c:\"s = parseInt(results[\" + currentGroup + \"], 10);\\n\",\n\
            s:\"(\\\\d{2})\"};\n\
    case \"O\":\n\
        return {g:0,\n\
            c:null,\n\
            s:\"[+-]\\\\d{4}\"};\n\
    case \"T\":\n\
        return {g:0,\n\
            c:null,\n\
            s:\"[A-Z]{3}\"};\n\
    case \"Z\":\n\
        return {g:0,\n\
            c:null,\n\
            s:\"[+-]\\\\d{1,5}\"};\n\
    default:\n\
        return {g:0,\n\
            c:null,\n\
            s:String.escape(character)};\n\
    }\n\
}\n\
\n\
Date.prototype.getTimezone = function() {\n\
    return this.toString().replace(\n\
        /^.*? ([A-Z]{3}) [0-9]{4}.*$/, \"$1\").replace(\n\
        /^.*?\\(([A-Z])[a-z]+ ([A-Z])[a-z]+ ([A-Z])[a-z]+\\)$/, \"$1$2$3\");\n\
}\n\
\n\
Date.prototype.getGMTOffset = function() {\n\
    return (this.getTimezoneOffset() > 0 ? \"-\" : \"+\")\n\
        + String.leftPad(Math.floor(this.getTimezoneOffset() / 60), 2, \"0\")\n\
        + String.leftPad(this.getTimezoneOffset() % 60, 2, \"0\");\n\
}\n\
\n\
Date.prototype.getDayOfYear = function() {\n\
    var num = 0;\n\
    Date.daysInMonth[1] = this.isLeapYear() ? 29 : 28;\n\
    for (var i = 0; i < this.getMonth(); ++i) {\n\
        num += Date.daysInMonth[i];\n\
    }\n\
    return num + this.getDate() - 1;\n\
}\n\
\n\
Date.prototype.getWeekOfYear = function() {\n\
    // Skip to Thursday of this week\n\
    var now = this.getDayOfYear() + (4 - this.getDay());\n\
    // Find the first Thursday of the year\n\
    var jan1 = new Date(this.getFullYear(), 0, 1);\n\
    var then = (7 - jan1.getDay() + 4);\n\
    document.write(then);\n\
    return String.leftPad(((now - then) / 7) + 1, 2, \"0\");\n\
}\n\
\n\
Date.prototype.isLeapYear = function() {\n\
    var year = this.getFullYear();\n\
    return ((year & 3) == 0 && (year % 100 || (year % 400 == 0 && year)));\n\
}\n\
\n\
Date.prototype.getFirstDayOfMonth = function() {\n\
    var day = (this.getDay() - (this.getDate() - 1)) % 7;\n\
    return (day < 0) ? (day + 7) : day;\n\
}\n\
\n\
Date.prototype.getLastDayOfMonth = function() {\n\
    var day = (this.getDay() + (Date.daysInMonth[this.getMonth()] - this.getDate())) % 7;\n\
    return (day < 0) ? (day + 7) : day;\n\
}\n\
\n\
Date.prototype.getDaysInMonth = function() {\n\
    Date.daysInMonth[1] = this.isLeapYear() ? 29 : 28;\n\
    return Date.daysInMonth[this.getMonth()];\n\
}\n\
\n\
Date.prototype.getSuffix = function() {\n\
    switch (this.getDate()) {\n\
        case 1:\n\
        case 21:\n\
        case 31:\n\
            return \"st\";\n\
        case 2:\n\
        case 22:\n\
            return \"nd\";\n\
        case 3:\n\
        case 23:\n\
            return \"rd\";\n\
        default:\n\
            return \"th\";\n\
    }\n\
}\n\
\n\
String.escape = function(string) {\n\
    return string.replace(/('|\\\\)/g, \"\\\\$1\");\n\
}\n\
\n\
String.leftPad = function (val, size, ch) {\n\
    var result = new String(val);\n\
    if (ch == null) {\n\
        ch = \" \";\n\
    }\n\
    while (result.length < size) {\n\
        result = ch + result;\n\
    }\n\
    return result;\n\
}\n\
\n\
Date.daysInMonth = [31,28,31,30,31,30,31,31,30,31,30,31];\n\
Date.monthNames =\n\
   [\"January\",\n\
    \"February\",\n\
    \"March\",\n\
    \"April\",\n\
    \"May\",\n\
    \"June\",\n\
    \"July\",\n\
    \"August\",\n\
    \"September\",\n\
    \"October\",\n\
    \"November\",\n\
    \"December\"];\n\
Date.dayNames =\n\
   [\"Sunday\",\n\
    \"Monday\",\n\
    \"Tuesday\",\n\
    \"Wednesday\",\n\
    \"Thursday\",\n\
    \"Friday\",\n\
    \"Saturday\"];\n\
Date.y2kYear = 50;\n\
Date.monthNumbers = {\n\
    Jan:0,\n\
    Feb:1,\n\
    Mar:2,\n\
    Apr:3,\n\
    May:4,\n\
    Jun:5,\n\
    Jul:6,\n\
    Aug:7,\n\
    Sep:8,\n\
    Oct:9,\n\
    Nov:10,\n\
    Dec:11};\n\
Date.patterns = {\n\
    ISO8601LongPattern:\"Y-m-d H:i:s\",\n\
    ISO8601ShortPattern:\"Y-m-d\",\n\
    ShortDatePattern: \"n/j/Y\",\n\
    LongDatePattern: \"l, F d, Y\",\n\
    FullDateTimePattern: \"l, F d, Y g:i:s A\",\n\
    MonthDayPattern: \"F d\",\n\
    ShortTimePattern: \"g:i A\",\n\
    LongTimePattern: \"g:i:s A\",\n\
    SortableDateTimePattern: \"Y-m-d\\\\TH:i:s\",\n\
    UniversalSortableDateTimePattern: \"Y-m-d H:i:sO\",\n\
    YearMonthPattern: \"F, Y\"};\n\
\n\
var date = new Date(\"1/1/2007 1:11:11\");\n\
\n\
for (i = 0; i < 4000; ++i) {\n\
    var shortFormat = date.dateFormat(\"Y-m-d\");\n\
    var longFormat = date.dateFormat(\"l, F d, Y g:i:s A\");\n\
    date.setTime(date.getTime() + 84266956);\n\
}\n\
\n\
// FIXME: Find a way to validate this test.\n\
// https://bugs.webkit.org/show_bug.cgi?id=114849\n\
\n\
", "\
/*\n\
 * Copyright (C) Rich Moore.  All rights reserved.\n\
 *\n\
 * Redistribution and use in source and binary forms, with or without\n\
 * modification, are permitted provided that the following conditions\n\
 * are met:\n\
 * 1. Redistributions of source code must retain the above copyright\n\
 *    notice, this list of conditions and the following disclaimer.\n\
 * 2. Redistributions in binary form must reproduce the above copyright\n\
 *    notice, this list of conditions and the following disclaimer in the\n\
 *    documentation and/or other materials provided with the distribution.\n\
 *\n\
 * THIS SOFTWARE IS PROVIDED BY CONTRIBUTORS ``AS IS'' AND ANY\n\
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n\
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR\n\
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR\n\
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,\n\
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,\n\
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR\n\
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY\n\
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\n\
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n\
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. \n\
 */\n\
\n\
/////. Start CORDIC\n\
\n\
var AG_CONST = 0.6072529350;\n\
\n\
function FIXED(X)\n\
{\n\
  return X * 65536.0;\n\
}\n\
\n\
function FLOAT(X)\n\
{\n\
  return X / 65536.0;\n\
}\n\
\n\
function DEG2RAD(X)\n\
{\n\
  return 0.017453 * (X);\n\
}\n\
\n\
var Angles = [\n\
  FIXED(45.0), FIXED(26.565), FIXED(14.0362), FIXED(7.12502),\n\
  FIXED(3.57633), FIXED(1.78991), FIXED(0.895174), FIXED(0.447614),\n\
  FIXED(0.223811), FIXED(0.111906), FIXED(0.055953),\n\
  FIXED(0.027977) \n\
              ];\n\
\n\
var Target = 28.027;\n\
\n\
function cordicsincos(Target) {\n\
    var X;\n\
    var Y;\n\
    var TargetAngle;\n\
    var CurrAngle;\n\
    var Step;\n\
 \n\
    X = FIXED(AG_CONST);         /* AG_CONST * cos(0) */\n\
    Y = 0;                       /* AG_CONST * sin(0) */\n\
\n\
    TargetAngle = FIXED(Target);\n\
    CurrAngle = 0;\n\
    for (Step = 0; Step < 12; Step++) {\n\
        var NewX;\n\
        if (TargetAngle > CurrAngle) {\n\
            NewX = X - (Y >> Step);\n\
            Y = (X >> Step) + Y;\n\
            X = NewX;\n\
            CurrAngle += Angles[Step];\n\
        } else {\n\
            NewX = X + (Y >> Step);\n\
            Y = -(X >> Step) + Y;\n\
            X = NewX;\n\
            CurrAngle -= Angles[Step];\n\
        }\n\
    }\n\
\n\
    return FLOAT(X) * FLOAT(Y);\n\
}\n\
\n\
///// End CORDIC\n\
\n\
var total = 0;\n\
\n\
function cordic( runs ) {\n\
  var start = new Date();\n\
\n\
  for ( var i = 0 ; i < runs ; i++ ) {\n\
      total += cordicsincos(Target);\n\
  }\n\
\n\
  var end = new Date();\n\
\n\
  return end.getTime() - start.getTime();\n\
}\n\
\n\
cordic(25000);\n\
\n\
var expected = 10362.570468755888;\n\
\n\
if (total != expected)\n\
    throw \"ERROR: bad result: expected \" + expected + \" but got \" + total;\n\
\n\
\n\
", "\
// The Computer Language Shootout\n\
// http://shootout.alioth.debian.org/\n\
// contributed by Isaac Gouy\n\
\n\
function partial(n){\n\
    var a1 = a2 = a3 = a4 = a5 = a6 = a7 = a8 = a9 = 0.0;\n\
    var twothirds = 2.0/3.0;\n\
    var alt = -1.0;\n\
    var k2 = k3 = sk = ck = 0.0;\n\
    \n\
    for (var k = 1; k <= n; k++){\n\
        k2 = k*k;\n\
        k3 = k2*k;\n\
        sk = Math.sin(k);\n\
        ck = Math.cos(k);\n\
        alt = -alt;\n\
        \n\
        a1 += Math.pow(twothirds,k-1);\n\
        a2 += Math.pow(k,-0.5);\n\
        a3 += 1.0/(k*(k+1.0));\n\
        a4 += 1.0/(k3 * sk*sk);\n\
        a5 += 1.0/(k3 * ck*ck);\n\
        a6 += 1.0/k;\n\
        a7 += 1.0/k2;\n\
        a8 += alt/k;\n\
        a9 += alt/(2*k -1);\n\
    }\n\
    \n\
    // NOTE: We don't try to validate anything from pow(),  sin() or cos() because those aren't\n\
    // well-specified in ECMAScript.\n\
    return a6 + a7 + a8 + a9;\n\
}\n\
\n\
var total = 0;\n\
\n\
for (var i = 1024; i <= 16384; i *= 2) {\n\
    total += partial(i);\n\
}\n\
\n\
var expected = 60.08994194659945;\n\
\n\
if (total != expected) {\n\
    throw \"ERROR: bad result: expected \" + expected + \" but got \" + total;\n\
}\n\
\n\
\n\
", "\
// The Great Computer Language Shootout\n\
// http://shootout.alioth.debian.org/\n\
//\n\
// contributed by Ian Osgood\n\
\n\
function A(i,j) {\n\
  return 1/((i+j)*(i+j+1)/2+i+1);\n\
}\n\
\n\
function Au(u,v) {\n\
  for (var i=0; i<u.length; ++i) {\n\
    var t = 0;\n\
    for (var j=0; j<u.length; ++j)\n\
      t += A(i,j) * u[j];\n\
    v[i] = t;\n\
  }\n\
}\n\
\n\
function Atu(u,v) {\n\
  for (var i=0; i<u.length; ++i) {\n\
    var t = 0;\n\
    for (var j=0; j<u.length; ++j)\n\
      t += A(j,i) * u[j];\n\
    v[i] = t;\n\
  }\n\
}\n\
\n\
function AtAu(u,v,w) {\n\
  Au(u,w);\n\
  Atu(w,v);\n\
}\n\
\n\
function spectralnorm(n) {\n\
  var i, u=[], v=[], w=[], vv=0, vBv=0;\n\
  for (i=0; i<n; ++i) {\n\
    u[i] = 1; v[i] = w[i] = 0;\n\
  }\n\
  for (i=0; i<10; ++i) {\n\
    AtAu(u,v,w);\n\
    AtAu(v,u,w);\n\
  }\n\
  for (i=0; i<n; ++i) {\n\
    vBv += u[i]*v[i];\n\
    vv  += v[i]*v[i];\n\
  }\n\
  return Math.sqrt(vBv/vv);\n\
}\n\
\n\
var total = 0;\n\
\n\
for (var i = 6; i <= 48; i *= 2) {\n\
    total += spectralnorm(i);\n\
}\n\
\n\
var expected = 5.086694231303284;\n\
\n\
if (total != expected)\n\
    throw \"ERROR: bad result: expected \" + expected + \" but got \" + total;\n\
\n\
\n\
", /* regexp-dna removed */ "", "\
/* ***** BEGIN LICENSE BLOCK *****\n\
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1\n\
 *\n\
 * The contents of this file are subject to the Mozilla Public License Version\n\
 * 1.1 (the \"License\"); you may not use this file except in compliance with\n\
 * the License. You may obtain a copy of the License at\n\
 * http://www.mozilla.org/MPL/\n\
 *\n\
 * Software distributed under the License is distributed on an \"AS IS\" basis,\n\
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License\n\
 * for the specific language governing rights and limitations under the\n\
 * License.\n\
 *\n\
 * The Original Code is Mozilla XML-RPC Client component.\n\
 *\n\
 * The Initial Developer of the Original Code is\n\
 * Digital Creations 2, Inc.\n\
 * Portions created by the Initial Developer are Copyright (C) 2000\n\
 * the Initial Developer. All Rights Reserved.\n\
 *\n\
 * Contributor(s):\n\
 *   Martijn Pieters <mj@digicool.com> (original author)\n\
 *   Samuel Sieb <samuel@sieb.net>\n\
 *\n\
 * Alternatively, the contents of this file may be used under the terms of\n\
 * either the GNU General Public License Version 2 or later (the \"GPL\"), or\n\
 * the GNU Lesser General Public License Version 2.1 or later (the \"LGPL\"),\n\
 * in which case the provisions of the GPL or the LGPL are applicable instead\n\
 * of those above. If you wish to allow use of your version of this file only\n\
 * under the terms of either the GPL or the LGPL, and not to allow others to\n\
 * use your version of this file under the terms of the MPL, indicate your\n\
 * decision by deleting the provisions above and replace them with the notice\n\
 * and other provisions required by the GPL or the LGPL. If you do not delete\n\
 * the provisions above, a recipient may use your version of this file under\n\
 * the terms of any one of the MPL, the GPL or the LGPL.\n\
 *\n\
 * ***** END LICENSE BLOCK ***** */\n\
\n\
// From: http://lxr.mozilla.org/mozilla/source/extensions/xml-rpc/src/nsXmlRpcClient.js#956\n\
\n\
/* Convert data (an array of integers) to a Base64 string. */\n\
var toBase64Table = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';\n\
var base64Pad = '=';\n\
\n\
function toBase64(data) {\n\
    var result = '';\n\
    var length = data.length;\n\
    var i;\n\
    // Convert every three bytes to 4 ascii characters.\n\
    for (i = 0; i < (length - 2); i += 3) {\n\
        result += toBase64Table[data.charCodeAt(i) >> 2];\n\
        result += toBase64Table[((data.charCodeAt(i) & 0x03) << 4) + (data.charCodeAt(i+1) >> 4)];\n\
        result += toBase64Table[((data.charCodeAt(i+1) & 0x0f) << 2) + (data.charCodeAt(i+2) >> 6)];\n\
        result += toBase64Table[data.charCodeAt(i+2) & 0x3f];\n\
    }\n\
\n\
    // Convert the remaining 1 or 2 bytes, pad out to 4 characters.\n\
    if (length%3) {\n\
        i = length - (length%3);\n\
        result += toBase64Table[data.charCodeAt(i) >> 2];\n\
        if ((length%3) == 2) {\n\
            result += toBase64Table[((data.charCodeAt(i) & 0x03) << 4) + (data.charCodeAt(i+1) >> 4)];\n\
            result += toBase64Table[(data.charCodeAt(i+1) & 0x0f) << 2];\n\
            result += base64Pad;\n\
        } else {\n\
            result += toBase64Table[(data.charCodeAt(i) & 0x03) << 4];\n\
            result += base64Pad + base64Pad;\n\
        }\n\
    }\n\
\n\
    return result;\n\
}\n\
\n\
/* Convert Base64 data to a string */\n\
var toBinaryTable = [\n\
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,\n\
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,\n\
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,62, -1,-1,-1,63,\n\
    52,53,54,55, 56,57,58,59, 60,61,-1,-1, -1, 0,-1,-1,\n\
    -1, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,\n\
    15,16,17,18, 19,20,21,22, 23,24,25,-1, -1,-1,-1,-1,\n\
    -1,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,\n\
    41,42,43,44, 45,46,47,48, 49,50,51,-1, -1,-1,-1,-1\n\
];\n\
\n\
function base64ToString(data) {\n\
    var result = '';\n\
    var leftbits = 0; // number of bits decoded, but yet to be appended\n\
    var leftdata = 0; // bits decoded, but yet to be appended\n\
\n\
    // Convert one by one.\n\
    for (var i = 0; i < data.length; i++) {\n\
        var c = toBinaryTable[data.charCodeAt(i) & 0x7f];\n\
        var padding = (data.charCodeAt(i) == base64Pad.charCodeAt(0));\n\
        // Skip illegal characters and whitespace\n\
        if (c == -1) continue;\n\
        \n\
        // Collect data into leftdata, update bitcount\n\
        leftdata = (leftdata << 6) | c;\n\
        leftbits += 6;\n\
\n\
        // If we have 8 or more bits, append 8 bits to the result\n\
        if (leftbits >= 8) {\n\
            leftbits -= 8;\n\
            // Append if not padding.\n\
            if (!padding)\n\
                result += String.fromCharCode((leftdata >> leftbits) & 0xff);\n\
            leftdata &= (1 << leftbits) - 1;\n\
        }\n\
    }\n\
\n\
    // If there are any bits left, the base64 string was corrupted\n\
    if (leftbits)\n\
        throw Components.Exception('Corrupted base64 string');\n\
\n\
    return result;\n\
}\n\
\n\
var str = \"\";\n\
\n\
for ( var i = 0; i < 8192; i++ )\n\
        str += String.fromCharCode( (25 * Math.random()) + 97 );\n\
\n\
for ( var i = 8192; i <= 16384; i *= 2 ) {\n\
\n\
    var base64;\n\
\n\
    base64 = toBase64(str);\n\
    var encoded = base64ToString(base64);\n\
    if (encoded != str)\n\
        throw \"ERROR: bad result: expected \" + str + \" but got \" + encoded;\n\
\n\
    // Double the string\n\
    str += str;\n\
}\n\
\n\
toBinaryTable = null;\n\
\n\
", "\
// The Great Computer Language Shootout\n\
//  http://shootout.alioth.debian.org\n\
//\n\
//  Contributed by Ian Osgood\n\
\n\
var last = 42, A = 3877, C = 29573, M = 139968;\n\
\n\
function rand(max) {\n\
  last = (last * A + C) % M;\n\
  return max * last / M;\n\
}\n\
\n\
var ALU =\n\
  \"GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGG\" +\n\
  \"GAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGA\" +\n\
  \"CCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAAT\" +\n\
  \"ACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCA\" +\n\
  \"GCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGG\" +\n\
  \"AGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCC\" +\n\
  \"AGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA\";\n\
\n\
var IUB = {\n\
  a:0.27, c:0.12, g:0.12, t:0.27,\n\
  B:0.02, D:0.02, H:0.02, K:0.02,\n\
  M:0.02, N:0.02, R:0.02, S:0.02,\n\
  V:0.02, W:0.02, Y:0.02\n\
}\n\
\n\
var HomoSap = {\n\
  a: 0.3029549426680,\n\
  c: 0.1979883004921,\n\
  g: 0.1975473066391,\n\
  t: 0.3015094502008\n\
}\n\
\n\
function makeCumulative(table) {\n\
  var last = null;\n\
  for (var c in table) {\n\
    if (last) table[c] += table[last];\n\
    last = c;\n\
  }\n\
}\n\
\n\
function fastaRepeat(n, seq) {\n\
  var seqi = 0, lenOut = 60;\n\
  while (n>0) {\n\
    if (n<lenOut) lenOut = n;\n\
    if (seqi + lenOut < seq.length) {\n\
      ret += seq.substring(seqi, seqi+lenOut).length;\n\
      seqi += lenOut;\n\
    } else {\n\
      var s = seq.substring(seqi);\n\
      seqi = lenOut - s.length;\n\
      ret += (s + seq.substring(0, seqi)).length;\n\
    }\n\
    n -= lenOut;\n\
  }\n\
}\n\
\n\
function fastaRandom(n, table) {\n\
  var line = new Array(60);\n\
  makeCumulative(table);\n\
  while (n>0) {\n\
    if (n<line.length) line = new Array(n);\n\
    for (var i=0; i<line.length; i++) {\n\
      var r = rand(1);\n\
      for (var c in table) {\n\
        if (r < table[c]) {\n\
          line[i] = c;\n\
          break;\n\
        }\n\
      }\n\
    }\n\
    ret += line.join('').length;\n\
    n -= line.length;\n\
  }\n\
}\n\
\n\
var ret = 0;\n\
\n\
var count = 7;\n\
fastaRepeat(2*count*100000, ALU);\n\
fastaRandom(3*count*1000, IUB);\n\
fastaRandom(5*count*1000, HomoSap);\n\
\n\
var expected = 1456000;\n\
\n\
if (ret != expected)\n\
    throw \"ERROR: bad result: expected \" + expected + \" but got \" + ret;\n\
\n\
\n\
", /* string-tagcloud removed */ "", /* string-unpack-code removed */ "", "\
letters = new Array(\"a\",\"b\",\"c\",\"d\",\"e\",\"f\",\"g\",\"h\",\"i\",\"j\",\"k\",\"l\",\"m\",\"n\",\"o\",\"p\",\"q\",\"r\",\"s\",\"t\",\"u\",\"v\",\"w\",\"x\",\"y\",\"z\");\n\
numbers = new Array(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26);\n\
colors  = new Array(\"FF\",\"CC\",\"99\",\"66\",\"33\",\"00\");\n\
\n\
var endResult;\n\
\n\
function doTest()\n\
{\n\
   endResult = \"\";\n\
\n\
   // make up email address\n\
   for (var k=0;k<4000;k++)\n\
   {\n\
      username = makeName(6);\n\
      (k%2)?email=username+\"@mac.com\":email=username+\"(at)mac.com\";\n\
\n\
      // validate the email address\n\
      var pattern = /^[a-zA-Z0-9\\-\\._]+@[a-zA-Z0-9\\-_]+(\\.?[a-zA-Z0-9\\-_]*)\\.[a-zA-Z]{2,3}$/;\n\
\n\
      if(pattern.test(email))\n\
      {\n\
         var r = email + \" appears to be a valid email address.\";\n\
         addResult(r);\n\
      }\n\
      else\n\
      {\n\
         r = email + \" does NOT appear to be a valid email address.\";\n\
         addResult(r);\n\
      }\n\
   }\n\
\n\
   // make up ZIP codes\n\
   for (var s=0;s<4000;s++)\n\
   {\n\
      var zipGood = true;\n\
      var zip = makeNumber(4);\n\
      (s%2)?zip=zip+\"xyz\":zip=zip.concat(\"7\");\n\
\n\
      // validate the zip code\n\
      for (var i = 0; i < zip.length; i++) {\n\
          var ch = zip.charAt(i);\n\
          if (ch < \"0\" || ch > \"9\") {\n\
              zipGood = false;\n\
              r = zip + \" contains letters.\";\n\
              addResult(r);\n\
          }\n\
      }\n\
      if (zipGood && zip.length>5)\n\
      {\n\
         zipGood = false;\n\
         r = zip + \" is longer than five characters.\";\n\
         addResult(r);\n\
      }\n\
      if (zipGood)\n\
      {\n\
         r = zip + \" appears to be a valid ZIP code.\";\n\
         addResult(r);\n\
      }\n\
   }\n\
}\n\
\n\
function makeName(n)\n\
{\n\
   var tmp = \"\";\n\
   for (var i=0;i<n;i++)\n\
   {\n\
      var l = Math.floor(26*Math.random());\n\
      tmp += letters[l];\n\
   }\n\
   return tmp;\n\
}\n\
\n\
function makeNumber(n)\n\
{\n\
   var tmp = \"\";\n\
   for (var i=0;i<n;i++)\n\
   {\n\
      var l = Math.floor(9*Math.random());\n\
      tmp = tmp.concat(l);\n\
   }\n\
   return tmp;\n\
}\n\
\n\
function addResult(r)\n\
{\n\
   endResult += \"\\n\" + r;\n\
}\n\
\n\
doTest();\n\
\n\
// FIXME: Come up with a way of validating this test.\n\
// https://bugs.webkit.org/show_bug.cgi?id=114851\n\
\n\
" };
