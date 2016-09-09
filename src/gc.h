/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   gc.h
 * Author: primi
 *
 * Created on 8 de septiembre de 2016, 09:03 AM
 */

#ifndef GC_H
#define GC_H

#include <time.h>

#include "liblist.h"
#include "object.h"
#include "vm.h"

#define HABILITAR_GC 1

#define TIME_THIS(X)                                         \
  {                                                          \
    struct timespec ts1, ts2;                                \
    clock_gettime( CLOCK_REALTIME, &ts1 );                   \
    X;                                                       \
    clock_gettime( CLOCK_REALTIME, &ts2 );                   \
    printf( #X " demora: %f\n",                              \
      (float) ( 1.0*(1.0*ts2.tv_nsec - ts1.tv_nsec*1.0)*1e-9 \
      + 1.0*ts2.tv_sec - 1.0*ts1.tv_sec ) );                 \
  }

void __colector_agregar(lat_mv* vm, lat_objeto* o);
void __colector_limpiar(lat_mv* vm);

#endif /* GC_H */
