/** -*- c++ -*- 
 *
 * \file   fc_layer.cpp
 * \date   Sat May 14 11:36:52 2016
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
 * \brief  fully connected layer
 */

#include "_dnn.h" 

void icvCNNDenseClear( CvDNNLayer * p_layer );

/*************************************************************************/
ML_IMPL
CvDNNLayer * cvCreateDenseLayer( 
    const int dtype, const char * name, const int visualize,
    const CvDNNLayer * input_layer, int n_inputs, int n_outputs, 
    float init_learn_rate, int learn_rate_decrease_type, const char * activation,
    CvMat * weights )
{
  CvDNNDenseLayer* layer = 0;

  CV_FUNCNAME("cvCreateDenseLayer");
  __BEGIN__;

  if ( init_learn_rate <= 0) {
    CV_ERROR( CV_StsBadArg, "Incorrect parameters" );
  }

  fprintf(stderr,"DenseLayer(%s): input (%d), output (%d)\n", name,
          n_inputs,n_outputs);
  
  CV_CALL(layer = (CvDNNDenseLayer*)icvCreateLayer( ICV_DNN_FULLCONNECT_LAYER, dtype, name, 
      sizeof(CvDNNDenseLayer), n_inputs, 1, 1, n_outputs, 1, 1,
      init_learn_rate, learn_rate_decrease_type,
      icvCNNDenseRelease, icvCNNDenseForward, icvCNNDenseBackward ));

  layer->WX = 0;
  layer->dE_dW = 0;
  layer->seq_length = 1;
  layer->clear = icvCNNDenseClear;

  strcpy(layer->activation,activation);
  layer->visualize = visualize;
  layer->input_layers.push_back((CvDNNLayer*)input_layer);

  CV_CALL(layer->weights = cvCreateMat( n_outputs, n_inputs+1, dtype ));
  if ( weights ){
    if ( !ICV_IS_MAT_OF_TYPE( weights, dtype ) ) {
      CV_ERROR( CV_StsBadSize, "Type of initial weights matrix must be CV_32F" );
    }
    if ( !CV_ARE_SIZES_EQ( weights, layer->weights ) ) {
      CV_ERROR( CV_StsBadSize, "Invalid size of initial weights matrix" );
    }
    CV_CALL(cvCopy( weights, layer->weights ));
  } else {
    CvRNG rng = cvRNG( 0xFFFFFFFF );
    cvRandArr( &rng, layer->weights, CV_RAND_UNI, 
               cvScalar(-1.f/sqrt(n_inputs)), cvScalar(1.f/sqrt(n_inputs)) );
    // initialize bias to zero
    for (int ii=0;ii<n_outputs;ii++){ CV_MAT_ELEM(*layer->weights,float,ii,n_inputs)=0; }
  }

  __END__;

  if ( cvGetErrStatus() < 0 && layer ){
    cvReleaseMat( &layer->WX );
    cvReleaseMat( &layer->weights );
    cvFree( &layer );
  }

  return (CvDNNLayer*)layer;
}
/****************************************************************************************/
void icvCNNDenseForward( CvDNNLayer* _layer, const CvMat* _X, CvMat* _Y )
{
  CV_FUNCNAME("icvCNNDenseForward");

  if ( !icvIsDenseLayer(_layer) ) {
    CV_ERROR( CV_StsBadArg, "Invalid layer" );
  }

  __BEGIN__;

  CvDNNDenseLayer * layer = (CvDNNDenseLayer*)_layer;
  int dtype = layer->dtype;
  CvDNNLayer * input_layer = layer->input_layers.size()>0?layer->input_layers[0]:0;
  CvMat * weights = layer->weights;
  CvMat sub_weights, biascol;
  CvMat * X = (CvMat*)_X;
  CvMat * Y = (CvMat*)_Y;
  int n_inputs = layer->n_input_planes;
  int n_outputs = Y->cols;
  int seq_length = 1;
  int batch_size = X->rows;
  
  if (input_layer){
    if (icvIsSimpleRNNLayer(input_layer)){
      seq_length = ((CvDNNSimpleRNNLayer*)input_layer)->seq_length;
      batch_size = X->rows/seq_length;
    }else if (icvIsMaxPoolingLayer(input_layer)){
      seq_length = 1;
      n_inputs = input_layer->n_output_planes*input_layer->output_height*input_layer->output_width;
    }
    CvMat * Xsrc = input_layer->Y;
    if (layer->n_output_planes*seq_length==Y->cols){
      X = cvCreateMat(batch_size,n_inputs*seq_length,dtype);
      CV_ASSERT(n_inputs*seq_length*batch_size==Xsrc->rows*Xsrc->cols);
      cvCopy(Xsrc,X);
      n_outputs=layer->n_output_planes/seq_length;
      CV_ASSERT(n_outputs*seq_length==layer->n_output_planes);
    }else{
      X = cvCreateMat(batch_size,n_inputs,dtype);
      CV_ASSERT(n_inputs*seq_length*batch_size==Xsrc->rows*Xsrc->cols);
      CvMat X_submat; 
      if (icvIsSimpleRNNLayer(input_layer)){
        cvGetRow(Xsrc,&X_submat,((CvDNNSimpleRNNLayer*)input_layer)->time_index);
      }else{
        cvGetRow(Xsrc,&X_submat,0);
      }
      cvCopy(&X_submat,X);
      seq_length=1;
    }
  }else if (icvIsSimpleRNNLayer(layer->prev_layer) && n_inputs*seq_length!=X->rows){
    CvDNNSimpleRNNLayer * rnn_layer = ((CvDNNSimpleRNNLayer*)layer->prev_layer);
    CV_ASSERT(X->cols==rnn_layer->seq_length*n_inputs);
    X = cvCreateMat(batch_size,n_inputs,dtype);
    CvMat X_submat;
    cvGetSubRect(_X,&X_submat,cvRect(n_inputs*rnn_layer->time_index,0,n_inputs,batch_size));
    cvCopy(&X_submat,X);
  }

  CV_ASSERT(X->rows == batch_size && X->cols == layer->n_input_planes*seq_length);
  CV_ASSERT(Y->rows == batch_size && Y->cols == layer->n_output_planes);
  CV_ASSERT(cvCountNAN(X)<1);

  CvRect roi = cvRect(0, 0, weights->cols-1, weights->rows );
  CV_CALL(cvGetSubRect( weights, &sub_weights, roi));
  CV_CALL(cvGetCol( weights, &biascol, weights->cols-1));
  CvMat * bias = cvCreateMat(biascol.rows,batch_size,dtype); 
  cvRepeat(&biascol,bias);
  layer->WX = cvCreateMat( batch_size, n_outputs, dtype ); cvZero( layer->WX );
  CV_CALL(cvGEMM( X, &sub_weights, 1, bias, 1, layer->WX, CV_GEMM_B_T+CV_GEMM_C_T ));
  if (bias){cvReleaseMat(&bias);bias=0;}

  if (!strcmp(layer->activation,"none")){ // do nothing
  }else if (!strcmp(layer->activation,"tanh")){ CV_CALL(cvTanh( layer->WX, Y ));
  }else if (!strcmp(layer->activation,"sigmoid")){ CV_CALL(cvSigmoid( layer->WX, Y ));
  }else if (!strcmp(layer->activation,"relu")){ CV_CALL(cvReLU( layer->WX, Y ));
  }else if (!strcmp(layer->activation,"softmax")){
    cvSoftmax( layer->WX, Y ); CV_ASSERT(Y->rows == batch_size && Y->cols == layer->n_output_planes);
  }else{CV_ERROR(CV_StsBadArg,"Unknown activation type");}

  if (layer->Y){
    if (layer->Y->rows==Y->rows){cvCopy(Y,layer->Y);}else{cvReleaseMat(&layer->Y);layer->Y=cvCloneMat(Y);}
  }else{layer->Y=cvCloneMat(Y);}
  if (layer->visualize==1){icvVisualizeCNNLayer((CvDNNLayer*)layer,Y);}
  else if (layer->visualize==2){fprintf(stderr,"\n");cvPrintf(stderr,"%f ",Y);}

  if ( (input_layer && icvIsSimpleRNNLayer(input_layer)) ||
       (icvIsSimpleRNNLayer(layer->prev_layer) && n_inputs*seq_length!=X->cols) ){
    CV_ASSERT(X!=_X); if (X) { cvReleaseMat(&X); X = 0; }
  }
  __END__;
}


