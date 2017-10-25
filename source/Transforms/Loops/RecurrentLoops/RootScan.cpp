#include <math.h>

typedef double number;

inline int imax(int i, int j)
{
 if (i>j) return i;
 return j;
};



inline number nabs(number x)
{
 if (x<0) return -x;
 return x;
};



int rootscan(number* coeff, int n)
{


 number **A;
 number *cc,*ss;//arrays of parameters for Jacobi transformations
 number eps,eps2,b,c,d,shift,nrm,t,x,y,cmax,tmpmax,anorm;//,s;

 int nn,noluck,rval,maxiter,i,j,k;//,pos;

 maxiter=30;

 eps=1e-8;
 eps2=1e-3;


 if (nabs(coeff[n-1])>=1) return 1;//The simplest estimate


//--------------Einstrom-Kakeya estimate-----------------------------


 if (coeff[0]<0 || coeff[0]>1-eps2) cmax=2;
 else
   {
     cmax=coeff[0];
     for (i=1;i<n;i++)
        {
           tmpmax=coeff[i]/coeff[i-1];
           if (tmpmax<0 || tmpmax>1-eps2)//If method cannot be applied
              {                          //Or estimate too large
                 cmax=2;
                 break;
              }
           else if (cmax<tmpmax)
                cmax=tmpmax;
        }
   }

 if (cmax<1-eps2) return 0;


//-------------------------------------------------------------------

 A=new number*[n+1];
 A[0]=new number[n*n+1];
 A[1]=A[0];
 for (i=2;i<=n;i++)
  A[i]=A[i-1]+n;



//---Initialize companion matrix and compute norm------

 anorm=0;
 for (i=1;i<=n;i++)
 {
   A[i][imax(i-1,1)]=1;
   anorm+=1;

   A[i][n]=coeff[n-i];
   anorm+=nabs(A[i][n]);

   for (j=i;j<=n-1;j++)
      A[i][j]=0;

   for (j=1;j<i-1;j++)
      A[i][j]=0;

 }

//-----------------------------------------------------
//-----------------------------------------------------

 cc=new number[n];
 ss=new number[n];


 shift=0; // Size of shift in QR
 nn=n;    // Size of matrix to analyze
 noluck=0;// Indicates how many iterations were made without success
 rval=1; // Returned value
         //0-> all roots <1 . 1-> some roots >1 or no convergence

// Begin iterations!!!
 while (nn>2)
 {
     if (noluck==maxiter)
        {
          rval=1;
          break;
        }
     noluck++;//Signalize that eigenvalue wasn't found yet


//-------Try to detect an eigenvalues------------------------------------



     if ( nabs(A[nn][nn-1])<eps*(nabs(A[nn-1][nn-1])+nabs(A[nn][nn]))) // One eigenvalue found
      {

        noluck=0;//Indicates that we've found an eigenvalue



        if (nabs(A[nn][nn])>1-eps2)
           {
             rval=1;
             break;
           }

        nn--;//Decrease the size of matrix

      }//end of if ( nabs(A[nn][nn-1])< eps)

     else if ( nabs(A[nn-1][nn-2])< eps*(nabs(A[nn-1][nn-1])+nabs(A[nn-2][nn-2]))  ) // A pair of eigenvalues found
      {

        noluck=0;//Indicates that we've found eigenvalues

        b=0.5*(A[nn-1][nn-1]+A[nn][nn]);
        c=A[nn-1][nn-1]*A[nn][nn]-A[nn][nn-1]*A[nn-1][nn];
        d=b*b-c;

        if (d>eps)  //Discriminant > 0 --- A real pair
          {


            d=sqrt(d);

            if (!((nabs(b+d)<1-eps2) &&  (nabs(b-d)<1-eps2)))
               {
                 rval=1;
                 break;
               }
          }
        else
          {
             if (d<-eps)// Discriminant<0 --- A complex pair
               {

                 if (b*b-d > 1-eps2)
                     {
                       rval=1;
                       break;
                     }
               }
             else  //Discriminant=0 --- Double real eigenvalue
               {

                  if (nabs(b)>1-eps2)
                    {
                      rval=1;
                      break;
                    }
               }

          }

        nn-=2;//Decrease the size of matrix
      }//end of else if ( nabs(A[nn-1][nn-2])<eps )
//---------------------------------------------------------------

   if (noluck)//If we didn't find an eigenvalue we make transformation
      {


//------Choose the shift--------------------------------

//We choose shift close to eigenvalues of 2x2 submatrix
//in the bottom right hand side of the matrix

       b=0.5*(A[nn-1][nn-1]+A[nn][nn]);
       c=A[nn-1][nn-1]*A[nn][nn]-A[nn][nn-1]*A[nn-1][nn];
       d=b*b-c;


       if (d<0) shift=b;//cant make a complex shift
       else
        {
          d=sqrt(d);
          if (nabs(b+d-A[nn][nn])<nabs(b-d-A[nn][nn]))
               shift=b-d;
          else shift=b+d;

        }



       if (noluck==15||noluck==25) // If too much unfortunate transformations
       {
         shift=1.5*(A[nn-1][nn-1]+A[nn][nn]);//form exceptional shift

       }
     if (shift==0) shift=anorm/2;
//------Make shift--------------------------------------


     for (j=1;j<=nn;j++)
       A[j][j]-=shift;

/******  QR TRANSFORMATION  ********************************/


//------------Loop of Jacobi transformations------------

  //-----Choose the angle of rotation---------------------
     for (i=1;i<=nn-1;i++)
       {

          if (A[i][i]==0)
            {
               ss[i]=1; //Remember the angles
               cc[i]=0;
            }//end if
            else
            {
                 t=-A[i+1][i]/A[i][i];
                 nrm=sqrt(1+t*t);
                 ss[i]=t/nrm;//Remember the angles
                 cc[i]=1/nrm;
            }//end else
           //Now c=cos(a) s=sin(a) Now transform rows

   //----Rows transformation------------------------

           A[i  ][i]=cc[i]*A[i][i]-ss[i]*A[i+1][i];
           A[i+1][i]=0;

           for (j=i+1;j<=nn;j++)
             {
               x=A[i  ][j];
               y=A[i+1][j];

               A[i]  [j]=cc[i]*x-ss[i]*y;
               A[i+1][j]=ss[i]*x+cc[i]*y;

             }//end for(j=i+1...


     }//end for(i=1;...


  //----Columns transformations---------------------
    for (i=1;i<=nn-1;i++)
       {
           for (k=1;k<=i+1;k++)
             {
               x=A[k][i  ];
               y=A[k][i+1];

               A[k][i  ]=cc[i]*x-ss[i]*y;//Rotate columns to the same angle
               A[k][i+1]=ss[i]*x+cc[i]*y;//as rows
             }
       }

//---Make reverse shift--------------------------------------


     for (j=1;j<=nn;j++)
       A[j][j]+=shift;


/****** END OF QR TRANSFORMATION  ********************************/

    }//End if noluck


 }// end of while

//--------Analyze rest of matrix. Compute last eigenvalues---------------

     if (nn==1) //Last single  value
      {


        if (nabs(A[1][1]) > 1-eps2 )  rval=1;//We have found large value
        else                          rval=0;//Last value is also small

        nn--;
      }

     if (nn==2)// Last pair of eigenvalues
       {

         b=0.5*(A[1][1]+A[2][2]);
         c=A[1][1]*A[2][2]-A[2][1]*A[1][2];
         d=b*b-c;

         if (d>eps)  //Discriminant > 0 --- A real pair
            {
               d=sqrt(d);




               if ((nabs(b+d)<1-eps2) && (nabs(b-d)<1-eps2)) rval=0;
               else    rval=1;

            }
         else// if (d>eps)
           {
             if (d<-eps)// Discriminant<0 --- A complex pair
               {





                 if (b*b-d > 1-eps2) rval=1;
                 else   rval=0;


               }
              else  //Discriminant=0 --- Double real eigenvalue
               {



                  if (nabs(b)>1-eps2) rval=1;
                  else rval=0;


               }

           }//end of else
       nn-=2;
     } //end if (nn==2)

 delete[] A[0];
 delete[] A;

 delete[] cc;
 delete[] ss;

 return rval;

}
