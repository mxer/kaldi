# Personal Kaldi Script & Program & Note

## HMM related
* hmm matlab isolated word recognition: http://sourceforge.net/projects/hmm-asr-matlab/files/temp/


## Directory details
* fanbo: for kaldi alignment and get kaldi align time
    * DONE frame accuracy(nnet-loss.cc Xent)
    * TODO alignment to triphone
* dnn_scripts
* kaldi pitch


## Kaldi program tips TODO

* DONE uint test
* TODO thread wrap
* DONE log assert
* TODO io binary
* TODO memory free

## Some Api
* ConvertStringToInteger ����תint

## Kadli Matrix Api

### Vector
Vector and SubVector are the child class of VectorBase, and SubVector
can not change size.
* Init, Vector<BaseFloat> vec(dim)
* ��0, vec.SetZero()
* ��Ϊĳ��ֵ, vec.Set(val)
* ά�ȣ� vec.Dim()
* ����Ԫ�� vec(i)
* ������ SubVector vec = vec.Range(start, len)
* ����N vec.scale(1.0 / N)
* �����ڻ� VecVec(vec1, vec2)
* �Ӽ����� add: vec.AddVec(1.0, vec1) sub: vec.AddVec(-1.0, vec1)
* Resize

### Matrix
Matrix and SubMatrix are the child class of MatrixBase, and SubMatrix
can not change size.
* ��0, Matrix<BaseFloat> mat(row, col)
* ��Ϊĳֵ��mat.SetZero()
* ά�ȣ� mat.NumRows() mat.NumCols()
* ����Ԫ�� mat(i,j)
* ������ SubVector vec = vec.Row(i)
* ������ SubVector vec = vec.Col(i)
* �Ӿ��� vec.Range(row_offset, num_rows, col_offset, num_cols)
* �Ӿ��� vec.RowRange(row_offset, num_rows)
* �Ӿ��� vec.ColRange(col_offset, num_cols)
* ����N vec.scale(1.0 / N)
* Resize
* �Ӽ�: mat.AddMat(1.0, mat1); *this += alpha * M [or M^T]
* �ӳ�: A.AndMatMat(alpha, B, kNoTrans, C, kTrans, beta); // *this = beta* this + alpha * B * C
* ���: A.MulElements(B)