/****************************************************************************************/
/* <dE_dY>, <dE_dX> should be row-vectors.
   Function computes partial derivatives <dE_dX>, <dE_dW>
   of the loss function with respect to the planes components
   of the previous layer (X) and the weights of the current layer (W)
   and updates weights od the current layer by using <dE_dW>.
   It is a basic function for back propagation method.
   Input parameter <dE_dY> is the partial derivative of the
   loss function with respect to the planes components
   of the current layer. */
void icvCNNDenseBackward(CvDNNLayer * _layer, int t,
                               const CvMat * _X, const CvMat * _dE_dY, CvMat * _dE_dX )
{
  CvMat* dE_dY_afder = 0;
  CvMat* dE_dW = 0;

  CV_FUNCNAME( "icvCNNDenseBackward" );

  if ( !icvIsDenseLayer(_layer) ) {
      CV_ERROR( CV_StsBadArg, "Invalid layer" );
  }

  __BEGIN__;

  int i;
  CvDNNDenseLayer * layer = (CvDNNDenseLayer*)_layer;
  int dtype = layer->dtype;
  CvDNNLayer * input_layer = layer->input_layers.size()>0?layer->input_layers[0]:0;
  CvDNNLayer * output_layer = layer->output_layers.size()>0?layer->output_layers[0]:0;
  int n_outputs = layer->n_output_planes;
  int n_inputs  = layer->n_input_planes;
  CvMat * weights = layer->weights;
  CvMat sub_weights, Xtemplate, WXrow;
  int batch_size = _dE_dY->rows; CV_ASSERT(_dE_dY->rows==_X->rows);
  int seq_length = 1, time_index = 0;
  CvMat * X = (CvMat*)_X;
  CvMat * dE_dY = (CvMat*)_dE_dY;
  CvMat * dE_dX = (CvMat*)_dE_dX;

  if (input_layer){
    n_inputs = input_layer->n_output_planes;
    if (icvIsSimpleRNNLayer(input_layer)){
      seq_length = ((CvDNNSimpleRNNLayer*)input_layer)->seq_length;
      time_index = ((CvDNNSimpleRNNLayer*)input_layer)->time_index;
    }else if (icvIsMaxPoolingLayer(input_layer) || icvIsConvolutionLayer(input_layer)){ 
      seq_length = 1; time_index = 0; 
      n_inputs = input_layer->n_output_planes*input_layer->output_height*input_layer->output_width;
    }
    CvMat X_submat; CvMat * Xsrc = input_layer->Y;
    X = cvCreateMat(batch_size,n_inputs,dtype);
    CV_ASSERT(n_inputs*seq_length*batch_size==Xsrc->rows*Xsrc->cols);
    cvGetRows(Xsrc,&X_submat,batch_size*time_index,batch_size*(time_index+1));
    cvCopy(&X_submat,X); 
    // initialize dE_dX in layer member variable
    if (!layer->dE_dX){
      layer->dE_dX = cvCreateMat(batch_size, n_inputs, dtype); 
    }else{CV_ASSERT(layer->dE_dX->rows==batch_size && layer->dE_dX->cols==n_inputs);}
    cvZero(layer->dE_dX);
    seq_length=1; dE_dX = layer->dE_dX;
  }else if (icvIsSimpleRNNLayer(layer->prev_layer)){
    CvDNNSimpleRNNLayer * rnn_layer = (CvDNNSimpleRNNLayer*)layer->prev_layer;
    if (X->rows!=n_inputs){
      CV_ASSERT(X->rows==rnn_layer->seq_length*n_inputs);
      X = cvCreateMat(n_inputs,batch_size,dtype);
      CvMat X_submat;
      cvGetSubRect(_X,&X_submat,cvRect(0,n_inputs*rnn_layer->time_index,batch_size,n_inputs));
      cvCopy(&X_submat,X);
      // initialize dE_dX in layer member variable
      if (!layer->dE_dX) { 
        layer->dE_dX = cvCreateMat(batch_size, n_inputs, dtype); 
      }else{CV_ASSERT(layer->dE_dX->rows==batch_size && layer->dE_dX->cols==n_inputs);}
      cvZero(layer->dE_dX);
      dE_dX = layer->dE_dX;
    }
  }

  if (output_layer){
    if (icvIsMergeLayer(output_layer)){
      int n_input_layers = ((CvDNNMergeLayer*)output_layer)->input_layers.size();
      int layer_index = -1;
      int output_layer_index = 0;
      int output_layer_size = 0;
      for (int lidx=0;lidx<n_input_layers;lidx++){
        output_layer_index+=((CvDNNMergeLayer*)output_layer)->input_layers[lidx]->n_output_planes;
        if (!strcmp(((CvDNNMergeLayer*)output_layer)->input_layers[lidx]->name,layer->name)){
          layer_index=lidx;
          output_layer_index-=((CvDNNMergeLayer*)output_layer)->input_layers[lidx]->n_output_planes;
          output_layer_size=((CvDNNMergeLayer*)output_layer)->input_layers[lidx]->n_output_planes;
          break;
        }
      }
      if (layer_index>=0){
        dE_dY = cvCreateMat(batch_size,output_layer_size,CV_32F);
        CvMat dE_dX_submat; 
        cvGetCols(output_layer->dE_dX,&dE_dX_submat,output_layer_index,output_layer_index+output_layer_size);
        cvCopy(&dE_dX_submat,dE_dY);
      }else{CV_ERROR(CV_StsBadArg, "output_layer->input_layer should be current layer.");}
    }
  }

  CV_ASSERT(X->rows == batch_size && X->cols == n_inputs);
  CV_ASSERT(dE_dY->rows == batch_size && dE_dY->cols == n_outputs );
  CV_ASSERT(dE_dX->rows == batch_size && dE_dX->cols == n_inputs );

  CV_CALL(dE_dY_afder = cvCreateMat( batch_size, n_outputs, dtype ));

  // compute (tanh'(WX))*dE_dY
  if (!strcmp(layer->activation,"none")){
  }else if (!strcmp(layer->activation,"tanh")){ 
    cvTanhDer(layer->WX,dE_dY_afder);
    cvMul(dE_dY_afder,dE_dY,dE_dY_afder);
  }else if (!strcmp(layer->activation,"sigmoid")){ 
    cvSigmoidDer(layer->WX,dE_dY_afder);
    cvMul(dE_dY_afder,dE_dY,dE_dY_afder);
  }else if (!strcmp(layer->activation,"softmax")){ 
    cvSoftmaxDer(layer->WX,(CvMat*)dE_dY,dE_dY_afder);
  }else if (!strcmp(layer->activation,"relu")){ 
    cvReLUDer(layer->WX,dE_dY_afder);
    cvMul(dE_dY_afder,dE_dY,dE_dY_afder);
  }else{CV_ASSERT(false);}

  // compute dE_dX=dE_dY_afder*W
  CV_CALL(cvGetCols( weights, &sub_weights, 0, weights->cols-1 ));
  CV_CALL(cvGEMM( dE_dY_afder, &sub_weights, 1, 0, 1, dE_dX));
  
  // compute dE_dW=dE_dY*X
  CV_ASSERT( dE_dY->rows == batch_size && dE_dY->cols == n_outputs );
  dE_dW = cvCreateMat(n_outputs,n_inputs+1,dtype); cvZero(dE_dW);
  CvMat * Xcol = cvCreateMat(X->rows,X->cols+1,dtype); 
  cvSet(Xcol,cvScalar(1)); // all ones on last row
  CvMat Xcol_submat; cvGetCols(Xcol,&Xcol_submat,0,X->cols); cvCopy(X,&Xcol_submat);
  cvGEMM(dE_dY,Xcol,1,0,1,dE_dW,CV_GEMM_A_T);
  cvReleaseMat(&Xcol);

  // copy `dE_dW` into layer variable for gradient checking
  if (!layer->dE_dW){layer->dE_dW = cvCloneMat(dE_dW);}else{cvCopy(dE_dW,layer->dE_dW);}
  if (input_layer){
    if (layer->dE_dX){cvCopy(dE_dX,layer->dE_dX);}else{layer->dE_dX=cvCloneMat(dE_dX);}
  }else{
    if (layer->dE_dX){
      CV_ERROR(CV_StsBadArg, "layer->dE_dX should not be initialize if the layer don't have input_layer defined.");
    }
  }

  // 2) update weights
  float eta;
  if ( layer->decay_type == CV_DNN_LEARN_RATE_DECREASE_LOG_INV ){
    eta = -layer->init_learn_rate/logf(1+(float)t);
  }else if ( layer->decay_type == CV_DNN_LEARN_RATE_DECREASE_SQRT_INV ){
    eta = -layer->init_learn_rate/sqrtf((float)t);
  }else{
    eta = -layer->init_learn_rate/(float)t;
  }
  cvScaleAdd( dE_dW, cvRealScalar(eta), weights, weights );

  // if (output_layer && dE_dY){cvReleaseMat(&dE_dY);dE_dY=0;}
  if (layer->WX){ cvReleaseMat(&layer->WX);layer->WX=0; }
  __END__;

  cvReleaseMat( &dE_dY_afder );
  cvReleaseMat( &dE_dW );
}


