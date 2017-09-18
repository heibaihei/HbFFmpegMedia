%module MediaEdit
%{
#include "MediaFilter.h"
#include "MTVideoTools.h"
%}

%include "typemaps.i"

%typemap(jstype) double[] "double[]"
%typemap(jtype) double[] "double[]"
%typemap(jni) double[] "jdoubleArray"
%typemap(javain) double[] "$javainput"
%typemap(in) double[] {
     $1 = JCALL2(GetDoubleArrayElements, jenv, $input, NULL);
}
%typemap(argout) double[]{
    JCALL3(ReleaseDoubleArrayElements,jenv, $input, $1, 0);
}

%apply (char *STRING, size_t LENGTH) { (uint8_t data [], size_t len) }
%apply int *OUTPUT { int *videoWidth, int *videoHeight};
%ignore getFrameRGBAData(float mediaPos, int *videoWidth, int *videoHeight);

//int generateThumb(const char *src, const char* save_path, double *times, int length);
%pointer_functions(double, doublep);
%include "MediaFilter.h"
%include "MTVideoTools.h"
