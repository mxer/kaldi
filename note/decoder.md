# Note on ASR Decoder

## TODO
��Ԫ�� 942M �ڹ�ͼ�����л�ռ��70%���ң�64G���ڴ�

## ���ݴ���
* amr
* bv

## Kaldi Faster Decoder
* ͨ��max_activeȷ��weight_cutoff, adptive_beam(max_active_cutoff - best_cost)
* next_weight_cutoff = new_weight + adaptive_beam
