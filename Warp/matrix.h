/*
   Definitions for Matrix manipulation routines

   D. H. House  10/26/94
*/



#include <cstdio>
#include <cmath>
#include <cstring>
#include <iostream>	
#include <iomanip>
#include <cstdlib>


#ifndef PI
#define PI		3.1415926536
#endif

using namespace std;

struct Vector3D{
  double x, y, w;
};

struct Vector2D{
  double x, y;
};

class Matrix3D{
private:
  double M[3][3];
  
public:
  Matrix3D();
  Matrix3D(const double coefs[3][3]);
  Matrix3D(const Matrix3D &mat);

  void print() const;

  void setidentity();
  void set(const double coefs[3][3]);
  
  double determinant() const;
  Matrix3D adjoint() const;
  Matrix3D inverse() const;
  
  Vector2D operator*(const Vector2D &v) const;
  Matrix3D operator*(const Matrix3D &m2) const;
  double *operator[](int i);
};

struct BilinearCoeffs{
  double width, height;
  double a0, a1, a2, a3;
  double b0, b1, b2, b3;
  double c2;
};

void setbilinear(double width, double height,
		 Vector2D xycorners[4], BilinearCoeffs &coeff);
void invbilinear(const BilinearCoeffs &c, Vector2D xy, Vector2D &uv);

Matrix3D::Matrix3D(){
  setidentity();
}

Matrix3D::Matrix3D(const double coefs[3][3]){
  set(coefs);
}

Matrix3D::Matrix3D(const Matrix3D &mat){
  set(mat.M);
}

/*
   Print the contents of a 3x3 transformation matrix
*/
void Matrix3D::print()const{

  printf("\n");
  for(int row = 0; row < 3; row++){
    printf("%8.4f %8.4f %8.4f\n", M[row][0], M[row][1], M[row][2]);
  }
  printf("\n");
}

/*
   Set the matrix m to the indentity matrix
*/
void Matrix3D::setidentity(){

  M[0][0] = M[1][1] = M[2][2] = 1.0;
  M[0][1] = M[0][2] = 0.0;
  M[1][0] = M[1][2] = 0.0;
  M[2][0] = M[2][1] = 0.0;
}

/*
 Set the matrix m to the contents of a 3x3 array
 */
void Matrix3D::set(const double coefs[3][3]){
  
  for(int row = 0; row < 3; row++)
	for(int col = 0; col < 3; col++)
	  M[row][col] = coefs[row][col];
}

/*
   Function to compute and return the determinant of the 3x3 matrix m.
*/
double Matrix3D::determinant()const{
  double det;

  det  = M[0][0] * M[1][1] * M[2][2];
  det += M[0][1] * M[1][2] * M[2][0];
  det += M[0][2] * M[2][1] * M[1][0];
  det -= M[2][0] * M[1][1] * M[0][2];
  det -= M[1][0] * M[0][1] * M[2][2];
  det -= M[0][0] * M[1][2] * M[2][1];

  return det;
}

/*
   Compute the adjoint of 3x3 matrix m and place the result in 3x3
   matrix adj.
*/
Matrix3D Matrix3D::adjoint()const{
  Matrix3D adj;
  
  adj.M[0][0] = (M[1][1] * M[2][2] - M[1][2] * M[2][1]);
  adj.M[0][1] = (M[0][2] * M[2][1] - M[0][1] * M[2][2]);
  adj.M[0][2] = (M[0][1] * M[1][2] - M[0][2] * M[1][1]);
  adj.M[1][0] = (M[1][2] * M[2][0] - M[1][0] * M[2][2]);
  adj.M[1][1] = (M[0][0] * M[2][2] - M[0][2] * M[2][0]);
  adj.M[1][2] = (M[0][2] * M[1][0] - M[0][0] * M[1][2]);
  adj.M[2][0] = (M[1][0] * M[2][1] - M[1][1] * M[2][0]);
  adj.M[2][1] = (M[0][1] * M[2][0] - M[0][0] * M[2][1]);
  adj.M[2][2] = (M[0][0] * M[1][1] - M[0][1] * M[1][0]);
  
  return adj;
}

/*
   Compute the inverse of 3x3 matrix m and place the result in 3x3
   matrix inv.
*/
Matrix3D Matrix3D::inverse()const{
  Matrix3D inv;  
  double det = determinant();

  inv.M[0][0] = (M[1][1] * M[2][2] - M[1][2] * M[2][1]) / det;
  inv.M[0][1] = (M[0][2] * M[2][1] - M[0][1] * M[2][2]) / det;
  inv.M[0][2] = (M[0][1] * M[1][2] - M[0][2] * M[1][1]) / det;
  inv.M[1][0] = (M[1][2] * M[2][0] - M[1][0] * M[2][2]) / det;
  inv.M[1][1] = (M[0][0] * M[2][2] - M[0][2] * M[2][0]) / det;
  inv.M[1][2] = (M[0][2] * M[1][0] - M[0][0] * M[1][2]) / det;
  inv.M[2][0] = (M[1][0] * M[2][1] - M[1][1] * M[2][0]) / det;
  inv.M[2][1] = (M[0][1] * M[2][0] - M[0][0] * M[2][1]) / det;
  inv.M[2][2] = (M[0][0] * M[1][1] - M[0][1] * M[1][0]) / det;
  
  return inv;
}

/*
 Transform the 2D vector v by by the 3x3 matrix m.  The process is to
 1) extend v by adding a w coordinate of 1.0,
 2) premultiply this 3D vector by m,
 3) scale the resulting vector by 1/w,
 4) Place the x and y coordinates of the result in vout.
 */
Vector2D Matrix3D::operator*(const Vector2D &v)const{
  Vector3D vw;
  Vector2D vout;
  
  vw.x =  M[0][0] * v.x + M[0][1] * v.y + M[0][2];
  vw.y =  M[1][0] * v.x + M[1][1] * v.y + M[1][2];
  vw.w =  M[2][0] * v.x + M[2][1] * v.y + M[2][2];
  
  if(vw.w == 0.0){
    fprintf(stderr, "WARNING: w coordinate of 0!\n");
    vout.x = vw.x;
    vout.y = vw.y;
  }
  else if(vw.w == 1.0){
    vout.x = vw.x;
    vout.y = vw.y;
  }
  else{
    vout.x = vw.x / vw.w;
    vout.y = vw.y / vw.w;
  }
  
  return vout;
}

/*
 Multiply 3x3 matrix m1 by 3x3 matrix m2 and place the result in
 3x3 matrix m3.  I.e. m3 = m1 * m2
 */
Matrix3D Matrix3D::operator*(const Matrix3D &m2)const{
  double sum;
  int row, col, rc;
  Matrix3D prod;	/* make internal copy in case m3 is m1 or m2 */
  
  for(row = 0; row < 3; row++)
    for(col = 0; col < 3; col++){
      sum = 0.0;
      for(rc = 0; rc < 3; rc++)
		sum += M[row][rc] * m2.M[rc][col];
      prod.M[row][col] = sum;
    }
  
  return prod;
}

double *Matrix3D::operator[](int i){
  return (double *)&(M[i]);
}