/****************************************************************************************/
void icvCNNDenseRelease( CvDNNLayer** p_layer )
{
  CV_FUNCNAME("icvCNNDenseRelease");
  __BEGIN__;

  CvDNNDenseLayer* layer = 0;

  if ( !p_layer )
      CV_ERROR( CV_StsNullPtr, "Null double pointer" );

  layer = *(CvDNNDenseLayer**)p_layer;

  if ( !layer )
      return;
  if ( !icvIsDenseLayer((CvDNNLayer*)layer) )
      CV_ERROR( CV_StsBadArg, "Invalid layer" );

  cvReleaseMat( &layer->WX );
  cvReleaseMat( &layer->weights );
  cvFree( p_layer );

  __END__;
}

void icvCNNDenseClear( CvDNNLayer * p_layer )
{
  CV_FUNCNAME("icvCNNDenseClear");
  __BEGIN__;

  CvDNNDenseLayer* layer = 0;

  if ( !p_layer )
      CV_ERROR( CV_StsNullPtr, "Null double pointer" );

  layer = (CvDNNDenseLayer*)p_layer;

  if ( !layer )
      return;
  if ( !icvIsDenseLayer((CvDNNLayer*)layer) )
      CV_ERROR( CV_StsBadArg, "Invalid layer" );

  if (layer->WX) { cvReleaseMat( &layer->WX ); layer->WX=0; }

  __END__;
}
