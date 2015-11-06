# Note on HTK

## Tools
* HCopy
* HCompV 
* HLED �ı��༭�����ɵ����غ��������б�
* HHED HMM Edit Tools, �޸�hmm�ļ�, sil��sp�󶨣�������
* HINIT ��ʼ��
* HRest ��ʼ����, ����HINT
* HERest �����ع�

## HLED

## HInit
�������зֳ�����(����HMM)�����з��ڲ��Ը�HMM��Viterbi����,���Ʋ���
HInit first divides the training observation vectors equally
amongst the model states and then uses equations 1.11 and 1.12 to give initial values for the mean
and variance of each state. It then finds the maximum likelihood state sequence using the Viterbi
algorithm described below, reassigns the observation vectors to states and then uses equations 1.11
and 1.12 again to get better initial values. This process is repeated until the estimates do not
change.
```cpp
if (newModel){
    // �ȷֳ�ʼ��
    UniformSegment();
}
totalP=LZERO;
// ����maxIter��
for (iter=1; !converged && iter<=maxIter; iter++){
    ZeroAccs(&hset, uFlags);              /* Clear all accumulators */
    numSegs = NumSegs(segStore);
    /* Align on each training segment and accumulate stats */
    for (newP=0.0,i=1;i<=numSegs;i++) {
        segLen = SegLength(segStore,i);
        states = CreateIntVec(&gstack,segLen);
        mixes  = (hset.hsKind==DISCRETEHS)? NULL : CreateMixes(&gstack,segLen);
        // Viterbi����
        newP += ViterbiAlign(i,segLen,states,mixes);
        if (trace&T_ALN) ShowAlignment(i,segLen,states,mixes);
        UpdateCounts(i,segLen,states,mixes);
        FreeIntVec(&gstack,states); /* disposes mixes too */
    }    
    /* Update parameters or quit */
    newP /= (float)numSegs;
    delta = newP - totalP;
    converged = ((iter>1) && (fabs(delta) < epsilon)) ? TRUE:FALSE;
    // �����ع�
    if (!converged)
        UpdateParameters();
    //...
}

```

## HRest
��ÿ��HMM�ڣ�������EM�ع�, ����HMM�Ļ���EM�㷨
HRest performs basic Baum-Welch re-estimation of the parameters of a single HMM using a set
of observation sequences.
```cpp
/* ReEstimateModel: top level of algorithm */
void ReEstimateModel(void)
{
    LogFloat segProb,oldP,newP,delta;
    LogDouble ap,bp;
    int converged,iteration,seg;

    iteration=0; 
    oldP=LZERO;
    do {        /*main re-est loop*/   
        ZeroAccs(&hset, uFlags); newP = 0.0; ++iteration;
        nTokUsed = 0;
        for (seg=1;seg<=nSeg;seg++) {
            T=SegLength(segStore,seg);
            SetOutP(seg);
            if ((ap=SetAlpha(seg)) > LSMALL){
                bp = SetBeta(seg);
                if (trace & T_LGP)
                    printf("%d.  Pa = %e, Pb = %e, Diff = %e\n",seg,ap,bp,ap-bp);
                segProb = (ap + bp) / 2.0;  /* reduce numeric error */
                newP += segProb; ++nTokUsed;
                UpdateCounters(segProb,seg);
            } else
                if (trace&T_TOP) 
                    printf("Example %d skipped\n",seg);
        }
        if (nTokUsed==0)
            HError(2226,"ReEstimateModel: No Usable Training Examples");
        UpdateTheModel();
        newP /= nTokUsed;
        delta=newP-oldP; oldP=newP;
        converged=(fabs(delta)<epsilon); 
        if (trace&T_TOP) {
            printf("Ave LogProb at iter %d = %10.5f using %d examples",
                    iteration,oldP,nTokUsed);
            if (iteration > 1)
                printf("  change = %10.5f",delta);
            printf("\n");
            fflush(stdout);
        }
    } while ((iteration < maxIter) && !converged);
    
}
```
## HERest
�����仰����ǰ�������룬�ع�������

## HLED
