#include "LLL.h"
#include <math.h>

namespace OPS
{
namespace LatticeGraph
{
namespace CountIntPoints
{

RationalNumber innerProduct(RationalNumber* u, RationalNumber* v, int n) \
{
    RationalNumber s(0);
    for (int i = 0; i < n; ++i) 
        s += u[i]*v[i];
    return s;
}

double innerProduct(double* u, double* v, int n) \
{
    double s = 0;
    for (int i = 0; i < n; ++i) 
        s += u[i]*v[i];
    return s;
}

double innerProduct(RationalNumber* u, double* v, int n) \
{
    double s = 0;
    for (int i = 0; i < n; ++i) 
        s += u[i].cast2Double() * v[i];
    return s;
}


void LLL(RationalNumber** a, int n, double delta)
{
	double **b,**mu,*normB2, **mu_proverka, **b_proverka;
    b = new double*[n]; 
    b_proverka = new double*[n]; 
    mu = new double*[n]; 
    mu_proverka = new double*[n]; 
    normB2 = new double[n];
    int perestavlaemInd=0, newPerestavlaemInd=n-2;
    RationalNumber* tmp;
    bool flagLLLFinished = false,  flagNashliPerestanovku = false;
	double newMu;

    //выделяем память для базиса Грамма-Шмидта и mu
    for (int i=0; i<n; ++i) b[i] = new double[n];
    for (int i=0; i<n; ++i) b_proverka[i] = new double[n];
    for (int i=0; i<n; ++i) mu[i] = new double[n];
    for (int i=0; i<n; ++i) mu_proverka[i] = new double[n];

    while (!flagLLLFinished)
    {
        //считаем базис Грамма-Шмидта
        flagNashliPerestanovku = false;
        for (int k = perestavlaemInd; (k < n) && (!flagNashliPerestanovku); ++k)
        {
            //считаем mu 
            for (int m = 0; m < k; ++m) 
            {
                mu[k][m] = innerProduct(a[k],b[m],n)/normB2[m];
            }
            //считаем ортогональные векторы
            for (int j = 0; j < n; ++j)
            {
                //считаем сумму mu[k][l]*b[l]
                double s=0;
                for (int l = 0; l < k; ++l) s+=mu[k][l]*b[l][j];
                b[k][j] = a[k][j].cast2Double() - s;
            }
            //считаем квадрат нормы вектора
            normB2[k] = innerProduct(b[k],b[k],n);
            //выполняем приближенную ортогонализацию
            for (int l=k-1; l>=0; --l)
            {
                int floorMu = floor(mu[k][l]+0.49);
                //отнимаем от a_k вектор a_l*floorMu
                for (int j = 0; j < n; ++j)
                {
                    a[k][j] -= RationalNumber(floorMu)*a[l][j];
                }
                //пересчитываем все mu_k_j -= floorMu*mu_l_j
                for (int j = 0; j < l; ++j)
                {
                    mu[k][j] -= floorMu*mu[l][j];
                }
                mu[k][l] -= floorMu;
            }
            //проверяем условие перестановки векторов
            if (k > 0)
            {
                newMu=mu[k][k-1];
                if (normB2[k] < (delta-newMu*newMu)*normB2[k-1]) 
                {
                    newPerestavlaemInd = k-1;
                    flagNashliPerestanovku = true;
                }
                else
                {
                    if (k == n-1) 
                    {
                        flagLLLFinished = true;
                        newPerestavlaemInd = n-2;
                    }
                }
            }
        }
        //переставляем векторы
        perestavlaemInd = newPerestavlaemInd;
        tmp = a[perestavlaemInd];
        a[perestavlaemInd] = a[perestavlaemInd+1];
        a[perestavlaemInd+1] = tmp;
    }
}

}//end of namespace
}//end of namespace
}//end of namespace

