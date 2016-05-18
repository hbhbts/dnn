/** -*- mode: c++ -*- 
 *
 * \file   _dnn.h
 * \date   Sat May 14 20:17:45 2016
 *
 * \copyright 
 * Copyright (c) 2016 Liangfu Chen <liangfu.chen@nlpr.ia.ac.cn>.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the Brainnetome Center & NLPR at Institute of Automation, CAS. The 
 * name of the Brainnetome Center & NLPR at Institute of Automation, CAS 
 * may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * \brief  internal functions for implementing layers
 */
#ifndef __DNN_H__
#define __DNN_H__

#include "cvext_c.h"
#include "layers.h"
#include "precomp.hpp"

/*-------------- functions for input data layer -----------------------*/
void icvCNNInputDataRelease( CvCNNLayer** p_layer );
void icvCNNInputDataForward( CvCNNLayer* layer, const CvMat* X, CvMat* Y );
void icvCNNInputDataBackward( CvCNNLayer* layer, int t, const CvMat*, const CvMat* dE_dY, CvMat* dE_dX );

/*--------------- functions for convolutional layer --------------------*/
void icvCNNConvolutionRelease( CvCNNLayer** p_layer );
void icvCNNConvolutionForward( CvCNNLayer* layer, const CvMat* X, CvMat* Y );
void icvCNNConvolutionBackward( CvCNNLayer*  layer, int t, const CvMat* X, const CvMat* dE_dY, CvMat* dE_dX );

/*------------------ functions for sub-sampling layer -------------------*/
void icvCNNSubSamplingRelease( CvCNNLayer** p_layer );
void icvCNNSubSamplingForward( CvCNNLayer* layer, const CvMat* X, CvMat* Y );
void icvCNNSubSamplingBackward( CvCNNLayer*  layer, int t, const CvMat* X, const CvMat* dE_dY, CvMat* dE_dX );

/*---------- functions for full connected layer -----------------------*/
void icvCNNFullConnectRelease( CvCNNLayer** p_layer );
void icvCNNFullConnectForward( CvCNNLayer* layer, const CvMat* X, CvMat* Y );
void icvCNNFullConnectBackward( CvCNNLayer* layer, int t, const CvMat*, const CvMat* dE_dY, CvMat* dE_dX );

/*-------------- functions for recurrent layer -----------------------*/
void icvCNNRecurrentRelease( CvCNNLayer** p_layer );
void icvCNNRecurrentForward( CvCNNLayer* layer, const CvMat* X, CvMat* Y );
void icvCNNRecurrentBackward( CvCNNLayer* layer, int t, const CvMat*, const CvMat* dE_dY, CvMat* dE_dX );

/*-------------- functions for multi target layer -----------------------*/
void icvCNNMultiTargetRelease( CvCNNLayer** p_layer );
void icvCNNMultiTargetForward( CvCNNLayer* layer, const CvMat* X, CvMat* Y );
void icvCNNMultiTargetBackward( CvCNNLayer* layer, int t, const CvMat*, const CvMat* dE_dY, CvMat* dE_dX );

CvCNNLayer* icvCreateCNNLayer( int layer_type, 
    const int dtype, const char * name, int header_size,
    int n_input_planes, int input_height, int input_width,
    int n_output_planes, int output_height, int output_width,
    float init_learn_rate, int learn_rate_decrease_type,
    CvCNNLayerRelease release, CvCNNLayerForward forward, CvCNNLayerBackward backward );

void icvVisualizeCNNLayer(CvCNNLayer * layer, const CvMat * Y);

#endif // __DNN_H__