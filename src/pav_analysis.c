#include <math.h>
#include "pav_analysis.h"

float compute_power(const float *x, unsigned int N) {
    float suma = 0;
    float power;
    for (int i = 0; i < N; i++)
    {
        suma = suma + x[i]*x[i];
    }
    power = suma/N;
    return 10*log10(power);
}

float compute_am(const float *x, unsigned int N) {
    float suma=0;
    for (int i = 0; i < N; i++)
    {
        suma = suma + fabs (x[i]);
    }
    return suma/N;
}

float compute_zcr(const float *x, unsigned int N, float fm) {
    float suma = 0;
    for (int i = 0; i < N; i++)
    {
        if ((x[i-1]>=0 && x[i]<=0)||(x[i-1]<=0 && x[i]>=0))
        {
            suma = suma + 1;
        }
    }
    return (suma*fm)/(2*(N-1));
}
